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
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de> 
 */
#ifndef DORIOT_DCA_RUNTIME_H
#define DORIOT_DCA_RUNTIME_H

#include <stdint.h>
#include <thread.h>
#include <sched.h>
#include <string.h>
#include <schedstatistics.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Return CPU load in percent */
int32_t runtime_get_cpu_load(void);
/*Return CPU utilization in percent */
float runtime_get_cpu_util(void);
/** Return number of processes */
int32_t runtime_get_num_processes(void);
/** Return size of total stack used */
int32_t runtime_get_stack_used(void);
/** Return size of allocated heap space in kB */
int32_t runtime_get_heap(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
