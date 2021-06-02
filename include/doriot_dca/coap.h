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

#ifndef DORIOT_DCA_COAP_H
#define DORIOT_DCA_COAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Run CoAP services */
int db_coap_init(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif