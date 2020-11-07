/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */

#include <doriot_dca/ps.h>
#include <stddef.h>
#include <stdint.h>
#include <thread.h>
#include <sched.h>
#include <string.h>
#define ENABLE_DEBUG (0)
#include "debug.h"

typedef enum
{
    PID,
    STATE,
    PRIORITY,
    STACK,
    STACK_USED,
    COUNT
} thread_property_t;

static const char *field_names[COUNT] = {
    [PID] = "pid",
    [STATE] = "state",
    [PRIORITY] = "priority",
    [STACK] = "total stack",
    [STACK_USED] = "stack used"
};

#define FIELD_NAME_UNKNOWN "unknown"

static const char *state_names[STATUS_NUMOF] = {
    [STATUS_STOPPED] = "stopped",
    [STATUS_ZOMBIE] = "zombie",
    [STATUS_SLEEPING] = "sleeping",
    [STATUS_MUTEX_BLOCKED] = "bl mutex",
    [STATUS_RECEIVE_BLOCKED] = "bl rx",
    [STATUS_SEND_BLOCKED] = "bl send",
    [STATUS_REPLY_BLOCKED] = "bl reply",
    [STATUS_FLAG_BLOCKED_ANY] = "bl anyfl",
    [STATUS_FLAG_BLOCKED_ALL] = "bl allfl",
    [STATUS_MBOX_BLOCKED] = "bl mbox",
    [STATUS_COND_BLOCKED] = "bl cond",
    [STATUS_RUNNING] = "running",
    [STATUS_PENDING] = "pending",
};
#define STATE_NAME_UNKNOWN "unknown"

typedef struct
{
    /* The pid that the node represents */
    kernel_pid_t pid;
    /* 2: firstlevel branch root,1: secondlevel branch root, 0: leaf node*/
    uint8_t is_root;

} _db_ps_node_private_data_t;

int8_t _field_count = 0;
char *_ps_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX]);
int _ps_node_getnext_child(db_node_t *node, db_node_t *next_child);
int _ps_node_getnext(db_node_t *node, db_node_t *next);
db_node_type_t _ps_node_gettype(const db_node_t *node);
size_t _ps_node_getsize(const db_node_t *node);
int32_t _ps_node_getint_value(const db_node_t *node);
float _ps_node_getfloat_value(const db_node_t *node);
size_t _ps_node_getstr_value(const db_node_t *node, char *value, size_t bufsize);
char *_ps_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX]);

static db_node_ops_t _db_ps_node_ops = {
    .get_name_fn = _ps_node_getname,
    .get_next_child_fn = _ps_node_getnext_child,
    .get_next_fn = _ps_node_getnext,
    .get_type_fn = _ps_node_gettype,
    .get_size_fn = _ps_node_getsize,
    .get_int_value_fn = _ps_node_getint_value,
    .get_float_value_fn = NULL,
    .get_str_value_fn = _ps_node_getstr_value};

/* ps node constructor */
void _ps_node_init(db_node_t *node, kernel_pid_t pid, uint8_t is_root)
{

    node->ops = &_db_ps_node_ops;
    memset(node->private_data.u8, 0, DB_NODE_PRIVATE_DATA_MAX);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    private_data->pid = pid;
    private_data->is_root = is_root;
}

kernel_pid_t _get_next_pid(kernel_pid_t pid)
{
    assert(pid >= KERNEL_PID_FIRST);
    assert(pid <= KERNEL_PID_LAST + 1);
    thread_t *p;
    do
    {
        pid += 1;
        if (pid == KERNEL_PID_LAST + 1)
        {
            return KERNEL_PID_LAST + 1;
        }
        p = (thread_t *)sched_threads[pid];
    } while (pid <= KERNEL_PID_LAST && p == NULL);
    return pid;
}

void db_new_ps_node(db_node_t *node)
{
    assert(node);
    assert(sizeof(_db_ps_node_private_data_t) <= DB_NODE_PRIVATE_DATA_MAX);
    _ps_node_init(node, KERNEL_PID_FIRST, 2u);
}

char *_ps_node_getname(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        strncpy(name, "ps", DB_NODE_NAME_MAX);
    }
    else if (private_data->is_root == 1u)
    {
        assert(private_data->pid <= KERNEL_PID_LAST);
        thread_t *p = (thread_t *)sched_threads[private_data->pid];
        assert(p != NULL);
        DEBUG("subroot name:%s\n", p->name);
        strncpy(name, p->name, DB_NODE_NAME_MAX);
    }

    else if (private_data->is_root == 0u && _field_count < COUNT)
    {
        strncpy(name, _ps_node_get_field_name(node, name), DB_NODE_NAME_MAX);
        DEBUG("field name:%s\n", name);
    }
    return name;
}

int _ps_node_getnext_child(db_node_t *node, db_node_t *next_child)

