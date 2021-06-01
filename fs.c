/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */

#include "doriot_dca.h"
#include "doriot_dca/fs.h"
#include "doriot_dca/db.h"

#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "vfs.h"
#include "fmt.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/* File system operations */
static int dcafs_mount(vfs_mount_t *mountp);
static int dcafs_umount(vfs_mount_t *mountp);
static int dcafs_unlink(vfs_mount_t *mountp, const char *name);
static int dcafs_stat(vfs_mount_t *mountp, const char *restrict name, struct stat *restrict buf);
static int dcafs_statvfs(vfs_mount_t *mountp, const char *restrict path, struct statvfs *restrict buf);

/* File operations */
static int dcafs_close(vfs_file_t *filp);
static int dcafs_fstat(vfs_file_t *filp, struct stat *buf);
static off_t dcafs_lseek(vfs_file_t *filp, off_t off, int whence);
static int dcafs_open(vfs_file_t *filp, const char *name, int flags, mode_t mode, const char *abs_path);
static ssize_t dcafs_read(vfs_file_t *filp, void *dest, size_t nbytes);
static ssize_t dcafs_write(vfs_file_t *filp, const void *src, size_t nbytes);

/* Directory operations */
static int dcafs_opendir(vfs_DIR *dirp, const char *dirname, const char *abs_path);
static int dcafs_readdir(vfs_DIR *dirp, vfs_dirent_t *entry);
static int dcafs_closedir(vfs_DIR *dirp);

static const vfs_file_system_ops_t dcafs_fs_ops = {
    .mount = dcafs_mount,
    .umount = dcafs_umount,
    .unlink = dcafs_unlink,
    .statvfs = dcafs_statvfs,
    .stat = dcafs_stat,
};

static const vfs_file_ops_t dcafs_file_ops = {
    .close = dcafs_close,
    .fstat = dcafs_fstat,
    .lseek = dcafs_lseek,
    .open  = dcafs_open,
    .read  = dcafs_read,
    .write = dcafs_write,
};

static const vfs_dir_ops_t dcafs_dir_ops = {
    .opendir = dcafs_opendir,
    .readdir = dcafs_readdir,
    .closedir = dcafs_closedir,
};

const vfs_file_system_t dcafs_file_system = {
    .f_op = &dcafs_file_ops,
    .fs_op = &dcafs_fs_ops,
    .d_op = &dcafs_dir_ops,
};

/**
 * @internal
 * @brief Fill a file information struct with information about the file
 * pointed to by @p fp
 *
 * @param[in]  node     node to query
 * @param[out] buf    output buffer
 */
static void _dcafs_write_stat(const db_node_t *node, struct stat *restrict buf);

static int dcafs_mount(vfs_mount_t *mountp)
{
    (void) mountp;
    return 0;
}

static int dcafs_umount(vfs_mount_t *mountp)
{
    (void) mountp;
    return 0;
}

static int dcafs_unlink(vfs_mount_t *mountp, const char *name)
{
    (void) mountp;
    (void) name;
    return -EROFS;
}

static int dcafs_stat(vfs_mount_t *mountp, const char *restrict name, struct stat *restrict buf)
{
    (void) mountp;
    if (buf == NULL) {
        return -EFAULT;
    }
    int ret;
    db_node_t node;
    ret = db_find_node_by_path(name, &node);
    if(ret < 0) {
        DEBUG("dcafs_stat: Not found :(\n");
        return ret;
    }
    _dcafs_write_stat(&node, buf);
    DEBUG("dcafs_stat: Found :)\n");
    return 0;
}

static int dcafs_statvfs(vfs_mount_t *mountp, const char *restrict path, struct statvfs *restrict buf)
{
    (void) mountp;
    (void) path;
    if (buf == NULL) {
        return -EFAULT;
    }
    memset(buf, 0, sizeof(*buf));
    buf->f_bsize = sizeof(uint8_t); /* block size */
    buf->f_frsize = sizeof(uint8_t); /* fundamental block size */
    fsblkcnt_t f_blocks = 1;
    /* we could count everything here, but that is too costly
    for (size_t i = 0; i < fs->nfiles; ++i) {
        f_blocks += fs->files[i].size;
    }
    */
    buf->f_blocks = f_blocks;  /* Blocks total */
    buf->f_bfree = 0;          /* Blocks free */
    buf->f_bavail = 0;         /* Blocks available to non-privileged processes */
    buf->f_files = 0;          /* Total number of file serial numbers */
    buf->f_ffree = 0;          /* Total number of free file serial numbers */
    buf->f_favail = 0;         /* Number of file serial numbers available to non-privileged process */
    buf->f_fsid = 0;           /* File system id */
    buf->f_flag = (ST_RDONLY | ST_NOSUID); /* File system flags */
    buf->f_namemax = UINT8_MAX; /* Maximum file name length */
    return 0;
}

static int dcafs_close(vfs_file_t *filp)
{
    (void) filp;
    return 0;
}

static int dcafs_fstat(vfs_file_t *filp, struct stat *buf)
{
    if (buf == NULL) {
        return -EFAULT;
    }
    db_node_t *node = (db_node_t*) filp->private_data.buffer;
    _dcafs_write_stat(node, buf);
    return 0;
}

