/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */
#include "doriot_dca/linked_list.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

struct neighbor_entryl *head = NULL;
struct neighbor_entryl *current = NULL;

void linked_list_insert_node(struct neighbor_entryl *node)
{
    /*create a link*/
    struct neighbor_entryl *link = (struct neighbor_entryl *)malloc(sizeof(struct neighbor_entryl));
    link = node;
    /*point it to old first node*/
    link->next = head;
    /*point first to new first node*/
    head = link;
}

uint8_t linked_list_read_ip(uint8_t num_neighbours, char *addr_str)
{
    struct neighbor_entryl *ptr = head;
    if (head == NULL)
    {
        return 1;
    }
    for (uint8_t i = 0; i < num_neighbours - 1; i++)
    {
        ptr = ptr->next;
    }
    if (ptr != NULL)
    {
        ipv6_addr_to_str(addr_str, &ptr->addr, IPV6_ADDR_MAX_STR_LEN);
        return 0;
    }
    return 1;
}

uint32_t linked_list_read(uint8_t subfield_count, uint8_t num_neighbours)
{
    struct neighbor_entryl *ptr = head;
    if (head == NULL)
    {
        return 1;
    }
    int32_t value = 1;
    for (uint8_t i = 0; i < num_neighbours - 1; i++)
    {
        ptr = ptr->next;
    }
    /*start from the beginning*/
    if (ptr != NULL)
    {
        switch (subfield_count)
        {
        case 2:
            value = ptr->latency;
            break;
        case 3:
            value = ptr->packet_loss;
            break;
        case 4:
            value = ptr->throughput;
            break;
        default:
            break;
        }
    }
    return value;
}

uint8_t linked_list_node_exists(ipv6_addr_t *ip_cache)
{
    struct neighbor_entryl *ptr = head;
    /*start from the beginning*/
    while (ptr != NULL)
    {
        if (ipv6_addr_equal(&ptr->addr, ip_cache))
        {
            return 0;
        }
        ptr = ptr->next;
    }

    return 1;
}

uint8_t linked_list_update_latency(struct neighbor_entryl *node)
{
    struct neighbor_entryl *ptr = head;
    while (ptr != NULL)
    {
        if (ipv6_addr_equal(&ptr->addr, &node->addr))
        {
            ptr->latency = node->latency;
            ptr->packet_loss = node->packet_loss;
            return 0;
        }
        ptr = ptr->next;
    }
    return 1;
}

uint8_t linked_list_update_throughput(struct neighbor_entryl *node)
{
    struct neighbor_entryl *ptr = head;
    while (ptr != NULL)
    {
        if (ipv6_addr_equal(&ptr->addr, &node->addr))
        {
            ptr->throughput = node->throughput;
            return 0;
        }
        ptr = ptr->next;
    }
    return 1;
}