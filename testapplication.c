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
 * @brief
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 */
#include <stdio.h>
#include <doriot_dca.h>
#include <doriot_dca/db_node.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio_base.h>
#include <msg.h>
#include <shell.h>
#include "saul_reg.h"
#include "saul.h"
#include <doriot_dca/udp_throughput.h>
#include <doriot_dca/latency.h>

#define ENABLE_DEBUG (0)
#include <debug.h>

#if POSIX_C_SOURCE < 200809L
    #define strnlen(a,b) strlen(a)
#endif

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* created from the constfs example */
#if defined(USE_DCAFS)
static vfs_mount_t dcafs_mount = {
    .fs = &dcafs_file_system,
    .mount_point = "/dca",
    .private_data = NULL,
};
#endif /* defined(USE_DCAFS) */

static int _cat(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    /* With newlib, low-level syscalls are plugged to RIOT vfs
     * on native, open/read/write/close/... are plugged to RIOT vfs */
#ifdef MODULE_NEWLIB
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    char c;
    while (fread(&c, 1, 1, f) != 0) {
        putchar(c);
    }
    if(c != '\n') putchar('\n');
    fclose(f);
#else
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    char c;
    while (read(fd, &c, 1) != 0) {
        putchar(c);
    }
    if(c != '\n') putchar('\n');
    close(fd);
#endif
    return 0;
}

void _puts(const char* str) {
    stdio_write(str, strlen(str));
}

#define TREE_MAX_PATH_LEN 256
#define TREE_MAX_FILENAME_LEN 30

static int _tree_r(int8_t depth, uint8_t print_contents, char* tree_path_buf, char* tree_filename) {
    /*DEBUG("_tree_r(): %p\n", &depth);*/
    for (int i=0; i<depth; i++) _puts("  ");
    _puts("- ");
    _puts(tree_filename);
    char* p = tree_path_buf + strnlen(tree_path_buf, TREE_MAX_PATH_LEN);
    strncat(tree_path_buf, "/", TREE_MAX_PATH_LEN);
    strncat(tree_path_buf, tree_filename, TREE_MAX_PATH_LEN);
    struct stat statbuf;
    int r = vfs_stat(tree_path_buf, &statbuf);
    if (r < 0) {
        _puts("tree: stat on path ");
        _puts(tree_path_buf);
        _puts(" failed\n");
        return r;
    }
    if(!(statbuf.st_mode & S_IFDIR)) {
        /* recursion end (file), print contents and return */
        if(print_contents) {
            int fd = open(tree_path_buf, O_RDONLY);
            if (fd < 0) {
                _puts("file ");
                _puts(tree_path_buf);
                _puts(" cannot be opened\n");
                return 1;
            }
            char str[33];
            memset(str, 0, 33);
            if (read(fd, str, 32) != 0) {
                _puts(": ");
                _puts(str);
            }
            close(fd);
        }
        putchar('\n');
    }
    else {
        /* it is a directory, go deeper */
        putchar('/');
        putchar('\n');
        vfs_DIR dir;
        r = vfs_opendir(&dir, tree_path_buf);
        if (r < 0) {
            _puts("tree: Error opening dir ");
            _puts(tree_path_buf);
            putchar('\n');
            return r;
        }
        vfs_dirent_t entry;
        while((r = vfs_readdir(&dir, &entry)) > 0) {
            /* works only because _tree_filename is not used after _tree_r() call */
            strncpy(tree_filename, entry.d_name, TREE_MAX_FILENAME_LEN);
            r = _tree_r(depth+1, print_contents, tree_path_buf, tree_filename);
            if (r < 0) {
                return r;
            }
        }
        if (r < 0) {
            _puts("tree: Error reading dir ");
            _puts(tree_path_buf);
            putchar('\n');
            return r;
        }
        vfs_closedir(&dir);
    }
    /* remove dir from path*/
    *p = '\0';
    return 0;
}

static int _tree(int argc, char **argv)
{
    if (argc < 2 || (argv[1][0] == '-' && argc < 3) ) {
        printf("Usage: %s [-p] <path>\nshow folder contents as a tree\n", argv[0]);
        printf("-p also prints file contents\n");
        return 1;
    }
    char *p = argv[1];
    uint8_t print_contents = 0;
    if(p[0] == '-') {
        print_contents = 1;
        p = argv[2];
    }
    struct stat statbuf;
    int r = vfs_stat(p, &statbuf);
    if(r < 0 || !(statbuf.st_mode & S_IFDIR)) {
        perror("");
        printf("path %s does not exist or is not a directory\n", p);
        return r;
    }
    while (*p == '/') p += 1;
    assert(*p);
    char tree_path_buf[TREE_MAX_PATH_LEN+1];
    char tree_filename[TREE_MAX_FILENAME_LEN+1];
    memset(tree_path_buf, 0, TREE_MAX_PATH_LEN+1);
    memset(tree_filename, 0, TREE_MAX_FILENAME_LEN+1);
    strncpy(tree_filename, p, TREE_MAX_FILENAME_LEN);
    return _tree_r(0, print_contents, tree_path_buf, tree_filename);
}

static int mock_test_saul(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    int res;
    static const saul_driver_t s0_dri = {NULL, NULL, SAUL_ACT_SERVO};
    static const saul_driver_t s1_dri = {NULL, NULL, SAUL_SENSE_TEMP};
    static const saul_driver_t s2_dri = {NULL, NULL, SAUL_SENSE_LIGHT};
    static const saul_driver_t s3_dri = {NULL, NULL, SAUL_ACT_LED_RGB};
    static const saul_driver_t s4_dri = {NULL, NULL, SAUL_ACT_SWITCH};

    static saul_reg_t s0 = {NULL, NULL, "act_1", &s0_dri};
    static saul_reg_t s1 = {NULL, NULL, "sens_2", &s1_dri};
    static saul_reg_t s2 = {NULL, NULL, "sens_3", &s2_dri};
    static saul_reg_t s3 = {NULL, NULL, "act_4", &s3_dri};
    static saul_reg_t s4 = {NULL, NULL, "act_5", &s4_dri};

    res = saul_reg_add(&s0);
    res = saul_reg_add(&s1);
    res = saul_reg_add(&s2);
    res = saul_reg_add(&s3);
    res = saul_reg_add(&s4);
    return res;
}

static const shell_command_t shell_commands[] = {
    { "cat", "print the content of a file", _cat },
    { "tree", "print directory tree", _tree },
    { "latency", "measure latency and packet loss to neighbors", network_latency },
    { "throughput", "measure throughput to neighbors", network_throughput },
    { "mock_saul", "adding devices to saul registry for testing ", mock_test_saul },
    { NULL, NULL, NULL }
};

int main(void)
{
#if defined(USE_DCAFS)
    int res = vfs_mount(&dcafs_mount);
    if (res < 0) {
        puts("Error while mounting dcafs");
    }
    else {
        puts("dcafs mounted successfully");
    }
#endif /* defined(USE_DCAFS) */

    udp_server(1883);    
    msg_init_queue(_main_msg_queue,MAIN_QUEUE_SIZE);
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
