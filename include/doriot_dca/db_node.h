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
 * by the db_node_t. Each db_node_t has a set of "file" operations defined
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
#ifndef DORIOT_DCA_DB_NODE_H
#define DORIOT_DCA_DB_NODE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DB_NODE_NAME_MAX 30
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
    db_node_ops_t *ops;
    union {
        void *ptr;
        uint8_t u8[DB_NODE_PRIVATE_DATA_MAX];
    } private_data;
} db_node_t;

/**
 * Node operations.
 * They should not be called directly, but through their respective wrappers.
 */
struct db_node_ops {
    char * (*get_name_fn) (const db_node_t *node, char name[DB_NODE_NAME_MAX]);
    int (*get_next_child_fn) (db_node_t *node, db_node_t *next_child);
    int (*get_next_fn) (db_node_t *node, db_node_t *next); //maybe not needed?
    db_node_type_t (*get_type_fn) (const db_node_t *node);
    size_t (*get_size_fn) (const db_node_t *node);
    int32_t (*get_int_value_fn) (const db_node_t *node);
    float (*get_float_value_fn) (const db_node_t *node);
    size_t (*get_str_value_fn) (const db_node_t *node, char *value, size_t bufsize);
};

/** Get a null node instance */
void db_node_set_null(db_node_t *node);
/** Return 1 if node is a null node */
int db_node_is_null(const db_node_t *node);
/** Copy node */
void db_node_copy(db_node_t *dest, const db_node_t *src);
/** Return a node by its path name, relative to the root. */
int db_find_node_by_path(const char *path, db_node_t *node);

/* Ops wrappers */

/** Get name of node */
char *db_node_get_name(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
/** Iterate through children of node, terminating with a null node */
int db_node_get_next_child(db_node_t *node, db_node_t *next_child);
/** Get the neighbor of this node. TODO: Not implemented everywhere */
int db_node_get_next(db_node_t *node, db_node_t *next);
/** Get type of the node */
db_node_type_t db_node_get_type(const db_node_t *node);
/** Get size of the node's data, if it is a leaf node */
size_t db_node_get_size(const db_node_t *node);
/** Get value of node, if it has db_node_type_int */
int32_t db_node_get_int_value(const db_node_t *node);
/** Get value of node, if it has db_node_type_float */
float db_node_get_float_value(const db_node_t *node);
/** Get value of node, if it has db_node_type_str */
size_t db_node_get_str_value(const db_node_t *node, char *value, size_t bufsize);

/* Helper functions */

/** Convert the node value into a string */
int db_node_value_to_str(const db_node_t *node, char *buf, size_t bufsize);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
