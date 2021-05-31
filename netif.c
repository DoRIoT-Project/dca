/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */

#include <doriot_dca/netif.h>
#include <stddef.h>
#include <stdint.h>
#include <net/gnrc/netif.h>
#include <string.h>
#include <fmt.h>
#include "net/gnrc/ipv6.h"
#include "net/gnrc/ipv6/nib/nc.h"
#define ENABLE_DEBUG (0)
#include "debug.h"

#if POSIX_C_SOURCE < 200809L
#define strnlen(a, b) strlen(a)
#endif

typedef enum
{
    DEVICE_NAME,
    ADDRESS,
    LINK_TYPE,
    NUM_NEIGH,
    NEIGH,
    COUNT
} device_property_t;

static const char *field_names[COUNT] = {
    [DEVICE_NAME] = "pid",
    [ADDRESS] = "inet6 addr",
    [LINK_TYPE] = "Link type",
    [NUM_NEIGH] = "num_neighbours",
    [NEIGH] = "neighbours"};

typedef enum
{
    NEIGH_ADDR,
    LATENCY,
    PACKET_LOSS,
    THROUGHPUT,
    QOS_COUNT
} qos_property_t;

static const char *sub_field_names[QOS_COUNT] = {
    [NEIGH_ADDR] = "ip",
    [LATENCY] = "latency",
    [PACKET_LOSS] = "packet_loss",
    [THROUGHPUT] = "throughput"};
#define FIELD_NAME_UNKNOWN "unknown"

typedef struct
{
    /* The iface that the node represents */
    netif_t *iface;
    /* 1: firstlevel branch root, 0: leaf node */
    uint8_t is_root;
} _db_netif_node_private_data_t;

uint8_t _netif_field_count = 0;
uint8_t _netif_sub_field_count = 1;
uint8_t _num_neighbours = 0;
char *_netif_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _netif_node_getnext_child(db_node_t *node, db_node_t *next_child);
int _netif_node_getnext(db_node_t *node, db_node_t *next);
db_node_type_t _netif_node_gettype(const db_node_t *node);
size_t _netif_node_getsize(const db_node_t *node);
size_t _netif_node_getstr_value(const db_node_t *node, char *value, size_t bufsize);
int32_t _netif_node_getint_value(const db_node_t *node);
char *_netif_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
char *_netif_node_get_sub_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int32_t _netif_get_latency(uint8_t _num_neighbours, uint8_t _netif_sub_field_count, char *value);
int32_t _netif_get_throughput(uint8_t _num_neighbours, uint8_t _netif_sub_field_count, char *value);
int32_t _netif_get_packetloss(uint8_t _num_neighbours, uint8_t _netif_sub_field_count, char *value);
int _netif_get_ip(uint8_t _num_neighbours, char *addr_str);
int _netif_node_add_list(ipv6_addr_t *ip_addr);

static db_node_ops_t _db_netif_node_ops = {
    .get_name_fn = _netif_node_getname,
    .get_next_child_fn = _netif_node_getnext_child,
    .get_next_fn = _netif_node_getnext,
    .get_type_fn = _netif_node_gettype,
    .get_size_fn = _netif_node_getsize,
    .get_int_value_fn = _netif_node_getint_value,
    .get_float_value_fn = NULL,
    .get_str_value_fn = _netif_node_getstr_value};

/* netif node constructor */
void _netif_node_init(db_node_t *node, netif_t *iface, uint8_t is_root)
{
    node->ops = &_db_netif_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    private_data->iface = iface;
    private_data->is_root = is_root;
}

void db_new_netif_node(db_node_t *node)
{
    assert(node);
    assert(sizeof(_db_netif_node_private_data_t) <= DB_NODE_PRIVATE_DATA_MAX);
    _netif_field_count = 0;
    _netif_sub_field_count = 1;
    netif_t *iface = netif_iter(NULL);
    _netif_node_init(node, iface, 4u);
}

