/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */
/**
 * @defgroup
 * @ingroup
 * @brief
 * @{
 *
 * @file
 * @brief DCA "saul" branch data retrieval functions
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 */
#ifndef DORIOT_DCA_SAUL_NODE_H
#define DORIOT_DCA_SAUL_NODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return number of sensors*/
int32_t saul_get_num_sensors(void);
/* Return number of actuators */
int32_t saul_get_num_actuators(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
