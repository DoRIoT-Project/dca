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
 * @brief DCA "board" branch data retrieval functions
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#ifndef DORIOT_DCA_BOARD_H
#define DORIOT_DCA_BOARD_H

#include <doriot_dca/db.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t board_get_name(char* strbuf, size_t bufsize);
size_t board_get_mcu(char* strbuf, size_t bufsize);
int32_t board_get_ram(void);
int32_t board_get_clock(void);
int32_t board_get_nonvolatile(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
