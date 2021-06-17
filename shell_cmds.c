/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */

#include "doriot_dca.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "xfa.h"
#include "shell.h"
#include "stdio_base.h"

#define DCA_SHELL_STRBUF_SIZE 128

#ifdef CONFIG_DCA_SHELL

static void _puts(const char *str)
{
    stdio_write(str, strlen(str));
}

static int _dcaq(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <path>\nquery the DCA database\n", argv[0]);
        return 1;
    }
    char *path = argv[1];
    db_node_t node;
    int r = db_find_node_by_path(path, &node);

    if (r < 0) {
        printf("path %s does not exist\n", path);
        return 1;
    }
    db_node_type_t type = db_node_get_type(&node);

    if (type == db_node_type_inner) {
        /* print a list of all child nodes */
        db_node_t child;
        db_node_get_next_child(&node, &child);
        char name[DB_NODE_NAME_MAX];
        while (!db_node_is_null(&child)) {
            db_node_get_name(&child, name);
            _puts(name);
            putchar('\n');
            db_node_get_next_child(&node, &child);
        }
    }
    else if (type == db_node_type_int
             || type == db_node_type_float
             || type == db_node_type_str) {
        char str[DCA_SHELL_STRBUF_SIZE];
        db_node_value_to_str(&node, str, DCA_SHELL_STRBUF_SIZE);
        _puts(str);
        putchar('\n');
    }
    else {
        printf("node %s has invalid type\n", path);
        return 1;
    }

    return 0;
}

static int _tree_r(int8_t depth, uint8_t print_contents, db_node_t *node)
{
    assert(node);
    char name[DB_NODE_NAME_MAX];
    db_node_type_t type = db_node_get_type(node);

    db_node_get_name(node, name);
    for (int i = 0; i < depth; i++) {
        _puts("  ");
    }
    _puts("- ");
    _puts(name);
    if (type == db_node_type_int
        || type == db_node_type_float
        || type == db_node_type_str) {
        /* recursion end (file), print contents and return */
        if (print_contents) {
            char str[DCA_SHELL_STRBUF_SIZE];
            db_node_value_to_str(node, str, DCA_SHELL_STRBUF_SIZE);
            _puts(": ");
            _puts(str);
        }
        putchar('\n');
    }
    else if (type == db_node_type_inner) {
        /* go deeper */
        putchar('/');
        putchar('\n');
        db_node_t child;
        db_node_get_next_child(node, &child);
        while (!db_node_is_null(&child)) {
            int r = _tree_r(depth + 1, print_contents, &child);
            if (r < 0) {
                return r; /* in case of error */
            }
            db_node_get_next_child(node, &child);
        }
    }
    else {
        _puts("node ");
        _puts(name);
        _puts(" has invalid type\n");
    }
    return 0;
}

static int _tree(const char *path, uint8_t print_contents)
{
    db_node_t node;
    int r = db_find_node_by_path(path, &node);

    if (r < 0) {
        printf("path %s does not exist\n", path);
        return 1;
    }
    return _tree_r(0, print_contents, &node);
}

static int _dcadump(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return _tree("/", 1u);
}

XFA_USE_CONST(shell_command_t *, shell_commands_xfa);

shell_command_t _dcaq_cmd = { "dcaq", "Query DCA database", _dcaq };
shell_command_t _dcadump_cmd = { "dcadump", "Dump the whole DCA database", _dcadump };

XFA_ADD_PTR(
    shell_commands_xfa,
    0,
    sc_dcaq,
    &_dcaq_cmd
    );

XFA_ADD_PTR(
    shell_commands_xfa,
    0,
    sc_dcadump,
    &_dcadump_cmd
    );

#endif /* defined(CONFIG_DCA_SHELL) */