static off_t dcafs_lseek(vfs_file_t *filp, off_t off, int whence)
{
    db_node_t *node = (db_node_t*) filp->private_data.buffer;
    switch (whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            off += filp->pos;
            break;
        case SEEK_END:
            off += db_node_get_size(node);
            break;
        default:
            return -EINVAL;
    }
    if (off < 0) {
        return -EINVAL;
    }
    filp->pos = off;
    return off;
}

static int dcafs_open(vfs_file_t *filp, const char *name, int flags, mode_t mode, const char *abs_path)
{
    (void) mode;
    (void) abs_path;
    DEBUG("constfs_open: %p, \"%s\", 0x%x, 0%03lo, \"%s\"\n", (void *)filp, name, flags, (unsigned long)mode, abs_path);
    if ((flags & O_ACCMODE) != O_RDONLY) {
        return -EROFS;
    }
    int ret;
    db_node_t node;
    ret = db_find_node_by_path(name, &node);
    if(ret < 0) {
        DEBUG("dcafs_open: Not found :(\n");
        return ret;
    }
    memcpy(filp->private_data.buffer, &node, sizeof(db_node_t));
    DEBUG("dcafs_open: Found :)\n");
    return 0;
}

static ssize_t dcafs_read(vfs_file_t *filp, void *dest, size_t nbytes)
{
    DEBUG("dcafs_read: %p, %p, %lu\n", (void *)filp, dest, (unsigned long)nbytes);
    db_node_t *node = (db_node_t*) filp->private_data.buffer;
    size_t size;
    /* TODO: get_str_value_fn should rather get a starting offset,
       so we do not need such a large buffer */
    /* we could also make the bufer static and use mutexes to make
       it thread-safe, because we should not have too many users */
    char data[DCAFS_MAX_STR_LEN];
    switch(db_node_get_type(node)) {
    case db_node_type_int:
        {
            int32_t val = db_node_get_int_value(node);
            size = fmt_s32_dec(data, val);
        }
        break;
    case db_node_type_float:
        {
            float val = db_node_get_float_value(node);
            /* TODO: according to doc this func uses up to 2.4kB code,
               it should become optional in some way. */
            size = fmt_float(data, val, DCAFS_FLOAT_PRECISION);
        }
        break;
    case db_node_type_str:
        size = db_node_get_str_value(node, data, DCAFS_MAX_STR_LEN);
        break;
    default:
        DEBUG("dcafs_read: illegal node type\n");
        return -EFAULT;
    }
    if ((size_t)filp->pos >= size) {
        /* EOF, which is not perfectly true for strings because of the limited buffer */
        /* see todo above */
        return 0;
    }
    if (nbytes > (size - filp->pos)) {
        nbytes = size - filp->pos;
    }
    memcpy(dest, data + filp->pos, nbytes);
    DEBUG("dcafs_read: read %lu bytes\n", (long unsigned)nbytes);
    filp->pos += nbytes;
    return nbytes;
}

static ssize_t dcafs_write(vfs_file_t *filp, const void *src, size_t nbytes)
{
    (void) filp;
    (void) src;
    (void) nbytes;
    return -EBADF;
}

static int dcafs_opendir(vfs_DIR *dirp, const char *dirname, const char *abs_path)
{
    (void) abs_path;
    DEBUG("dcafs_opendir: %p, \"%s\", \"%s\"\n", (void *)dirp, dirname, abs_path);
    int ret;
    db_node_t node;
    ret = db_find_node_by_path(dirname, &node);
    if(ret < 0) {
        DEBUG("dcafs_opendir: Not found :(\n");
        return ret;
    }
    DEBUG("dcafs_opendir: Found :)\n");
    assert(sizeof(db_node_t) <= VFS_DIR_BUFFER_SIZE);
    memset(dirp->private_data.buffer, 0, VFS_DIR_BUFFER_SIZE);
    memcpy(dirp->private_data.buffer, &node, sizeof(db_node_t));
    return 0;
}

static int dcafs_readdir(vfs_DIR *dirp, vfs_dirent_t *entry)
{
    DEBUG("constfs_readdir: %p, %p\n", (void *)dirp, (void *)entry);
    db_node_t *dir = (db_node_t*) dirp->private_data.buffer;
    db_node_t node;
    db_node_get_next_child(dir, &node);
    if (db_node_is_null(&node)) {
        return 0;
    }
    assert(VFS_NAME_MAX > DB_NODE_NAME_MAX);
    node.ops->get_name_fn(&node, entry->d_name);
    entry->d_ino = 0;
    return 1;
}

static int dcafs_closedir(vfs_DIR *dirp)
{
    (void) dirp;
    return 0;
}

static void _dcafs_write_stat(const db_node_t *node, struct stat *restrict buf)
{
    assert(node && !db_node_is_null(node));
    memset(buf, 0, sizeof(*buf));
    buf->st_nlink = 1;
    if(db_node_get_type(node) == db_node_type_inner) {
        buf->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH;
    }
    else {
        buf->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
    }
    buf->st_size = db_node_get_size(node);
    buf->st_blocks = db_node_get_size(node);
    buf->st_blksize = sizeof(uint8_t);
}
