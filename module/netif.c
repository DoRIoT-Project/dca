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
    COUNT
} device_property_t;

static const char *field_names[COUNT] = {
    [DEVICE_NAME] = "pid",
    [ADDRESS] = "inet6 addr",
    [LINK_TYPE] = "Link type"};
#define FIELD_NAME_UNKNOWN "unknown"

typedef struct
{
    /* The iface that the node represents */
    netif_t *iface;
    /* 1: firstlevel branch root, 0: leaf node */
    uint8_t is_root;
} _db_netif_node_private_data_t;

int8_t _netif_field_count = 0;
char *_netif_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _netif_node_getnext_child(db_node_t *node, db_node_t *next_child);
int _netif_node_getnext(db_node_t *node, db_node_t *next);
db_node_type_t _netif_node_gettype(const db_node_t *node);
size_t _netif_node_getsize(const db_node_t *node);
size_t _netif_node_getstr_value(const db_node_t *node, char *value, size_t bufsize);
int32_t _netif_node_getint_value(const db_node_t *node);
char *_netif_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX]);

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
    /* may be NULL for root */
    netif_t *iface = netif_iter(NULL);
    _netif_node_init(node, iface, 2u);
}

char *_netif_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        strncpy(name, "netif", DB_NODE_NAME_MAX);
    }
    else if (private_data->is_root == 1u)
    {
        assert(private_data->iface != NULL);
        netif_get_name (private_data->iface,name);
        //fmt_u32_dec(name, private_data->iface->pid);
    }
    else if (private_data->is_root == 0u && _netif_field_count < COUNT)
    {
        strncpy(name, _netif_node_get_field_name(node, name), DB_NODE_NAME_MAX);
        DEBUG("field name:%s\n", name);
    }
    return name;
}

int _netif_node_getnext_child(db_node_t *node, db_node_t *next_child)
{
    assert(node);
    assert(next_child);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        /* return child node representing iface, advance own iface */
        if (private_data->iface != NULL)
        {
            _netif_node_init(next_child, private_data->iface, 1u);
            private_data->iface = netif_iter(private_data->iface);
        }
        else
        {
            /* end of iface list */
            db_node_set_null(next_child);
        }
    }
    else if (private_data->is_root == 1u)
    {
        /* return child node representing next pid, advance own pid */
        if (private_data->iface != NULL && _netif_field_count < COUNT)
        {
            //DEBUG("current pid :%d\n", private_data->iface->pid);
            _netif_node_init(next_child, private_data->iface, 0u);
            private_data->iface = netif_iter(private_data->iface);
            private_data->is_root = 0u;
        }
    }

    else
    {
        if (_netif_field_count == COUNT - 1)
        {
            /* end of processes list */
            _netif_field_count = 0;
            db_node_set_null(next_child);
        }
        else
        {
            _netif_field_count++;
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
    if (private_data->is_root == 2u)
    {
        /* this branch root node has no neighbor */
        db_node_set_null(next);
    }
    else if (private_data->is_root == 1u)
    {
        /* this branch root node has no neighbor */
        netif_t *iface = netif_iter(private_data->iface);
        if (private_data->iface != NULL)
        {
            _netif_node_init(next, iface, 0u);
        }
        else
        {
            /* end of processes list */
            db_node_set_null(next);
        }
    }
    else
    {
        /* retrieve the next child (leaf) node */
        netif_t *iface = netif_iter(private_data->iface);
        if (private_data->iface != NULL)
        {
            _netif_node_init(next, iface, 0u);
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
    if (private_data->is_root == 2u)
    {
        return db_node_type_inner;
    }
    else if (private_data->is_root == 1u)
    {
        return db_node_type_inner;
    }
    else if (_netif_field_count == 0) /* for pid */
    {
        return db_node_type_int;
    }
    else if (_netif_field_count == 1) /* for address */
    {
        return db_node_type_str;
    }
    else /*for  LINK TYPE*/
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
    //enable = NETOPT_DISABLE;
    int is_wired = netif_get_opt(iface, NETOPT_IS_WIRED, 0, &enable, sizeof(enable));
    if (is_wired ==1)
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

size_t _netif_node_getsize(const db_node_t *node)
{
    assert(node);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        return 0u;
    }
    else if (private_data->is_root == 1u)
    {
        return 0u;
    }
    else if (private_data->is_root == 0u && _netif_field_count == 2)
    {
        return sizeof(int32_t);
    }
    else
    {
        char addr[IPV6_ADDR_MAX_STR_LEN];
        return _netif_get_ipv6_addr(private_data->iface, addr);
    }
    return 0u;
}

size_t _netif_node_getstr_value(const db_node_t *node, char *value, size_t bufsize)
{
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t *)node->private_data.u8;
    assert(private_data->iface != NULL);
    (void)bufsize;
    if (_netif_field_count == 1)
    {
        return (int32_t)_netif_get_ipv6_addr(private_data->iface, value);
    }
    else if (_netif_field_count == 2)
    {

        return (int32_t)_netif_get_link_type(private_data->iface, value);
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
    else
    {
        return -1;
    }
}
