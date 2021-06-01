/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#include "doriot_dca/db.h"
#include "doriot_dca/board.h"

#include <assert.h>
#include <string.h>
#include <errno.h>

#include "fmt.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define FLOAT_PRESCISION 3

db_node_type_t _db_node_null_get_type(const db_node_t *node)
{
    assert(node);
    return db_node_type_null;
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

void db_node_set_null(db_node_t *node)
{
    assert(node);
    node->ops = &_db_node_null_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
}

int db_node_is_null(const db_node_t *node)
{
    assert(node);
    return (db_node_get_type(node) == db_node_type_null);
}

void db_node_copy(db_node_t *dest, const db_node_t *src)
{
    assert(dest);
    assert(src);
    memcpy(dest, src, sizeof(db_node_t));
}

int db_find_node_by_path(const char *path, db_node_t *node)
{
    assert(path);
    assert(node);
    /* find all the / in path and iterate over the folders */
    char name_buf[DB_NODE_NAME_MAX];
    db_node_t child_node;
    const char *tok = path;

    while (*tok == '/') {
        tok += 1;
    }
    const char *path_end = &path[strlen(path)];
    const char *s_pos = strchr(tok, '/');

    if (s_pos == NULL) {
        s_pos = path_end;
    }
    db_get_root(node);
    DEBUG("db_find_node_by_path(): path: \"%s\"\n", path);
    while (tok != path_end) { /* for every folder in path */
        DEBUG("db_find_node_by_path(): path: %p, tok: %p, s_pos: %p, path_end: %p\n", path, tok,
              s_pos, path_end);
        assert(tok >= path);
        assert(tok < path_end);
        assert(s_pos >= path);
        assert(s_pos <= path_end);
        assert(tok <= s_pos);
        assert(!db_node_is_null(node));
        size_t len = s_pos - tok;
        do { /* for every folder in node */
            db_node_get_next_child(node, &child_node);
            if (db_node_is_null(&child_node)) {
                /* we have searched through all entries */
                return -ENOENT;
            }
            db_node_get_name(&child_node, name_buf);
        }while(memcmp(tok, name_buf, len) != 0);
        /* folder found, now descent */
        memcpy(node, &child_node, sizeof(db_node_t));
        tok = s_pos;
        while (*tok == '/') {
            tok += 1;
        }
        s_pos = strchr(tok, '/');
        if (s_pos == NULL) {
            s_pos = path_end;
        }
    }
    return 0; /* yay :) */
}

char *db_node_get_name(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(node->ops->get_name_fn);
    return node->ops->get_name_fn(node, name);
}

int db_node_get_next_child(db_node_t *node, db_node_t *next_child)
{
    assert(node);
    assert(node->ops->get_next_child_fn);
    return node->ops->get_next_child_fn(node, next_child);
}

int db_node_get_next(db_node_t *node, db_node_t *next)
{
    assert(node);
    assert(node->ops->get_next_fn);
    return node->ops->get_next_fn(node, next);
}

db_node_type_t db_node_get_type(const db_node_t *node)
{
    assert(node);
    assert(node->ops->get_type_fn);
    return node->ops->get_type_fn(node);
}

size_t db_node_get_size(const db_node_t *node)
{
    assert(node);
    assert(node->ops->get_size_fn);
    return node->ops->get_size_fn(node);
}

int32_t db_node_get_int_value(const db_node_t *node)
{
    assert(node->ops->get_int_value_fn);
    assert(node);
    assert(db_node_get_type(node) == db_node_type_int);
    return node->ops->get_int_value_fn(node);
}

float db_node_get_float_value(const db_node_t *node)
{
    assert(node);
    assert(node->ops->get_float_value_fn);
    assert(db_node_get_type(node) == db_node_type_float);
    return node->ops->get_float_value_fn(node);
}

size_t db_node_get_str_value(const db_node_t *node, char *value, size_t bufsize)
{
    assert(node);
    assert(node->ops->get_str_value_fn);
    assert(db_node_get_type(node) == db_node_type_str);
    return node->ops->get_str_value_fn(node, value, bufsize);
}

int db_node_value_to_str(const db_node_t *node, char *buf, size_t bufsize)
{
    assert(node);
    assert(bufsize >= 4);
    size_t size = 0u;

    switch (db_node_get_type(node)) {
    case db_node_type_int:
    {
        int32_t val = db_node_get_int_value(node);
        size = fmt_s32_dec(NULL, val);
        if (size > bufsize - 1) {
            strncpy(buf, "---", bufsize);
            size = 3u;
        }
        else {
            fmt_s32_dec(buf, val);
        }
    }
    break;
    case db_node_type_float:
    {
        float val = db_node_get_float_value(node);
        /* TODO: according to doc this func uses up to 2.4kB code,
           it should become optional in some way. */
        size = fmt_float(NULL, val, FLOAT_PRESCISION);
        if (size > bufsize - 1) {
            strncpy(buf, "---", bufsize);
            size = 3u;
        }
        else {
            fmt_float(buf, val, FLOAT_PRESCISION);
        }
    }
    break;
    case db_node_type_str:
        size = db_node_get_str_value(node, buf, bufsize - 1);
        break;
    default:
        DEBUG("db_node_value_to_str: illegal node type\n");
        return -EFAULT;
    }
    buf[size] = '\0';
    size += 1;
    return (int)size;
}
