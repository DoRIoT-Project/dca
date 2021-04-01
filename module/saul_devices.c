/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */

#include <doriot_dca/saul_devices.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <fmt.h>
#include "saul_reg.h"
#include "saul.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#if POSIX_C_SOURCE < 200809L
#define strnlen(a, b) strlen(a)
#endif

typedef enum
{
    CLASS,
    COUNT
} saul_property_t;

static const char *field_names[COUNT] = {
    [CLASS] = "class"};
#define FIELD_NAME_UNKNOWN "unknown"

typedef struct
{
    /* The SAUL registry entry that the node represents */
    saul_reg_t *dev;
    /* 1: firstlevel branch root, 0: leaf node */
    uint8_t is_root;
} _db_saul_node_private_data_t;

int8_t _saul_field_count = 0;
char *_saul_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _saul_node_getnext_child(db_node_t *node, db_node_t *next_child);
int _saul_node_getnext(db_node_t *node, db_node_t *next);
db_node_type_t _saul_node_gettype(const db_node_t *node);
size_t _saul_node_getsize(const db_node_t *node);
size_t _saul_node_getstr_value(const db_node_t *node, char *value, size_t bufsize);
char *_saul_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX]);

static db_node_ops_t _db_saul_node_ops = {
    .get_name_fn = _saul_node_getname,
    .get_next_child_fn = _saul_node_getnext_child,
    .get_next_fn = _saul_node_getnext,
    .get_type_fn = _saul_node_gettype,
    .get_size_fn = _saul_node_getsize,
    .get_int_value_fn = NULL,
    .get_float_value_fn = NULL,
    .get_str_value_fn = _saul_node_getstr_value};

/* saul node constructor */
void _saul_node_init(db_node_t *node, saul_reg_t *dev, uint8_t is_root)
{
    node->ops = &_db_saul_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    private_data->dev = dev;
    private_data->is_root = is_root;
}

void db_new_saul_node(db_node_t *node)
{
    assert(node);
    assert(sizeof(_db_saul_node_private_data_t) <= DB_NODE_PRIVATE_DATA_MAX);
    /* may be NULL for root */
    saul_reg_t *dev  = saul_reg;
    _saul_node_init(node, dev, 2u);
}

char *_saul_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        strncpy(name, "devices", DB_NODE_NAME_MAX);
    }
    else if (private_data->is_root == 1u)
    {
        assert(private_data->dev != NULL);
        strcpy(name,private_data->dev->name);
    }
    else if (private_data->is_root == 0u && _saul_field_count < COUNT)
    {
        strncpy(name, _saul_node_get_field_name(node, name), DB_NODE_NAME_MAX);
        DEBUG("field name:%s\n", name);
    }
    return name;
}

int _saul_node_getnext_child(db_node_t *node, db_node_t *next_child)
{
    assert(node);
    assert(next_child);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        /* return child node representing saul, advance to next */
        if (private_data->dev != NULL)
        {
            _saul_node_init(next_child, private_data->dev, 1u);
            private_data->dev = private_data->dev->next;
        }
        else
        {
            /* end of registry */
            db_node_set_null(next_child);
        }
    }
    else if (private_data->is_root == 1u)
    {
        /* return child node representing saul, advance to next*/
        if (private_data->dev != NULL && _saul_field_count < COUNT)
        {
            _saul_node_init(next_child, private_data->dev, 0u);
            private_data->dev = private_data->dev->next;
            private_data->is_root = 0u;
        }
    }
    else
    {
        if (_saul_field_count == COUNT - 1)
        {
            /* end of saul registry */
            _saul_field_count = 0;
            db_node_set_null(next_child);
        }
        else
        {
            _saul_field_count++;
        }
    }
    return 0;
}

int _saul_node_getnext(db_node_t *node, db_node_t *next)
{
    assert(node);
    assert(next);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        /* end of saul registry */
        db_node_set_null(next);
    }
    else if (private_data->is_root == 1u)
    {
        /* return child node representing saul, advance to next*/
        saul_reg_t *dev = private_data->dev->next;
        if (private_data->dev != NULL)
        {
            _saul_node_init(next, dev, 0u);
        }
        else
        {
            /* end of saul registry */
            db_node_set_null(next);
        }
    }
    else
    {
        /* retrieve the next child (leaf) node */
        saul_reg_t *dev= private_data->dev->next;
        if (private_data->dev != NULL)
        {
            _saul_node_init(next, dev, 0u);
        }
        else
        {
            /* end of saul registry */
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _saul_node_gettype(const db_node_t *node)
{
    assert(node);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        return db_node_type_inner;
    }
    else if (private_data->is_root == 1u)
    {
        return db_node_type_inner;
    }
    else /*for  class*/
    {
        return db_node_type_str;
    }
}

size_t _saul_node_getsize(const db_node_t *node)
{
    assert(node);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        return 0u;
    }
    else if (private_data->is_root == 1u)
    {
        return 0u;
    }
    return 0u;
}

size_t _saul_node_getstr_value(const db_node_t *node, char *value, size_t bufsize)
{
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    assert(private_data->dev != NULL);
    (void)bufsize;
    if (_saul_field_count == 0)
    {
        strcpy(value,saul_class_to_str(private_data->dev ->driver->type));
        return strlen(value);
    }
    return 88;
}

char *_saul_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_saul_node_private_data_t *private_data =
        (_db_saul_node_private_data_t *)node->private_data.u8;
    assert(private_data->dev != NULL);
    switch (_saul_field_count)
    {
    case 0:
        strncpy(name, field_names[CLASS], DB_NODE_NAME_MAX);
        break;
    default:
        break;
    }
    return name;
}


