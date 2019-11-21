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
 */

#include <stdio.h>

#include "doriot_dca.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <shell.h>

/* from the constfs example */

static vfs_mount_t dcafs_mount = {
    .fs = &dcafs_file_system,
    .mount_point = "/dca",
    .private_data = NULL,
};

/* Command handlers */
static int _mount(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if defined(MTD_0) && (defined(MODULE_SPIFFS) || defined(MODULE_LITTLEFS))
    int res = vfs_mount(&flash_mount);
    if (res < 0) {
        printf("Error while mounting %s...try format\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully mounted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _format(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if defined(MTD_0) && (defined(MODULE_SPIFFS) || defined(MODULE_LITTLEFS))
    int res = vfs_format(&flash_mount);
    if (res < 0) {
        printf("Error while formatting %s\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully formatted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _umount(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if defined(MTD_0) && (defined(MODULE_SPIFFS) || defined(MODULE_LITTLEFS))
    int res = vfs_umount(&flash_mount);
    if (res < 0) {
        printf("Error while unmounting %s\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully unmounted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

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

#define TREE_MAX_PATH_LEN 256
#define TREE_MAX_FILENAME_LEN 16

static int _tree_r(int8_t depth, uint8_t print_contents, char* tree_path_buf, char* tree_filename) {
    for (int i=0; i<depth; i++) printf("  ");
    printf("- %s", tree_filename);
    char* p = tree_path_buf + strnlen(tree_path_buf, TREE_MAX_PATH_LEN);
    strncat(tree_path_buf, "/", TREE_MAX_PATH_LEN);
    strncat(tree_path_buf, tree_filename, TREE_MAX_PATH_LEN);
    struct stat statbuf;
    int r = stat(tree_path_buf, &statbuf);
    if (r < 0) {
        printf("tree: stat on path %s failed\n", tree_path_buf);
        return r;
    }
    if(!(statbuf.st_mode & S_IFDIR)) {
        /* recursion end (file), print contents and return */
        if(print_contents) {
            int fd = open(tree_path_buf, O_RDONLY);
            if (fd < 0) {
                printf("file %s cannot be opened\n", tree_path_buf);
                return 1;
            }
            char str[33];
            memset(str, 0, 33);
            if (read(fd, str, 32) != 0) {
                printf(": %s", str);
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
            printf("tree: Error opening dir %s\n", tree_path_buf);
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
            printf("tree: Error reading dir %s\n", tree_path_buf);
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
    int r = stat(p, &statbuf);
    if(r < 0 || !(statbuf.st_mode & S_IFDIR)) {
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

static int _tee(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <file> <str>\n", argv[0]);
        return 1;
    }

#ifdef MODULE_NEWLIB
    FILE *f = fopen(argv[1], "w+");
    if (f == NULL) {
        printf("error while trying to create %s\n", argv[1]);
        return 1;
    }
    if (fwrite(argv[2], 1, strlen(argv[2]), f) != strlen(argv[2])) {
        puts("Error while writing");
    }
    fclose(f);
#else
    int fd = open(argv[1], O_RDWR | O_CREAT);
    if (fd < 0) {
        printf("error while trying to create %s\n", argv[1]);
        return 1;
    }
    if (write(fd, argv[2], strlen(argv[2])) != (ssize_t)strlen(argv[2])) {
        puts("Error while writing");
    }
    close(fd);
#endif
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "mount", "mount flash filesystem", _mount },
    { "format", "format flash file system", _format },
    { "umount", "unmount flash filesystem", _umount },
    { "cat", "print the content of a file", _cat },
    { "tee", "write a string in a file", _tee },
    { "tree", "print directory tree", _tree },
    { NULL, NULL, NULL }
};

int main(void)
{
    int res = vfs_mount(&dcafs_mount);
    if (res < 0) {
        puts("Error while mounting dcafs");
    }
    else {
        puts("dcafs mounted successfully");
    }

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
