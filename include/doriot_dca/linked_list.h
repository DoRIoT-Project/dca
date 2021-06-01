/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @defgroup doriot_dca DoRIoT Data Collection Agent
 * @ingroup  doriot
 * @brief
 * @{
 *
 * @file
 * @brief    functions for storing and retrieving data for neighbors
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 */
#ifndef DORIOT_DCA_LINKED_LIST_H
#define DORIOT_DCA_LINKED_LIST_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "net/gnrc/ipv6.h"

#ifdef __cplusplus
extern "C" {
#endif

struct neighbor_entryl
{
    ipv6_addr_t addr;
    uint32_t latency;
    uint32_t packet_loss;
    uint32_t throughput;
    struct neighbor_entryl *next;
};

/*checks if a neighbor exists*/
uint8_t linked_list_node_exists( ipv6_addr_t *ip_cache);
/*inserts a new neighbor*/
void linked_list_insert_node(struct neighbor_entryl *node);
/*updates throughput for an existing neighbor*/
uint8_t linked_list_update_throughput(struct neighbor_entryl *node);
/*updates latency for an existing neighbor*/
uint8_t linked_list_update_latency(struct neighbor_entryl *node);
/*reads latency,packetloss or throughput depending on subfield count*/
uint32_t linked_list_read(uint8_t subfield_count, uint8_t num_neighbours);
/*reads ip address of a neighbor*/
uint8_t linked_list_read_ip(uint8_t num_neighbours,char addr_str[IPV6_ADDR_MAX_STR_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* defined(DORIOT_DCA_LINKED_LIST_H) */
