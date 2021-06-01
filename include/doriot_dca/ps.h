/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @defgroup doriot_dca DoRIoT Data Collection Agent
 * @ingroup  doriot
 * @brief
 * @{
 *
 * @file
 * @brief    Shows running processes
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#ifndef DORIOT_DCA_PS_H
#define DORIOT_DCA_PS_H

#include "doriot_dca/db_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Get a ps node instance */
void db_new_ps_node(db_node_t* node);


#ifdef __cplusplus
}
#endif

/** @} */
#endif
