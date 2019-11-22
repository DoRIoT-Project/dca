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
 * @brief DCA "runtime" branch data retrieval functions
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#ifndef DORIOT_DCA_RUNTIME_H
#define DORIOT_DCA_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Return CPU load in percent */
int32_t runtime_get_cpu_load(void);
/** Return number of processes */
int32_t runtime_get_num_processes(void);
/** Return size of .data section in kB */
int32_t runtime_get_data(void);
/** Return size of .text section in kB */
int32_t runtime_get_text(void);
/** Return size of allocated heap space in kB */
int32_t runtime_get_heap(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