char *_netif_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 4u)
    {
        strncpy(name, "netif", DB_NODE_NAME_MAX);
    }
    else if (private_data->is_root == 3u)
    {
        assert(private_data->iface != NULL);
        netif_get_name(private_data->iface, name);
    }
    else if (private_data->is_root == 2u && _netif_field_count < COUNT)
    {
        strncpy(name, _netif_node_get_field_name(node, name), DB_NODE_NAME_MAX);
    }
    else if (private_data->is_root == 1u)
    {
        _netif_get_ip(_num_neighbours, name);
        DEBUG("_num_neighbours:%d,%s\n", _num_neighbours, name);
    }
    else if (private_data->is_root == 0u && _netif_sub_field_count <= QOS_COUNT)
    {
        strncpy(name, _netif_node_get_sub_field_name(node, name), DB_NODE_NAME_MAX);
    }
    return name;
}

int _netif_node_getnext_child(db_node_t *node, db_node_t *next_child)
{
    assert(node);
    assert(next_child);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 4u)
    {
        /* return child node representing iface, advance own iface */
        if (private_data->iface != NULL)
        {
            _netif_node_init(next_child, private_data->iface, 3u);
            private_data->iface = netif_iter(private_data->iface);
        }
        else
        {
            /* end of iface list */
            db_node_set_null(next_child);
        }
    }
    else if (private_data->is_root == 3u)
    {
        if (private_data->iface != NULL && _netif_field_count < COUNT)
        {
            _netif_node_init(next_child, private_data->iface, 2u);
            private_data->iface = netif_iter(private_data->iface);
            private_data->is_root = 2u;
        }
    }
    else if (private_data->is_root == 2u)
    {
        if (_netif_field_count == COUNT - 1 && private_data->iface != NULL && _num_neighbours > 0)
        {
            _netif_node_init(next_child, private_data->iface, 1u);
            private_data->is_root = 1u;
            _netif_sub_field_count++;
        }
        else if (_netif_field_count < COUNT - 1)
        {
            _netif_field_count++;
        }
        else
        {
            /* end of iface list */
            db_node_set_null(next_child);
        }
    }
    else if (private_data->is_root == 1u)
    {
        if (_num_neighbours <= 0)
        {
            /*reset _netif_sub_field_count*/
            _netif_sub_field_count = 1;
            db_node_set_null(next_child);
        }
        else if (_netif_sub_field_count == 1)
        {
            /*iterating sub field*/
            _netif_node_init(next_child, private_data->iface, 1u);
            private_data->is_root = 1u;
            _netif_sub_field_count++;
        }
        else if (_netif_sub_field_count < QOS_COUNT)
        {
            /*init leaf nodes for neighbor information*/
            _netif_node_init(next_child, private_data->iface, 0u);
            private_data->is_root = 0u;
        }
        else
        {
            _num_neighbours--;            
            if (_num_neighbours == 0) 
            {
                /* end of neighbor information*/
                _netif_sub_field_count = 1;
                db_node_set_null(next_child);
            }
            else
            {
                /* recursive call to access next neighbor ip*/
                _netif_sub_field_count = 1;
                private_data->is_root = 1u;
                _netif_node_getnext_child(node, next_child);
            }
        }
    }
    else if (private_data->is_root == 0u)
    {        
        if (_netif_sub_field_count < QOS_COUNT)
        {
            /*iterate leaf nodes for neighbor information*/
            _netif_sub_field_count++;
            private_data->is_root = 0u;
        }
        else if (_netif_sub_field_count == QOS_COUNT && _num_neighbours != 0)
        {
            /* end of neighbor information for current node*/
            _netif_sub_field_count++;
            db_node_set_null(next_child);
        }
        else
        {
            _num_neighbours--;
            if (_num_neighbours == 0) 
            {
                /* end of neighbor information*/
                _netif_sub_field_count = 1;
                db_node_set_null(next_child);
            }
            else
            {
                /* recursive call to access next neighbor ip*/
                _netif_sub_field_count = 1;
                private_data->is_root = 1u;
                _netif_node_getnext_child(node, next_child);
            }
        }
    }
    return 0;
}