{
    assert(node);
    assert(next_child);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;

    if (private_data->is_root == 2u)
    {
        if (private_data->pid <= KERNEL_PID_LAST)
        {
            _ps_node_init(next_child, private_data->pid, 1u);
            private_data->pid = _get_next_pid(private_data->pid);
        }
        else
        {
            /* end of processes list */
            db_node_set_null(next_child);
        }
    }

    else if (private_data->is_root == 1u)
    {
        /* return child node representing next pid, advance own pid */
        if (private_data->pid <= KERNEL_PID_LAST && _field_count < COUNT)
        {
            DEBUG("current pid :%d\n", private_data->pid);
            _ps_node_init(next_child, private_data->pid, 0u);
            private_data->pid = _get_next_pid(private_data->pid);
            private_data->is_root = 0u;
        }
    }

    else
    {
        if (_field_count == COUNT - 1)
        {
            /* end of processes list */
            _field_count = 0;
            db_node_set_null(next_child);
        }
        else
        {
            _field_count++;
        }
    }
    return 0;
}

int _ps_node_getnext(db_node_t *node, db_node_t *next)
{
    assert(node);
    assert(next);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        /* this branch root node has no neighbor */
        db_node_set_null(next);
    }
    else if (private_data->is_root == 1u)
    {
        /* this branch root node has no neighbor */
        if (private_data->pid <= KERNEL_PID_LAST)
        {
            kernel_pid_t pid = _get_next_pid(private_data->pid);
            _ps_node_init(next, pid, 1u);
        }
        else
        {
            /* end of processes list */
            db_node_set_null(next);
        }
    }

    else
    {
        /* retrieve the next child (leaf) node */
        if (private_data->pid <= KERNEL_PID_LAST)
        {
            kernel_pid_t pid = _get_next_pid(private_data->pid);
            _ps_node_init(next, pid, 0u);
        }
        else
        {
            /* end of processes list */
            db_node_set_null(next);
        }
    }
    return 0;
}

db_node_type_t _ps_node_gettype(const db_node_t *node)
{
    assert(node);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u) /* root */
    {
        return db_node_type_inner;
    }
    else if (private_data->is_root == 1u) /* subroot */
    {

        return db_node_type_inner;
    }
    else /* for fields */
    {
        switch (_field_count)
        {
        case 0:
            /* for pid */
            return db_node_type_int;
            break;
        case 1:
            /* for STATE */
            return db_node_type_str;
            break;
        case 2:
            /* for PRIORITY */
            return db_node_type_int;
            break;
        case 3:
            /*for STACK */
            return db_node_type_int;
            break;
        default: 
            /*for STACK USED*/
            return db_node_type_int;
            break;
        }
    }    
}

size_t _ps_node_getsize(const db_node_t *node)
{
    assert(node);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    if (private_data->is_root == 2u)
    {
        return 0u;
    }
    else if (private_data->is_root == 1u)
    {
        return 0u;
    }
    else
    {
        return sizeof(int32_t);
    }
    return 0u;
}

int32_t _ps_node_getint_value(const db_node_t *node)
{
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    assert(private_data->pid <= KERNEL_PID_LAST);
    switch (_field_count)
    {
    case 0:
        return (int32_t)private_data->pid;
        break; 
    case 2:
        {
            thread_t *p = (thread_t *)sched_threads[private_data->pid];
            assert(p != NULL);
            return p->priority;
            break;
        }
    case 3:
        {
            thread_t *p = (thread_t *)sched_threads[private_data->pid];
            assert(p != NULL);
            return p->stack_size;
            break;  
        }
    case 4:
        {
            thread_t *p = (thread_t *)sched_threads[private_data->pid];
            assert(p != NULL);
            int stacksz = p->stack_size;
            int stack_free = thread_measure_stack_free(p->stack_start);
            stacksz -= stack_free;
            return stacksz;
            break;
        }
    default:
        return -1;
        break;
    }
}

size_t _ps_node_getstr_value(const db_node_t *node, char *value, size_t bufsize)
{
    (void)bufsize;
    assert(node);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;

    assert(private_data->pid <= KERNEL_PID_LAST);
    thread_t *p = (thread_t *)sched_threads[private_data->pid];
    assert(p != NULL);

    if (_field_count == 1)
    {
        strncpy(value, state_names[p->status], DB_NODE_NAME_MAX);
    }
    return 88;
}

char *_ps_node_get_field_name(const db_node_t *node, char name[DB_NODE_NAME_MAX])
{
    assert(node);
    assert(name);
    _db_ps_node_private_data_t *private_data =
        (_db_ps_node_private_data_t *)node->private_data.u8;
    assert(private_data->pid <= KERNEL_PID_LAST);
    switch (_field_count)
    {
    case 0:
        strncpy(name, field_names[PID], DB_NODE_NAME_MAX);
        break;
    case 1:
        strncpy(name, field_names[STATE], DB_NODE_NAME_MAX);
        break;
    case 2:
        strncpy(name, field_names[PRIORITY], DB_NODE_NAME_MAX);
        break;
    case 3:
        strncpy(name, field_names[STACK], DB_NODE_NAME_MAX);
        break;
    case 4:
        strncpy(name, field_names[STACK_USED], DB_NODE_NAME_MAX);
        break;
    default:
        break;
    }
    return name;
}