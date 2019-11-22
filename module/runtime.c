/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */
#include <doriot_dca/runtime.h>

int32_t runtime_get_cpu_load(void) {
    /* TODO: should use mechanics of ps */
    return 1;
}

int32_t runtime_get_num_processes(void) {
    /* TODO: should use mechanics of ps */
    return 5;
}
int32_t runtime_get_data(void) {
    /* TODO: should use mechanics of ps */
    #ifdef BOARD_esp32_wroom_32
    return 17;
    #else
    return 23;
    #endif
}

int32_t runtime_get_text(void) {
    /* TODO: should use mechanics of ps */
    #ifdef BOARD_esp32_wroom_32
    return 420;
    #else
    return 1200;
    #endif
}

int32_t runtime_get_heap(void) {
    /* TODO: should use mechanics of ps */
    return 0;
}
