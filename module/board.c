/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @defgroup
 * @ingroup
 * @brief
 * @{
 *
 * @file
 * @brief DCA "board" tree representation
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */

#include <doriot_dca/board.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    char name[DB_NODE_NAME_MAX];
    db_node_type_t type;
    void (*get_value_fn)(void);
} _db_board_index_entry_t;

size_t _get_board_name(char* strbuf, size_t bufsize);
int _get_board_ram(void);
int _get_board_clock(void);
int _get_board_nonvolatile(void);

static _db_board_index_entry_t _db_board_index[] = {
    {"name", db_node_type_str, (void (*)(void)) _get_board_name},
    {"ram", db_node_type_int, (void (*)(void)) _get_board_ram},
    {"clock", db_node_type_int, (void (*)(void)) _get_board_clock},
    {"nonvolatile", db_node_type_int, (void (*)(void)) _get_board_nonvolatile}
};

#define _DB_BOARD_INDEX_NUMOF (sizeof(_db_board_index) / sizeof(_db_board_index_entry_t))

size_t _get_board_name(char* strbuf, size_t bufsize) {
    strncpy(strbuf, "Test Board", bufsize);
    return strlen(strbuf);
}

int _get_board_ram(void) {
    return 16536; // stonks!
}

int _get_board_clock(void) {
    return 20 * 1024 * 1024;
}

int _get_board_nonvolatile(void) {
    return 120 * 1024;
}

/* TODO: the following stuff should go somewhere else */

typedef struct {
    /* if root node: index for enumerating the leaf nodes */
    /* if not root node: index of current node */
    uint8_t idx;
    /* 1: root node, 0: leaf node */
    uint8_t is_root;
} _db_board_node_private_data_t;

char* _board_node_get_name (const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _board_node_get_next_child (db_node_t *node, db_node_t *next_child);
int _board_node_get_next (db_node_t *node, db_node_t *next);
db_node_type_t _board_node_get_type (const db_node_t *node);
size_t _board_node_get_size (const db_node_t *node);
int32_t _board_node_get_int_value (const db_node_t *node);
float _board_node_get_float_value (const db_node_t *node);
size_t _board_node_get_str_value (const db_node_t *node, char *value, size_t bufsize);

static db_node_ops_t _db_board_node_ops = {
    .get_name_fn = _board_node_get_name,
    .get_next_child_fn = _board_node_get_next_child,
    .get_next_fn = _board_node_get_next,
    .get_type_fn = _board_node_get_type,
    .get_size_fn = _board_node_get_size,
    .get_int_value_fn = _board_node_get_int_value,
    .get_float_value_fn = _board_node_get_float_value,
    .get_str_value_fn = _board_node_get_str_value
};

void db_new_board_node(db_node_t* node) {
    assert(node);
    node->ops = &_db_board_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    private_data->idx = 0;
    private_data->is_root = 1;
}

char* _board_node_get_name (const db_node_t *node, char name[DB_NODE_NAME_MAX]) {
    assert(node);
    assert(name);
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        strncpy(name, "board", DB_NODE_NAME_MAX);
    }
    else {
        assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
        strncpy(name, _db_board_index[private_data->idx].name, DB_NODE_NAME_MAX);
    }
    return name;
}

int _board_node_get_next_child (db_node_t *node, db_node_t *next_child) {
    assert(node);
    assert(next_child);
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        if(private_data->idx < _DB_BOARD_INDEX_NUMOF) {
            next_child->ops = &_db_board_node_ops;
            memset(next_child->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
            _db_board_node_private_data_t *next_private_data =
                (_db_board_node_private_data_t*) next_child->private_data.u8;
            next_private_data->is_root = 0;
            next_private_data->idx = private_data->idx;
            private_data->idx += 1;
        }
        else{
            db_node_set_null(next_child);
        }
    }
    else {
        db_node_set_null(next_child);
    }
    return 0;
}

int _board_node_get_next (db_node_t *node, db_node_t *next) {
    assert(node);
    assert(next);
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        // TODO: the "/board" node has a neighbor!
        db_node_set_null(next);
    }
    else {
        assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
        uint8_t next_idx = private_data->idx+1;
        if(next_idx < _DB_BOARD_INDEX_NUMOF) {
            next->ops = &_db_board_node_ops;
            memset(next->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
            _db_board_node_private_data_t *next_private_data =
                (_db_board_node_private_data_t*) next->private_data.u8;
            next_private_data->is_root = 0;
            next_private_data->idx = next_idx;
        }
        else{
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _board_node_get_type (const db_node_t *node) {
    assert(node);
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return db_node_type_inner;
    }
    else {
        assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
        return _db_board_index[private_data->idx].type;
    }
}

size_t _board_node_get_size (const db_node_t *node) {
    assert(node);
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return 0u;
    }
    else {
        char buf[128]; // so the max length is always 128
        assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
        switch(_db_board_index[private_data->idx].type) {
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

int32_t _board_node_get_int_value (const db_node_t *node) {
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
    return ( (int32_t (*)(void))
        _db_board_index[private_data->idx].get_value_fn)();
}

float _board_node_get_float_value (const db_node_t *node) {
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
    return ( (float (*)(void))
        _db_board_index[private_data->idx].get_value_fn)();
}

size_t _board_node_get_str_value (const db_node_t *node, char *value, size_t bufsize) {
    _db_board_node_private_data_t *private_data =
        (_db_board_node_private_data_t*) node->private_data.u8;
    assert(private_data->idx < _DB_BOARD_INDEX_NUMOF);
    return ( (size_t (*)(char*, size_t))
        _db_board_index[private_data->idx].get_value_fn)(value, bufsize);
}