int _netif_node_getnext(db_node_t *node, db_node_t *next)
{
    assert(node);
    assert(next);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 4u)
    {
        /* this branch root node has no neighbor */
        db_node_set_null(next);
    }
    else if (private_data->is_root == 3u)
    {
        /* this branch root node has no neighbor */
        netif_t *iface = netif_iter(private_data->iface);
        if (private_data->iface != NULL)
        {
            _netif_node_init(next, iface, 2u);
        }
        else
        {
            /* end of processes list */
            db_node_set_null(next);
        }
    }
    else if (private_data->is_root == 1u)
    {
        /* this branch root node has no neighbor */
        netif_t *iface = netif_iter(private_data->iface);
        if (private_data->iface != NULL)
        {
            _netif_node_init(next, iface, 1u);
        }
    }
    else
    {
        /* retrieve the next child (leaf) node */
        netif_t *iface = netif_iter(private_data->iface);
        if (private_data->iface != NULL)
        {
            _netif_node_init(next, iface, 2u);
        }
        else
        {
            /* end of processes list */
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _netif_node_gettype(const db_node_t *node)
{
    assert(node);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 4u)
    {
        return db_node_type_inner;
    }
    else if (private_data->is_root == 3u)
    {
        return db_node_type_inner;
    }
    /* for pid  and number of neighbors*/
    else if (_netif_field_count == 0 || _netif_field_count == 3)
    {
        return db_node_type_int;
    }
    /* for own address */
    else if (private_data->is_root == 2u && _netif_field_count == 1)
    {
        return db_node_type_str;
    }
    /* for link type */
    else if (private_data->is_root == 2u && _netif_field_count == 2)
    {
        return db_node_type_str;
    }
    /* for inner node */
    else if (private_data->is_root == 2u && _netif_field_count == 4)
    {
        return db_node_type_inner;
    }
    /* for ip inner node*/
    else if (private_data->is_root == 1u)
    {
        return db_node_type_inner;
    }
    /* for latency, packet loss,throughput  */
    else if (_netif_sub_field_count == 2 || _netif_sub_field_count == 3 || _netif_sub_field_count == 4)
    {
        return db_node_type_str;
    }
    else
    {
        return db_node_type_str;
    }
}

int _netif_get_ipv6_addr(netif_t *iface, char addr[IPV6_ADDR_MAX_STR_LEN])
{
    ipv6_addr_t v6addr;
    int r = netif_get_opt(iface, NETOPT_IPV6_ADDR, 0, &v6addr, sizeof(ipv6_addr_t));
    if (r >= 0)
    {
        ipv6_addr_to_str(addr, &v6addr, IPV6_ADDR_MAX_STR_LEN);
        return strnlen(addr, IPV6_ADDR_MAX_STR_LEN);
    }
    else
    {
        memset(addr, 0, IPV6_ADDR_MAX_STR_LEN);
        return 0;
    }
}

int _netif_get_link_type(netif_t *iface, char link[10])
{
    netopt_enable_t enable;
    int is_wired = netif_get_opt(iface, NETOPT_IS_WIRED, 0, &enable, sizeof(enable));
    if (is_wired == 1)
    {
        strcpy(link, "wired");
        return strlen(link);
    }
    else
    {
        strcpy(link, "wireless");
        return strlen(link);
    }
}

int _netif_get_ip(uint8_t _num_neighbours, char *addr_str)
{
    linked_list_read_ip(_num_neighbours, addr_str);
    return strnlen(addr_str, IPV6_ADDR_MAX_STR_LEN);
}

int32_t _netif_get_latency(uint8_t _num_neighbours, uint8_t _netif_sub_field_count, char *value)
{
    int32_t lat = (linked_list_read(_netif_sub_field_count, _num_neighbours));
    sprintf(value, "%" PRIu32 ".%03" PRIu32 " ms", lat / 2000, (lat / 2) % 1000);
    return strlen(value);
}


int32_t _netif_get_packetloss(uint8_t _num_neighbours, uint8_t _netif_sub_field_count, char *value)
{
    int32_t val = (linked_list_read(_netif_sub_field_count, _num_neighbours));
    sprintf(value, "%"PRIu32" %%", val);
    return strlen(value);
}

int32_t _netif_get_throughput(uint8_t _num_neighbours, uint8_t _netif_sub_field_count, char *value)
{
    int32_t val = (linked_list_read(_netif_sub_field_count, _num_neighbours));
    sprintf(value, "%"PRIu32" bytes/sec", val);
    return strlen(value);
}

size_t _netif_node_getsize(const db_node_t *node)
{
    assert(node);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 4u)
    {
        return 2u;
    }
    else if (private_data->is_root == 3u)
    {
        return 2u;
    }
    /* for number of neighbors*/
    else if (private_data->is_root == 2u && _netif_field_count == 2)
    {
        return sizeof(int32_t);
    }
    /* for packet loss,throughput */
    else if (private_data->is_root == 0u && _netif_sub_field_count < QOS_COUNT)
    {
        return sizeof(int32_t);
    }
    else
    {
        char addr[IPV6_ADDR_MAX_STR_LEN];
        return _netif_get_ipv6_addr(private_data->iface, addr);
    }
    return 2u;
}

