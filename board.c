/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */

#include "doriot_dca/board.h"

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define Hz (1)
#define kHz (1000)
#define MHz (1000*1000)
#define GHz (1000*1000*1000)

#define B (1)
#define kB (1024)
#define MB (1024*1024)
#define GB (1024*1024*1024)

/* 
    Rules:
    - BOARD_xxx, where xxx is the board name, converted to capital leters
    - MCU_xxx, where xxx is the mcu name, converted to capital leters
*/

#ifndef DCA_MCU_RAM
    #if defined(MCU_ESP32)
        #define DCA_MCU_RAM 2*160*kB
    #elif defined(MCU_NRF52)
        #define DCA_MCU_RAM 32*kB
    #else
        #define DCA_MCU_RAM 0*B
    #endif
#endif

#ifndef DCA_MCU_NVRAM
    #if defined(MCU_ESP32)
        #define DCA_MCU_NVRAM 0*B
    #elif defined(MCU_NRF52)
        #define DCA_MCU_NVRAM 192*kB
    #else
        #define DCA_MCU_NVRAM 0*B
    #endif
#endif

#ifndef DCA_MCU_CLOCK
    #if defined(MCU_ESP32)
        #define DCA_MCU_CLOCK 240*MHz
    #elif defined(MCU_NRF52)
        #define DCA_MCU_CLOCK 64*MHz
    #elif defined(MCU_STM32)
        #define DCA_MCU_CLOCK 32768*kHz
    #else
        #define DCA_MCU_CLOCK 0*Hz
    #endif
#endif

#ifndef DCA_BOARD_RAM
    #if defined(BOARD_ESP32_WROOM_32)
        #define DCA_BOARD_RAM 0*kB
    #elif defined(BOARD_NUCLEO_F429ZI)
        #define DCA_BOARD_RAM 256*kB
    #else
        #define DCA_BOARD_RAM 0*B
    #endif
#endif

#ifndef DCA_BOARD_NVRAM
    #if defined(BOARD_ESP32_WROOM_32)
        #define DCA_BOARD_NVRAM 16*kB
    #elif defined(BOARD_NUCLEO_F429ZI)
        #define DCA_BOARD_NVRAM 2*MB
    #else
        #define DCA_BOARD_NVRAM 0*B
    #endif
#endif

#ifndef DCA_EXTRA_NVRAM
    #define DCA_EXTRA_NVRAM 0*B
#endif

#ifndef DCA_EXTRA_RAM
    #define DCA_EXTRA_RAM 0*B
#endif

size_t board_get_name(char* strbuf, size_t bufsize) {
    strncpy(strbuf, RIOT_BOARD, bufsize);
    return strlen(strbuf);
}

size_t board_get_mcu(char* strbuf, size_t bufsize) {
    strncpy(strbuf, RIOT_MCU, bufsize);
    return strlen(strbuf);
}

int32_t board_get_ram(void) {
    return DCA_MCU_RAM + DCA_BOARD_RAM + DCA_EXTRA_RAM;
}

int32_t board_get_clock(void) {
    return DCA_MCU_CLOCK;
}

int32_t board_get_nonvolatile(void) {
    return DCA_MCU_NVRAM + DCA_BOARD_NVRAM + DCA_EXTRA_NVRAM;
}
