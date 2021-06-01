/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */

#include "doriot_dca/linked_list.h"
#include "doriot_dca/udp_throughput.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "net/sock/udp.h"
#include "net/ipv6/addr.h"
#include "thread.h"
#include "xtimer.h"
#include "xfa.h"
#include "shell.h"
#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ipv6/nib/nc.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define SERVER_MSG_QUEUE_SIZE (8)
#define SERVER_BUFFER_SIZE (64)
#define START_TEST 0
#define DATA_PACKET 1
#define RESULT 2
#define TEST_ACK 3
#define SUCCESS 4
#define PACKET_TIMEOUT 1000000
#define THROUGHPUT_TIMEOUT 3000000
#define UDP_PACKET_COUNT 3
#define UDP_PACKET_SIZE 128
#define UDP_SERVER_PORT 1883

static bool server_running = false;
static sock_udp_t sock;
static sock_udp_t sock_thread;
static char server_stack[THREAD_STACKSIZE_DEFAULT];
static msg_t server_msg_queue[SERVER_MSG_QUEUE_SIZE];
static uint32_t start_timer = 0;
static uint32_t end_timer = 0;

typedef struct {
    uint8_t id;
    uint8_t packet_count;
    uint8_t packet_size;
    uint32_t throughput;
} _udp_data;

void *_udp_server_thread(void *args)
{
    (void)args;
    sock_udp_ep_t server = { .port = CONFIG_DCA_UDP_SERVER_PORT, .family = AF_INET6 };

    msg_init_queue(server_msg_queue, SERVER_MSG_QUEUE_SIZE);
    _udp_data *udp_packet = malloc(sizeof(_udp_data));

    if (sock_udp_create(&sock_thread, &server, NULL, 0) < 0) {
        puts("Error creating socket");
        return NULL;
    }
    printf("Success: started UDP server on port %u\n", server.port);
    server_running = true;
    sock_udp_ep_t remote = { .family = AF_INET6 };

    while (1) {
        int res;
        if ((res = sock_udp_recv(&sock_thread, udp_packet,
                                 sizeof(_udp_data), SOCK_NO_TIMEOUT,
                                 &remote)) < 0) {
            puts("Error while receiving1");
            continue;
        }
        else if (res == 0) {
            puts("No data received");
            continue;
        }
        else if (res > 0) {
            if (udp_packet->id == START_TEST) {
                DEBUG("packet count:%d\n", udp_packet->packet_count);
                DEBUG("packet size:%d\n", udp_packet->packet_size);
                DEBUG("Client Connected");
                char buf[udp_packet->packet_size];
                udp_packet->id = TEST_ACK;
                sock_udp_send(&sock_thread, udp_packet,
                              sizeof(_udp_data),
                              &remote);
                int i = 0;
                for (i = 0; i < udp_packet->packet_count; i++) {
                    if ((res = sock_udp_recv(&sock_thread, buf,
                                             sizeof(buf), PACKET_TIMEOUT,
                                             &remote)) < 0) {
                        printf("Error while receiving: %d\n", res);
                        break;
                    }
                    else {
                        if (i == 0) {
                            start_timer = xtimer_now_usec();
                        }
                    }
                }
                end_timer = xtimer_now_usec() - 100;
                if (i < udp_packet->packet_count) {
                    end_timer -= PACKET_TIMEOUT;
                }
                udp_packet->throughput = (udp_packet->packet_size * i * US_PER_SEC) /
                                         (end_timer - start_timer);
                DEBUG("Receieved %d packets\n", i);
                DEBUG("total bytes received:%d\n", (udp_packet->packet_size * i));
                DEBUG("time diff: %" PRIu32 " uS\n", (end_timer - start_timer));
                DEBUG("throughput: %" PRIu32 " bytes/sec\n", udp_packet->throughput);
                xtimer_usleep(100000);
                udp_packet->id = SUCCESS;
                sock_udp_send(&sock_thread, udp_packet,
                              sizeof(_udp_data),
                              &remote);
            }
            else {
                DEBUG("ID error: %d", udp_packet->id);
                return NULL;
            }
        }
        else {
            DEBUG("Received %d", res);
            DEBUG("id :%d\n", udp_packet->id);
        }
    }
}

