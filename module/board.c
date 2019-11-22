/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */

#include <doriot_dca/board.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

size_t board_get_name(char* strbuf, size_t bufsize) {
    strncpy(strbuf, RIOT_BOARD, bufsize);
    return strlen(strbuf);
}

size_t board_get_mcu(char* strbuf, size_t bufsize) {
    strncpy(strbuf, RIOT_MCU, bufsize);
    return strlen(strbuf);
}

int board_get_ram(void) {
    return 16536; // stonks!
}

int board_get_clock(void) {
    return 20 * 1024 * 1024;
}

int board_get_nonvolatile(void) {
    return 120 * 1024;
}
