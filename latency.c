/**
 * @{
 *
 * @file
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 *
 * This implementation oriented itself on the [version by Mike
 * Muuss](http://ftp.arl.army.mil/~mike/ping.html) which was published under
 * public domain. The state-handling and duplicate detection was inspired by the
 * ping version of [inetutils](://www.gnu.org/software/inetutils/), which was
 * published under GPLv3
 *
 */
#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include "doriot_dca/latency.h"
#include "doriot_dca/linked_list.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include "bitfield.h"
#include "byteorder.h"
#include "msg.h"
#include "net/gnrc.h"
#include "net/gnrc/icmpv6.h"
#include "net/icmpv6.h"
#include "net/ipv6.h"
#include "timex.h"
#include "utlist.h"
#include "xtimer.h"
#include "fmt.h"
#include "xfa.h"
#include "shell.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#if(POSIX_C_SOURCE < 200809L && _XOPEN_SOURCE < 700)
char *strndup(const char *s, size_t n)
{
    char *ret = malloc(n);
    strcpy(ret, s);
    return ret;
}
#endif

typedef struct {
    gnrc_netreg_entry_t netreg;
    xtimer_t sched_timer;
    msg_t sched_msg;
    ipv6_addr_t host;
    char *hostname;
    unsigned long num_sent, num_recv, num_rept;
    unsigned long long tsum;
    unsigned tmin, tmax;
    unsigned count;
    size_t datalen;
    BITFIELD(cktab, CKTAB_SIZE);
    uint32_t timeout;
    uint32_t interval;
    gnrc_netif_t *netif;
    uint16_t id;
    uint8_t hoplimit;
    uint8_t pattern;
} _ping_data_t;

static int _configure(ipv6_addr_t *neigh, _ping_data_t *data);
static void _pinger(_ping_data_t *data);
static void _print_reply(_ping_data_t *data, gnrc_pktsnip_t *icmpv6,
                         ipv6_addr_t *from, unsigned hoplimit, gnrc_netif_hdr_t *netif_hdr);
static void _handle_reply(_ping_data_t *data, gnrc_pktsnip_t *pkt);
static int _finish(_ping_data_t *data);

int db_measure_network_latency(void)
{
    int res = 0;
    unsigned iface = 0;
    void *state = NULL;
    gnrc_ipv6_nib_nc_t nce;

    while (gnrc_ipv6_nib_nc_iter(iface, &state, &nce)) {
        _ping_data_t data = {
            .netreg = GNRC_NETREG_ENTRY_INIT_PID(ICMPV6_ECHO_REP,
                                                 thread_getpid()),
            .count = DEFAULT_COUNT,
            .tmin = UINT_MAX,
            .datalen = DEFAULT_DATALEN,
            .timeout = DEFAULT_TIMEOUT_USEC,
            .interval = DEFAULT_INTERVAL_USEC,
            .id = DEFAULT_ID,
            .pattern = DEFAULT_ID,
        };

        if ((res = _configure(&nce.ipv6, &data)) != 0) {
            return res;
        }
        gnrc_netreg_register(GNRC_NETTYPE_ICMPV6, &data.netreg);
        _pinger(&data);
        do{
            msg_t msg;
            msg_receive(&msg);
            switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
            {
                _handle_reply(&data, msg.content.ptr);
                gnrc_pktbuf_release(msg.content.ptr);
                break;
            }
            case _SEND_NEXT_PING:
                _pinger(&data);
                break;
            case _PING_FINISH:
                goto finish;
            default:
                /* requeue wrong packets */
                msg_send(&msg, thread_getpid());
                break;
            }
        } while (data.num_recv < data.count);
finish:
        xtimer_remove(&data.sched_timer);
        res = _finish(&data);
        gnrc_netreg_unregister(GNRC_NETTYPE_ICMPV6, &data.netreg);
        for (unsigned i = 0;
             i < cib_avail((cib_t *)&thread_get_active()->msg_queue);
             i++) {
            msg_t msg;

            /* remove all remaining messages (likely caused by duplicates) */
            if ((msg_try_receive(&msg) > 0) &&
                (msg.type == GNRC_NETAPI_MSG_TYPE_RCV) &&
                (((gnrc_pktsnip_t *)msg.content.ptr)->type == GNRC_NETTYPE_ICMPV6)) {
                gnrc_pktbuf_release(msg.content.ptr);
            }
            else {
                /* requeue other packets */
                msg_send(&msg, thread_getpid());
            }
        }
    }
    return res;
}

/* get the next netif, returns true if there are more */
static bool _netif_get(gnrc_netif_t **current_netif)
{
    gnrc_netif_t *netif = *current_netif;

    netif = gnrc_netif_iter(netif);

    *current_netif = netif;
    return !gnrc_netif_highlander() && gnrc_netif_iter(netif);
}

