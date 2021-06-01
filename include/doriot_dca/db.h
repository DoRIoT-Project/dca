/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @defgroup doriot_dca DoRIoT Data Collection Agent
 * @ingroup  doriot
 * @brief    Collects data regarding Network, CPU, QoS resources
 * @{
 *
 * @file
 * @brief    DCA database description
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 */
#ifndef DORIOT_DCA_DB_H
#define DORIOT_DCA_DB_H

#include "doriot_dca/db_fl.h"
#include "doriot_dca/db_node.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** This is the DCA database description */
extern db_fl_entry_t db_index[];

/** Return the number of elements in db_index */
size_t db_get_num_fl_nodes(void);

/** Get a root node instance */
void db_get_root(db_node_t* node);


#ifdef __cplusplus
}
#endif

/** @} */
#endif
