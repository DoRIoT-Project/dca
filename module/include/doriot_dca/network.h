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
 * @brief DCA "network" branch data retrieval functions
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#ifndef DORIOT_DCA_NETWORK_H
#define DORIOT_DCA_NETWORK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Return number of network interfaces */
int32_t network_get_num_ifaces(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