static int _configure(ipv6_addr_t *neigh, _ping_data_t *data)
{
    int res = 1;
    char addr_str[IPV6_ADDR_MAX_STR_LEN];

    ipv6_addr_to_str(addr_str, neigh, sizeof(addr_str));
    data->hostname = strndup(addr_str, IPV6_ADDR_MAX_STR_LEN);
    char *iface = ipv6_addr_split_iface(data->hostname);

    if (iface) {
        data->netif = gnrc_netif_get_by_pid(atoi(iface));
        res = 0;
    }
    /* preliminary select the first interface */
    else if (_netif_get(&data->netif)) {
        /* don't take it if there is more than one interface */
        data->netif = NULL;
        res = 1;
    }
    if (ipv6_addr_from_str(&data->host, data->hostname) == NULL) {
        res = 1;
    }
    else {
        res = 0;
    }
    data->id ^= (xtimer_now_usec() & UINT16_MAX);
    return res;
}

static void _pinger(_ping_data_t *data)
{
    gnrc_pktsnip_t *pkt, *tmp;
    ipv6_hdr_t *ipv6;
    uint32_t timer;
    uint8_t *databuf;

    /* schedule next event (next ping or finish) ASAP */
    if ((data->num_sent + 1) < data->count) {
        /* didn't send all pings yet - schedule next in data->interval */
        data->sched_msg.type = _SEND_NEXT_PING;
        timer = data->interval;
    }
    else {
        /* Wait for the last ping to come back.
         * data->timeout: wait for a response in milliseconds.
         * Affects only timeout in absence of any responses,
         * otherwise ping waits for two max RTTs. */
        data->sched_msg.type = _PING_FINISH;
        timer = data->timeout;
        if (data->num_recv) {
            /* approx. 2*tmax, in seconds (2 RTT) */
            timer = (data->tmax / (512UL * 1024UL)) * US_PER_SEC;
            if (timer == 0) {
                timer = 1U * US_PER_SEC;
            }
        }
    }
    xtimer_set_msg(&data->sched_timer, timer, &data->sched_msg,
                   thread_getpid());
    bf_unset(data->cktab, (size_t)data->num_sent % CKTAB_SIZE);
    pkt = gnrc_icmpv6_echo_build(ICMPV6_ECHO_REQ, data->id,
                                 (uint16_t)data->num_sent++,
                                 NULL, data->datalen);
    if (pkt == NULL) {
        DEBUG("error: packet buffer full\n");
        return;
    }
    databuf = (uint8_t *)(pkt->data) + sizeof(icmpv6_echo_t);
    memset(databuf, data->pattern, data->datalen);
    tmp = gnrc_ipv6_hdr_build(pkt, NULL, &data->host);
    if (tmp == NULL) {
        DEBUG("error: packet buffer full\n");
        goto error_exit;
    }
    pkt = tmp;
    ipv6 = pkt->data;
    /* if data->hoplimit is unset (i.e. 0) gnrc_ipv6 will select hop limit */
    ipv6->hl = data->hoplimit;
    if (data->netif != NULL) {
        tmp = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
        if (tmp == NULL) {
            DEBUG("error: packet buffer full\n");
            goto error_exit;
        }
        gnrc_netif_hdr_set_netif(tmp->data, data->netif);
        LL_PREPEND(pkt, tmp);
    }
    if (data->datalen >= sizeof(uint32_t)) {
        *((uint32_t *)databuf) = xtimer_now_usec();
    }
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_IPV6,
                                   GNRC_NETREG_DEMUX_CTX_ALL,
                                   pkt)) {
        DEBUG("error: unable to send ICMPv6 echo request\n");
        goto error_exit;
    }
    return;
error_exit:
    gnrc_pktbuf_release(pkt);
}

