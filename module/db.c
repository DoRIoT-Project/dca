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
 * @brief DCA internal database representation
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#include <doriot_dca/db.h>
#include <doriot_dca/board.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define ENABLE_DEBUG (0)
#include "debug.h"

db_node_type_t _db_node_null_get_type(const db_node *node) {
    assert(node);
    return db_node_type_null;
}

char* _root_get_name (const db_node *node, char name[DB_NODE_NAME_MAX]) {
    (void) node;
    strncpy(name, "/", DB_NODE_NAME_MAX);
    return name;
}

int _root_get_next_child (db_node *node, db_node *next_child) {
    assert(node);
    assert(next_child);
    uint8_t idx = node->private_data.u8[0];
    switch(idx) {
    case 0:
        db_new_board_node(next_child);
        break;
    default:
        db_node_set_null(next_child);
        break;
    }
    node->private_data.u8[0] = idx + 1;
    return 0;
}

int _root_get_next (db_node *node, db_node *next) {
    assert(node);
    assert(next);
    db_node_set_null(next);
    return 0;
}

db_node_type_t _root_get_type (const db_node *node) {
    (void) node;
    return db_node_type_inner;
}

size_t _root_get_size (const db_node *node) {
    (void) node;
    return 0u;
}

static db_node_ops_t _db_node_null_ops = {
    .get_name_fn = NULL,
    .get_next_child_fn = NULL,
    .get_next_fn = NULL,
    .get_type_fn = _db_node_null_get_type,
    .get_size_fn = NULL,
    .get_int_value_fn = NULL,
    .get_float_value_fn = NULL,
    .get_str_value_fn = NULL
};

void db_node_set_null(db_node* node) {
    assert(node);
    node->ops = &_db_node_null_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
}

int db_node_is_null(const db_node *node) {
    assert(node);
    return (node->ops->get_type_fn(node) == db_node_type_null);
}

static db_node_ops_t _db_root_ops = {
    .get_name_fn = _root_get_name,
    .get_next_child_fn = _root_get_next_child,
    .get_next_fn = _root_get_next,
    .get_type_fn = _root_get_type,
    .get_size_fn = _root_get_size,
    .get_int_value_fn = NULL,
    .get_float_value_fn = NULL,
    .get_str_value_fn = NULL
};

void db_get_root(db_node* node) {
    assert(node);
    node->ops = &_db_root_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
}

int db_find_node_by_path(const char *path, db_node *node) {
    assert(path);
    assert(node);
    /* find all the / in path and iterate over the folders */
    char name_buf[DB_NODE_NAME_MAX];
    db_node child_node;
    const char *tok = path;
    while(*tok == '/') tok += 1;
    const char *path_end = &path[strlen(path)];
    const char *s_pos = strchr(tok, '/');
    if(s_pos == NULL) s_pos = path_end;
    db_get_root(node);
    DEBUG("db_find_node_by_path(): path: \"%s\"\n", path);
    while (tok != path_end) { /* for every folder in path */
        DEBUG("db_find_node_by_path(): path: %p, tok: %p, s_pos: %p, path_end: %p\n", path, tok, s_pos, path_end);
        assert(tok >= path);
        assert(tok < path_end);
        assert(s_pos >= path);
        assert(s_pos <= path_end);
        assert(tok <= s_pos);
        assert(!db_node_is_null(node));
        size_t len = s_pos - tok;
        do { /* for every folder in node */
            node->ops->get_next_child_fn(node, &child_node);
            if(db_node_is_null(&child_node)) {
                /* we have searched through all entries */
                return -ENOENT;
            }
            child_node.ops->get_name_fn(&child_node, name_buf);
        }
        while(memcmp(tok, name_buf, len) != 0);
        /* folder found, now descent */
        memcpy(node, &child_node, sizeof(db_node));
        tok = s_pos;
        while(*tok == '/') tok += 1;
        s_pos = strchr(tok, '/');
        if(s_pos == NULL) s_pos = path_end;
    }
    return 0; /* yay :) */
}
