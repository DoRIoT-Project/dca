/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */
#include <doriot_dca/db.h>
#include <doriot_dca/board.h>
#include <doriot_dca/runtime.h>
#include <doriot_dca/db_fl.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static db_index_static_entry_t _board_static_entries[] =
{
    {"name", db_node_type_str, (void (*)(void)) board_get_name},
    {"mcu", db_node_type_str, (void (*)(void)) board_get_mcu},
    {"ram", db_node_type_int, (void (*)(void)) board_get_ram},
    {"clock", db_node_type_int, (void (*)(void)) board_get_clock},
    {"nonvolatile", db_node_type_int, (void (*)(void)) board_get_nonvolatile}
};

static db_index_static_entry_t _runtime_static_entries[] =
{
	{"cpu_load", db_node_type_int, (void (*)(void)) runtime_get_cpu_load},
	{"num_processes", db_node_type_int, (void (*)(void)) runtime_get_num_processes},
};

db_index_entry_t db_index[] =
{
	{
		.branch_name = "board",
		.num_static_entries = ARRAY_SIZE(_board_static_entries),
		.static_entries = _board_static_entries,
		.num_dynamic_entries = 0,
		.dynamic_entries = 0
	},
	{
		.branch_name = "runtime",
		.num_static_entries = ARRAY_SIZE(_runtime_static_entries),
		.static_entries = _runtime_static_entries,
		.num_dynamic_entries = 0,
		.dynamic_entries = 0
	},
};

size_t db_get_num_fl_nodes(void) {
    return ARRAY_SIZE(db_index);
}

/* for root node; could go to a separate file ... */

char* _root_get_name (const db_node_t *node, char name[DB_NODE_NAME_MAX]) {
    (void) node;
    strncpy(name, "/", DB_NODE_NAME_MAX);
    return name;
}

int _root_get_next_child (db_node_t *node, db_node_t *next_child) {
    assert(node);
    assert(next_child);
    uint8_t idx = node->private_data.u8[0];
    if(idx < ARRAY_SIZE(db_index)) {
        db_new_fl_node(next_child, idx);
        node->private_data.u8[0] = idx + 1;
    }
    else {
        db_node_set_null(next_child);
    }
    return 0;
}

int _root_get_next (db_node_t *node, db_node_t *next) {
    assert(node);
    assert(next);
    db_node_set_null(next);
    return 0;
}

db_node_type_t _root_get_type (const db_node_t *node) {
    (void) node;
    return db_node_type_inner;
}

size_t _root_get_size (const db_node_t *node) {
    (void) node;
    return 0u;
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

void db_get_root(db_node_t* node) {
    assert(node);
    node->ops = &_db_root_ops;
    /* node->private_data.u8 is the current element in db_index[] */
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
}
