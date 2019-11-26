/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */

#include <doriot_dca/ps.h>
#include <stddef.h>
#include <stdint.h>
#include <thread.h>
#include <sched.h>
#include <string.h>

typedef struct {
    /* The pid that the node represents */
    kernel_pid_t pid;
    /* 1: firstlevel branch root, 0: leaf node */
    uint8_t is_root;
} _db_ps_node_private_data_t;

char* _ps_node_getname (const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _ps_node_getnext_child (db_node_t *node, db_node_t *next_child);
int _ps_node_getnext (db_node_t *node, db_node_t *next);
db_node_type_t _ps_node_gettype (const db_node_t *node);
size_t _ps_node_getsize (const db_node_t *node);
int32_t _ps_node_getint_value (const db_node_t *node);
float _ps_node_getfloat_value (const db_node_t *node);
size_t _ps_node_getstr_value (const db_node_t *node, char *value, size_t bufsize);

static db_node_ops_t _db_ps_node_ops = {
    .get_name_fn = _ps_node_getname,
    .get_next_child_fn = _ps_node_getnext_child,
    .get_next_fn = _ps_node_getnext,
    .get_type_fn = _ps_node_gettype,
    .get_size_fn = _ps_node_getsize,
    .get_int_value_fn = _ps_node_getint_value,
    .get_float_value_fn = NULL,
    .get_str_value_fn = NULL
};

/* ps node constructor */
void _ps_node_init(db_node_t *node, kernel_pid_t pid, uint8_t is_root) {
    node->ops = &_db_ps_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    private_data->pid = pid;
    private_data->is_root = is_root;
}

kernel_pid_t _get_next_pid(kernel_pid_t pid) {
    assert(pid >= KERNEL_PID_FIRST);
    assert(pid <= KERNEL_PID_LAST+1);
    thread_t *p;
    do {
        pid += 1;
        if(pid == KERNEL_PID_LAST+1) {
            return KERNEL_PID_LAST+1;
        }
        p = (thread_t*) sched_threads[pid];
    }
    while(pid <= KERNEL_PID_LAST && p == NULL);
    return pid;
}

void db_new_ps_node(db_node_t *node) {
    assert(node);
    assert(sizeof(_db_ps_node_private_data_t) <= DB_NODE_PRIVATE_DATA_MAX);
    _ps_node_init(node, KERNEL_PID_FIRST, 1u);
}

char* _ps_node_getname (const db_node_t *node, char name[DB_NODE_NAME_MAX]) {
    assert(node);
    assert(name);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        strncpy(name, "ps", DB_NODE_NAME_MAX);
    }
    else {
        assert(private_data->pid <= KERNEL_PID_LAST);
        thread_t *p = (thread_t*) sched_threads[private_data->pid];
        assert(p != NULL);
        strncpy(name, p->name, DB_NODE_NAME_MAX);
    }
    return name;
}

int _ps_node_getnext_child (db_node_t *node, db_node_t *next_child) {
    assert(node);
    assert(next_child);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        /* return child node representing next pid, advance own pid */
        if(private_data->pid <= KERNEL_PID_LAST) {
            _ps_node_init(next_child, private_data->pid, 0u);
            private_data->pid = _get_next_pid(private_data->pid);
        }
        else{
            /* end of processes list */
            db_node_set_null(next_child);
        }
    }
    else {
        /* leaf entries do not have children */
        db_node_set_null(next_child);
    }
    return 0;
}

int _ps_node_getnext (db_node_t *node, db_node_t *next) {
    assert(node);
    assert(next);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        /* this branch root node has no neighbor */
        db_node_set_null(next);
    }
    else {
        /* retrieve the next child (leaf) node */
        if(private_data->pid <= KERNEL_PID_LAST) {
            kernel_pid_t pid = _get_next_pid(private_data->pid);
            _ps_node_init(next, pid, 0u);
        }
        else{
            /* end of processes list */
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _ps_node_gettype (const db_node_t *node) {
    assert(node);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return db_node_type_inner;
    }
    else {
        return db_node_type_int;
    }
}

size_t _ps_node_getsize (const db_node_t *node) {
    assert(node);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    if(private_data->is_root) {
        return 0u;
    }
    else {
        return sizeof(int32_t);
    }
    return 0u;
}

int32_t _ps_node_getint_value (const db_node_t *node) {
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t*) node->private_data.u8;
    assert(private_data->pid <= KERNEL_PID_LAST);
    return (int32_t) private_data->pid;
}