static void _print_reply(_ping_data_t *data, gnrc_pktsnip_t *icmpv6,
                         ipv6_addr_t *from, unsigned hoplimit,
                         gnrc_netif_hdr_t *netif_hdr)
{
    icmpv6_echo_t *icmpv6_hdr = icmpv6->data;
    kernel_pid_t if_pid = netif_hdr ? netif_hdr->if_pid : KERNEL_PID_UNDEF;
    int16_t rssi = netif_hdr ? netif_hdr->rssi : 0;

    /* discard if too short*/
    if (icmpv6->size < (data->datalen + sizeof(icmpv6_echo_t))) {
        return;
    }
    if (icmpv6_hdr->type == ICMPV6_ECHO_REP) {
        char from_str[IPV6_ADDR_MAX_STR_LEN];
        const char *dupmsg = " (DUP!)";
        uint32_t triptime = 0;
        uint16_t recv_seq;
        /* not our ping*/
        if (byteorder_ntohs(icmpv6_hdr->id) != data->id) {
            return;
        }
        if (!ipv6_addr_is_multicast(&data->host) &&
            !ipv6_addr_equal(from, &data->host)) {
            return;
        }
        recv_seq = byteorder_ntohs(icmpv6_hdr->seq);
        ipv6_addr_to_str(&from_str[0], from, sizeof(from_str));
        if (data->datalen >= sizeof(uint32_t)) {
            triptime = xtimer_now_usec() - *((uint32_t *)(icmpv6_hdr + 1));
            data->tsum += triptime;
            if (triptime < data->tmin) {
                data->tmin = triptime;
            }
            if (triptime > data->tmax) {
                data->tmax = triptime;
            }
        }
        if (bf_isset(data->cktab, recv_seq % CKTAB_SIZE)) {
            data->num_rept++;
        }
        else {
            bf_set(data->cktab, recv_seq % CKTAB_SIZE);
            data->num_recv++;
            dupmsg += 7;
        }
        if (gnrc_netif_highlander() || (if_pid == KERNEL_PID_UNDEF) ||
            !ipv6_addr_is_link_local(from)) {
            DEBUG("%u bytes from %s: icmp_seq=%u ttl=%u",
                  (unsigned)icmpv6->size,
                  from_str, recv_seq, hoplimit);
        }
        else {
            DEBUG("%u bytes from %s%%%u: icmp_seq=%u ttl=%u",
                  (unsigned)icmpv6->size,
                  from_str, if_pid, recv_seq, hoplimit);
        }
        if (rssi) {
            DEBUG(" rssi=%" PRId16 " dBm", rssi);
        }
        if (data->datalen >= sizeof(uint32_t)) {
            DEBUG(" time=%lu.%03lu ms", (long unsigned)triptime / 1000,
                  (long unsigned)triptime % 1000);
        }
        DEBUG("%s\n", dupmsg);
    }
}

static void _handle_reply(_ping_data_t *data, gnrc_pktsnip_t *pkt)
{
    gnrc_pktsnip_t *ipv6, *icmpv6, *netif;
    gnrc_netif_hdr_t *netif_hdr;
    ipv6_hdr_t *ipv6_hdr;

    netif = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_NETIF);
    ipv6 = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_IPV6);
    icmpv6 = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_ICMPV6);
    if ((ipv6 == NULL) || (icmpv6 == NULL)) {
        DEBUG("No IPv6 or ICMPv6 header found in reply\n");
        return;
    }
    ipv6_hdr = ipv6->data;
    netif_hdr = netif ? netif->data : NULL;
    _print_reply(data, icmpv6, &ipv6_hdr->src, ipv6_hdr->hl, netif_hdr);
}

static int _finish(_ping_data_t *data)
{
    struct neighbor_entryl *node = malloc(sizeof(struct neighbor_entryl));

    unsigned long tmp, nrecv, ndup;

    tmp = data->num_sent;
    nrecv = data->num_recv;
    ndup = data->num_rept;
    DEBUG("\n--- %s statistics ---\n"
          "%lu packets transmitted, "
          "%lu packets received, ",
          data->hostname, tmp, nrecv);
    node->addr = data->host;
    node->throughput = 0;
    node->latency = 0;
    node->packet_loss = 0;
    node->next = NULL;
    if (ndup) {
        DEBUG("%lu duplicates, ", ndup);
    }
    if (tmp > 0) {
        tmp = ((tmp - nrecv) * 100) / tmp;
        node->packet_loss = tmp;
    }
    if (data->tmin != UINT_MAX) {
        unsigned tavg = data->tsum / (nrecv + ndup);
        DEBUG("round-trip min/avg/max = %u.%03u/%u.%03u/%u.%03u ms\n",
              data->tmin / 1000, data->tmin % 1000,
              tavg / 1000, tavg % 1000,
              data->tmax / 1000, data->tmax % 1000);

        DEBUG("latency min/avg/max = %u.%03u/%u.%03u/%u.%03u ms\n",
              data->tmin / 2000, ((data->tmin) / 2) % 1000,
              tavg / 2000, (tavg / 2) % 1000,
              data->tmax / 2000, ((data->tmax) / 2) % 1000);
        node->latency = tavg;
    }
    DEBUG("%s/ \n\tlatency :%u.%03u ms\n\tpacket_loss:%lu%%\n", data->hostname,
           (uint16_t)(node->latency / 2000), (uint16_t)(node->latency / 2) % 1000, tmp);
    if (!linked_list_node_exists(&node->addr)) {
        linked_list_update_latency(node);
    }
    else {
        linked_list_insert_node(node);
    }
    xtimer_usleep(1000);
    /* if condition is true, exit with 1 -- 'failure' */
    return (nrecv == 0);
}

#ifdef CONFIG_DCA_SHELL

int _latency(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return db_measure_network_latency();
}

XFA_USE_CONST(shell_command_t *, shell_commands_xfa);

shell_command_t _latency_cmd = { "dcalat", "Run DCA latency measurements", _latency };

XFA_ADD_PTR(
    shell_commands_xfa,
    0,
    sc_dcalat,
    &_latency_cmd
    );

#endif /* defined(CONFIG_DCA_SHELL) */
