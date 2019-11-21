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
 * @brief    DCA internal database representation
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 *
 * The DCA database is hierarchical, which makes it very similar to a file
 * system. The only differences are: (1) It has types (float, int32_t or
 * char*). (2) The "file" contents are short. Read operations always return
 * the entire contents, so they are not opened or indexed.
 * (3) The DCA db is read-only. (4) Folder contents are indexed, so every file
 * has a successor.
 *
 * "Files" (leaf nodes) as well as "folders" (inner nodes) are represented
 * by the db_node. Each db_node has a set of "file" operations defined
 * by the db_node_ops. Data types are represented by the db_node_type.
 *
 * "Folders" (inner nodes) never contain data. Each "file" contains exactly one
 * typed piece of data, either a uint32_t, char* (string) or float value.
 *
 * There is no malloc() used within here. All data is held on the stack, so
 * every node that is returned has to be declared within the caller's scope.
 * Each node contains its entire state. There should be no side effects.
 *
 */
#ifndef DORIOT_DCA_DB_H
#define DORIOT_DCA_DB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DB_NODE_NAME_MAX 16
#define DB_NODE_PRIVATE_DATA_MAX 8

typedef enum {
    db_node_type_null,
    db_node_type_inner,
    db_node_type_int,
    db_node_type_float,
    db_node_type_str
} db_node_type_t;

typedef struct db_node_ops db_node_ops_t;

/**
* @brief A node in the hierarchical database
*
* @param ops The operations for this node.
* @param private_data_ptr place to hold implementation-specific data
*/
typedef struct {
    db_node_ops_t* ops;
    union {
        void* ptr;
        uint8_t u8[DB_NODE_PRIVATE_DATA_MAX];
    } private_data;
} db_node;

struct db_node_ops {
    char* (*get_name_fn) (const db_node *node, char name[DB_NODE_NAME_MAX]);
    int (*get_next_child_fn) (db_node *node, db_node *next_child);
    int (*get_next_fn) (db_node *node, db_node *next); //maybe not needed?
    db_node_type_t (*get_type_fn) (const db_node *node);
    size_t (*get_size_fn) (const db_node *node);
    int32_t (*get_int_value_fn) (const db_node *node);
    float (*get_float_value_fn) (const db_node *node);
    size_t (*get_str_value_fn) (const db_node *node, char *value, size_t bufsize);
};

/** Get a root node instance */
void db_get_root(db_node* node);
/** Get a null node instance */
void db_node_set_null(db_node* node);
/** Return 1 if node is a null node */
int db_node_is_null(const db_node *node);
/** Return a node by its path name, relative to the root. */
int db_find_node_by_path(const char *path, db_node *node);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
