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
 * @brief    Shows saul devices
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 */
#ifndef DORIOT_DCA_SAUL_DEVICES_H
#define DORIOT_DCA_SAUL_DEVICES_H

#include <doriot_dca/db_node.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Get a saul node instance */
void db_new_saul_node(db_node_t* node);


#ifdef __cplusplus
}
#endif

/** @} */
#endif