size_t _netif_node_getstr_value(const db_node_t *node, char *value, size_t bufsize)
{
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    assert(private_data->iface != NULL);
    (void)bufsize;
    /* for own ip*/
    if (_netif_field_count == 1)
    {
        return (int32_t)_netif_get_ipv6_addr(private_data->iface, value);
    }
    /* for link type*/
    else if (_netif_field_count == 2)
    {
        return (int32_t)_netif_get_link_type(private_data->iface, value);
    }
    /* for ip*/
    else if (_netif_sub_field_count == 1)
    {
        return (int32_t)_netif_get_ip(_num_neighbours, value);
    }
    /* for latency*/
    else if (_netif_sub_field_count == 2)
    {
        return (int32_t)_netif_get_latency(_num_neighbours, _netif_sub_field_count, value);
    }
    /* for packetloss*/
    else if (_netif_sub_field_count == 3)
    {
        return (int32_t)_netif_get_packetloss(_num_neighbours, _netif_sub_field_count, value);
    }
    /* for throughput*/
    else if (_netif_sub_field_count == 4)
    {
        return (int32_t)_netif_get_throughput(_num_neighbours, _netif_sub_field_count, value);
    }
    return 88;
}

char *_netif_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    assert(private_data->iface != NULL);
    switch (_netif_field_count)
    {
    case 0:
        strncpy(name, field_names[DEVICE_NAME], DB_NODE_NAME_MAX);
        break;
    case 1:
        strncpy(name, field_names[ADDRESS], DB_NODE_NAME_MAX);
        break;
    case 2:
        strncpy(name, field_names[LINK_TYPE], DB_NODE_NAME_MAX);
        break;
    case 3:
        strncpy(name, field_names[NUM_NEIGH], DB_NODE_NAME_MAX);
        break;
    case 4:
        strncpy(name, field_names[NEIGH], DB_NODE_NAME_MAX);
        break;
    default:
        break;
    }
    return name;
}

char *_netif_node_get_sub_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    DEBUG("--_netif_sub_field_count == %d--\n", _netif_sub_field_count);
    switch (_netif_sub_field_count)
    {
    case 1:
        strncpy(name, sub_field_names[NEIGH_ADDR], DB_NODE_NAME_MAX);
        break;
    case 2:
        strncpy(name, sub_field_names[LATENCY], DB_NODE_NAME_MAX);
        break;
    case 3:
        strncpy(name, sub_field_names[PACKET_LOSS], DB_NODE_NAME_MAX);
        break;
    case 4:
        strncpy(name, sub_field_names[THROUGHPUT], DB_NODE_NAME_MAX);
        break;
    default:
        break;
    }
    return name;
}

int32_t _netif_node_getint_value(const db_node_t *node)
{
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (_netif_field_count == 0)
    {
        return (int32_t)netif_get_id(private_data->iface);
    }
    else if (_netif_field_count == 3)
    {
        void *state = NULL;
        gnrc_ipv6_nib_nc_t nce;
        _num_neighbours = 0;
        while (gnrc_ipv6_nib_nc_iter((int32_t)netif_get_id(private_data->iface), &state, &nce))
        {
            _num_neighbours++;
            _netif_node_add_list(&nce.ipv6);
        }
        return _num_neighbours;
    }
    else
    {
        return -1;
    }
}

int _netif_node_add_list(ipv6_addr_t *ip_addr)
{
    if (linked_list_node_exists(ip_addr))
    {
        struct neighbor_entryl *node = malloc(sizeof(struct neighbor_entryl));
        node->addr = *ip_addr;
        node->latency = 0;
        node->packet_loss = 0;
        node->throughput = 0;
        node->next = NULL;
        linked_list_insert_node(node);
    }
    return 0;
}
