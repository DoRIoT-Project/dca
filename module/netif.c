/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */

#include <doriot_dca/netif.h>
#include <stddef.h>
#include <stdint.h>
#include <net/gnrc/netif.h>
#include <string.h>
#include <fmt.h>

#if POSIX_C_SOURCE < 200809L
    #define strnlen(a,b) strlen(a)
#endif

typedef struct {
    /* The iface that the node represents */
    gnrc_netif_t *iface;
    /* 1: firstlevel branch root, 0: leaf node */
    uint8_t is_root;
} _db_netif_node_private_data_t;

char* _netif_node_getname (const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _netif_node_getnext_child (db_node_t *node, db_node_t *next_child);
int _netif_node_getnext (db_node_t *node, db_node_t *next);
db_node_type_t _netif_node_gettype (const db_node_t *node);
size_t _netif_node_getsize (const db_node_t *node);
size_t _netif_node_getstr_value (const db_node_t *node, char *value, size_t bufsize);

static db_node_ops_t _db_netif_node_ops = {
    .get_name_fn = _netif_node_getname,
    .get_next_child_fn = _netif_node_getnext_child,
    .get_next_fn = _netif_node_getnext,
    .get_type_fn = _netif_node_gettype,
    .get_size_fn = _netif_node_getsize,
    .get_int_value_fn = NULL,
    .get_float_value_fn = NULL,
    .get_str_value_fn = _netif_node_getstr_value
};

/* ps node constructor */
void _netif_node_init(db_node_t *node, gnrc_netif_t* iface, uint8_t is_root) {
    node->ops = &_db_netif_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    private_data->iface = iface;
    private_data->is_root = is_root;
}

void db_new_netif_node(db_node_t *node) {
    assert(node);
    assert(sizeof(_db_netif_node_private_data_t) <= DB_NODE_PRIVATE_DATA_MAX);
    /* may be NULL for root */
    gnrc_netif_t *iface = gnrc_netif_iter(NULL);
    _netif_node_init(node, iface, 1u);
}

char* _netif_node_getname (const db_node_t *node, char name[DB_NODE_NAME_MAX]) {
    assert(node);
    assert(name);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        strncpy(name, "netif", DB_NODE_NAME_MAX);
    }
    else {
        assert(private_data->iface != NULL);
        fmt_u32_dec(name, private_data->iface->pid);
    }
    return name;
}

int _netif_node_getnext_child (db_node_t *node, db_node_t *next_child) {
    assert(node);
    assert(next_child);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        /* return child node representing iface, advance own iface */
        if(private_data->iface != NULL) {
            _netif_node_init(next_child, private_data->iface, 0u);
            private_data->iface = gnrc_netif_iter(private_data->iface);
        }
        else{
            /* end of iface list */
            db_node_set_null(next_child);
        }
    }
    else {
        /* leaf entries do not have children */
        db_node_set_null(next_child);
    }
    return 0;
}

int _netif_node_getnext (db_node_t *node, db_node_t *next) {
    assert(node);
    assert(next);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        /* this branch root node has no neighbor */
        db_node_set_null(next);
    }
    else {
        /* retrieve the next child (leaf) node */
        gnrc_netif_t *iface = gnrc_netif_iter(private_data->iface);
        if(private_data->iface != NULL) {
            _netif_node_init(next, iface, 0u);
        }
        else{
            /* end of processes list */
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _netif_node_gettype (const db_node_t *node) {
    assert(node);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return db_node_type_inner;
    }
    else {
        return db_node_type_str;
    }
}

int _netif_get_ipv6_addr(gnrc_netif_t *iface, char addr[IPV6_ADDR_MAX_STR_LEN]) {
    ipv6_addr_t v6addr;
    int r =
        gnrc_netapi_get(iface->pid, NETOPT_IPV6_ADDR, 0, &v6addr, sizeof(ipv6_addr_t));
    if(r >= 0) {
        ipv6_addr_to_str(addr, &v6addr, IPV6_ADDR_MAX_STR_LEN);
        return strnlen(addr, IPV6_ADDR_MAX_STR_LEN);
    }
    else {
        memset(addr, 0, IPV6_ADDR_MAX_STR_LEN);
        return 0;
    }
}

size_t _netif_node_getsize (const db_node_t *node) {
    assert(node);
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return 0u;
    }
    else {
        char addr[IPV6_ADDR_MAX_STR_LEN];
        return _netif_get_ipv6_addr(private_data->iface, addr);
    }
    return 0u;
}

size_t _netif_node_getstr_value (const db_node_t *node, char *value, size_t bufsize) {
    _db_netif_node_private_data_t *private_data =
        (_db_netif_node_private_data_t*) node->private_data.u8;
    assert(private_data->iface != NULL);
    assert(bufsize >= IPV6_ADDR_MAX_STR_LEN);
    return (int32_t) _netif_get_ipv6_addr(private_data->iface, value);
}
