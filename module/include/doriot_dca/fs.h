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
 * @brief    The DCA file system part (dcafs)
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#ifndef DORIOT_DCA_FS_H
#define DORIOT_DCA_FS_H

#include "vfs.h"

/* vfs_DIR and vfs_file_t have to hold a db_node_t */

#if VFS_FILE_BUFFER_SIZE < 16
#error VFS_FILE_BUFFER_SIZE is too small, at least 16 bytes is required
#endif

#if VFS_DIR_BUFFER_SIZE < 16
#error VFS_DIR_BUFFER_SIZE is too small, at least 16 bytes is required
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum size of a file */
#define DCAFS_MAX_STR_LEN 256
/** Number of digits after decimal point for float values */
#define DCAFS_FLOAT_PRECISION 7

/**
 * @brief dcafs file system driver
 *
 * For use with vfs_mount
 */
extern const vfs_file_system_t dcafs_file_system;

#ifdef __cplusplus
}
#endif

/** @} */
#endif
