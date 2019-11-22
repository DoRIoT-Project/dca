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

int32_t runtime_get_cpu_load(void);
int32_t runtime_get_num_processes(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
