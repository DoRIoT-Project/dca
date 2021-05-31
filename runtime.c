/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */
#include <doriot_dca/runtime.h>
#define ENABLE_DEBUG (0)
#include "debug.h"

int32_t runtime_get_cpu_load(void)
{
    int32_t load = 0;
    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; i++)
    {
        thread_t *p = (thread_t *)sched_threads[i];
        if (p != NULL)
        {
            if ((p->status == STATUS_RUNNING) || (p->status == STATUS_PENDING))
            {
                load++;
            }
        }
    }
    return load;
}

float runtime_get_cpu_util(void)
{
    float util = 0;
    uint64_t rt_idle = 0;
    uint64_t rt_sum = 0;
    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; i++)
    {
        thread_t *p = (thread_t *)sched_threads[i];
        if (p != NULL)
        {
            if (!strcmp(p->name, "idle"))
            {
                rt_idle = sched_pidlist[i].runtime_ticks;
            }
            rt_sum += sched_pidlist[i].runtime_ticks;
        }
    }
    util = (1 - (float)rt_idle / rt_sum) * 100;
    DEBUG("\ncpu_utilization:%f\n", util);
    return util;
}

int32_t runtime_get_num_processes(void)
{
    return sched_num_threads;
}
int32_t runtime_get_stack_used(void)
{
    int stack_used = 0;
    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; i++)
    {
        thread_t *p = (thread_t *)sched_threads[i];
        if (p != NULL)
        {
            int stacksz = p->stack_size;
            int stack_free = thread_measure_stack_free(p->stack_start);
            stacksz -= stack_free;
            stack_used += stacksz;
        }
    }
    return stack_used;
}

int32_t runtime_get_heap(void)
{
    /* TODO: should use mechanics of ps */
    return 0;
}
