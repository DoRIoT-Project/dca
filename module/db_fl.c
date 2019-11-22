/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */

 /* TODO: this representation is also kind of the static entry representation,
    which it shouldn't be. The dynamic entries are missing completely. */

#include <doriot_dca/db.h>
#include <doriot_dca/db_fl.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    /* if root node: index for enumerating the leaf nodes */
    /* if not root node: index of current node */
    uint8_t sub_idx;
    /* 1: root node, 0: leaf node */
    uint8_t is_root;
    /* element in db_index[] */
    uint8_t fl_idx;
} _db_fl_node_private_data_t;

char* _fl_node_getname (const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _fl_node_getnext_child (db_node_t *node, db_node_t *next_child);
int _fl_node_getnext (db_node_t *node, db_node_t *next);
db_node_type_t _fl_node_gettype (const db_node_t *node);
size_t _fl_node_getsize (const db_node_t *node);
int32_t _fl_node_getint_value (const db_node_t *node);
float _fl_node_getfloat_value (const db_node_t *node);
size_t _fl_node_getstr_value (const db_node_t *node, char *value, size_t bufsize);

static db_node_ops_t _db_fl_node_ops = {
    .get_name_fn = _fl_node_getname,
    .get_next_child_fn = _fl_node_getnext_child,
    .get_next_fn = _fl_node_getnext,
    .get_type_fn = _fl_node_gettype,
    .get_size_fn = _fl_node_getsize,
    .get_int_value_fn = _fl_node_getint_value,
    .get_float_value_fn = _fl_node_getfloat_value,
    .get_str_value_fn = _fl_node_getstr_value
};

/* fl node constructor */
void _fl_node_init(db_node_t *node, uint8_t fl_idx, uint8_t sub_idx, uint8_t is_root) {
    node->ops = &_db_fl_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    private_data->fl_idx = fl_idx;
    private_data->sub_idx = sub_idx;
    private_data->is_root = is_root;
}

void db_new_fl_node(db_node_t *node, uint8_t fl_idx) {
    assert(node);
    _fl_node_init(node, fl_idx, 0u, 1u);
}

char* _fl_node_getname (const db_node_t *node, char name[DB_NODE_NAME_MAX]) {
    assert(node);
    assert(name);
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    assert(private_data->fl_idx < db_get_num_fl_nodes());
    db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
    if(private_data->is_root) {
        strncpy(name, fl_ent->branch_name, DB_NODE_NAME_MAX);
    }
    else {
        assert(private_data->sub_idx < fl_ent->num_static_entries);
        strncpy(name, fl_ent->static_entries[private_data->sub_idx].name, DB_NODE_NAME_MAX);
    }
    return name;
}

int _fl_node_getnext_child (db_node_t *node, db_node_t *next_child) {
    assert(node);
    assert(next_child);
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    assert(private_data->fl_idx < db_get_num_fl_nodes());
    db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
    if(private_data->is_root) {
        /* return child node at sub_idx, advance sub_idx */
        if(private_data->sub_idx < fl_ent->num_static_entries) {
            _fl_node_init(next_child, private_data->fl_idx,
                private_data->sub_idx, 0u);
            private_data->sub_idx += 1;
        }
        else{
            /* end of subnode list */
            db_node_set_null(next_child);
        }
    }
    else {
        /* static db entries do not have children */
        db_node_set_null(next_child);
    }
    return 0;
}

int _fl_node_getnext (db_node_t *node, db_node_t *next) {
    assert(node);
    assert(next);
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    assert(private_data->fl_idx < db_get_num_fl_nodes());
    db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
    if(private_data->is_root) {
        /* retrieve the next firstlevel branch from db_index */
        if(private_data->fl_idx+1u < db_get_num_fl_nodes()) {
            db_new_fl_node(next, private_data->fl_idx+1);
        }
        else {
            db_node_set_null(next);
        }
    }
    else {
        /* retrieve the next child (leaf) node */
        assert(private_data->sub_idx < fl_ent->num_static_entries);
        uint8_t next_idx = private_data->sub_idx+1;
        if(next_idx < fl_ent->num_static_entries) {
            _fl_node_init(next, private_data->fl_idx, next_idx, 0u);
        }
        else{
            /* end of node list */
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _fl_node_gettype (const db_node_t *node) {
    assert(node);
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return db_node_type_inner;
    }
    else {
        assert(private_data->fl_idx < db_get_num_fl_nodes());
        db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
        assert(private_data->sub_idx < fl_ent->num_static_entries);
        return fl_ent->static_entries[private_data->sub_idx].type;
    }
}

size_t _fl_node_getsize (const db_node_t *node) {
    assert(node);
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return 0u;
    }
    else {
        assert(private_data->fl_idx < db_get_num_fl_nodes());
        db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
        char buf[128]; // so the max length is always 128
        assert(private_data->sub_idx < fl_ent->num_static_entries);
        switch(fl_ent->static_entries[private_data->sub_idx].type) {
        case db_node_type_int:
            return sizeof(int32_t);
        case db_node_type_float:
            return sizeof(float);
        case db_node_type_str:
            return db_node_get_str_value(node, buf, 128);
        default:
            assert(0);
        }
    }
    return 0u;
}

int32_t _fl_node_getint_value (const db_node_t *node) {
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    assert(private_data->fl_idx < db_get_num_fl_nodes());
    db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
    assert(private_data->sub_idx < fl_ent->num_static_entries);
    return ( (int32_t (*)(void))
        fl_ent->static_entries[private_data->sub_idx].get_value_fn)();
}

float _fl_node_getfloat_value (const db_node_t *node) {
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    assert(private_data->fl_idx < db_get_num_fl_nodes());
    db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
    assert(private_data->sub_idx < fl_ent->num_static_entries);
    return ( (float (*)(void))
        fl_ent->static_entries[private_data->sub_idx].get_value_fn)();
}

size_t _fl_node_getstr_value (const db_node_t *node, char *value, size_t bufsize) {
    _db_fl_node_private_data_t *private_data =
        (_db_fl_node_private_data_t*) node->private_data.u8;
    assert(private_data->fl_idx < db_get_num_fl_nodes());
    db_index_entry_t *fl_ent = &db_index[private_data->fl_idx];
    assert(private_data->sub_idx < fl_ent->num_static_entries);
    return ( (size_t (*)(char*, size_t))
        fl_ent->static_entries[private_data->sub_idx].get_value_fn)(value, bufsize);
}
