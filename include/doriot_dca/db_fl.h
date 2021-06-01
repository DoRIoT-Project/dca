/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @defgroup doriot_dca DoRIoT Data Collection Agent
 * @ingroup  doriot
 * @brief    Collects data regarding Network, CPU, QoS resources
 * @{
 *
 * @file
 * @brief    DCA firstlevel database branch
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 *
 */
#ifndef DORIOT_DCA_DB_FL_H
#define DORIOT_DCA_DB_FL_H

#include "doriot_dca/db_node.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[DB_NODE_NAME_MAX];
    void (*get_node_fn)(db_node_t *node);
} db_fl_dynamic_entry_t;

typedef struct {
    /** Entry (node) name */
    char name[DB_NODE_NAME_MAX];
    /** Entry (node) type */
    db_node_type_t type;
    /** Entry (node) value function */
    void (*get_value_fn)(void);
} db_fl_static_entry_t;

typedef struct {
    /** Name of the top node of the branch. */
    char branch_name[DB_NODE_NAME_MAX];
    /** Number of elements in entries */
	size_t num_static_entries;
    /** Array of static entries */
    db_fl_static_entry_t* static_entries;
    /** Number of elements in dynamic_entries */
	size_t num_dynamic_entries;
    /** Array of dynamic entries */
	db_fl_dynamic_entry_t* dynamic_entries;
} db_fl_entry_t;

void db_new_fl_node(db_node_t *next_child, uint8_t fl_idx);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