int db_measure_network_throughput(void)
{
    int res;
    int i = 0;
    sock_udp_ep_t remote = { .family = AF_INET6 };
    char addr_str[IPV6_ADDR_MAX_STR_LEN];
    unsigned iface = 0;
    void *state = NULL;
    gnrc_ipv6_nib_nc_t nce;

    while (gnrc_ipv6_nib_nc_iter(iface, &state, &nce)) {
        struct neighbor_entryl *node = malloc(sizeof(struct neighbor_entryl));
        node->addr = nce.ipv6;
        node->latency = 0;
        node->packet_loss = 0;
        node->throughput = 0;
        node->next = NULL;
        ipv6_addr_to_str(addr_str, &(nce.ipv6), sizeof(addr_str));
        if (ipv6_addr_from_str((ipv6_addr_t *)&remote.addr, addr_str) == NULL) {
            puts("Error: unable to parse destination address");
            return 1;
        }
        if (ipv6_addr_is_link_local((ipv6_addr_t *)&remote.addr)) {
            /* choose first interface when address is link local */
            gnrc_netif_t *netif = gnrc_netif_iter(NULL);
            remote.netif = (uint16_t)netif->pid;
        }
        remote.port = UDP_SERVER_PORT;
        sock_udp_ep_t client = { .port = 1884, .family = AF_INET6 };
        if (sock_udp_create(&sock, &client, &remote, 0) < 0) {
            puts("Error creating socket");
            return 1;
        }
        _udp_data *udp_packet = malloc(sizeof(_udp_data));
        udp_packet->id = START_TEST;
        udp_packet->packet_count = UDP_PACKET_COUNT;
        udp_packet->packet_size = UDP_PACKET_SIZE;
        udp_packet->throughput = 0;
        char payload[UDP_PACKET_SIZE];
        if ((res = sock_udp_send(&sock, udp_packet, sizeof(udp_packet), &remote)) < 0) {
            puts("could not send start_test packet");
            goto finish;
        }
        if ((res = sock_udp_recv(&sock, udp_packet,
                                 sizeof(_udp_data), PACKET_TIMEOUT,
                                 &remote)) < 0) {
            puts("Error while receiving test ack");
            goto finish;
        }
        if (udp_packet->id == TEST_ACK) {
            for (i = 0; i < udp_packet->packet_count; i++) {
                if ((res = sock_udp_send(&sock, payload, sizeof(payload), &remote)) < 0) {
                    puts("could not send udp payloads");
                    goto finish;
                }
                if (i == 0) {
                    xtimer_usleep(100);
                }
            }
            if ((res = sock_udp_recv(&sock, udp_packet,
                                     sizeof(_udp_data), THROUGHPUT_TIMEOUT,
                                     &remote)) < 0) {
                puts("error receiving result");
                goto finish;
            }
            if (udp_packet->id == SUCCESS) {
                printf("%s/ \n\tthroughput :%" PRIu32 " bytes/sec\n", addr_str,
                       udp_packet->throughput);
                node->throughput = udp_packet->throughput;
            }
        }
finish:
        DEBUG("Done throughput calculation :)\n");
        if (!linked_list_node_exists(&node->addr)) {
            linked_list_update_throughput(node);
        }
        else {
            linked_list_insert_node(node);
        }
        free(udp_packet);
        sock_udp_close(&sock);
        xtimer_usleep(US_PER_SEC);
    }
    return 0;
}

int db_start_udp_server(void)
{
    if ((server_running == false) &&
        thread_create(server_stack, sizeof(server_stack), THREAD_PRIORITY_MAIN - 2,
                      THREAD_CREATE_STACKTEST, _udp_server_thread, NULL,
                      "dca_udp_server") <= KERNEL_PID_UNDEF) {
        return -1;
    }
    return 0;
}

#ifdef CONFIG_DCA_SHELL

int _throughput(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return db_measure_network_throughput();
}

XFA_USE_CONST(shell_command_t *, shell_commands_xfa);

shell_command_t _throughput_cmd = { "dcatp", "Run DCA throughput measurements", _throughput };

XFA_ADD_PTR(
    shell_commands_xfa,
    0,
    sc_dcatp,
    &_throughput_cmd
    );

#endif /* defined(CONFIG_DCA_SHELL) */
