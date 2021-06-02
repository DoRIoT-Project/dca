/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */

#include "doriot_dca/network.h"

#include <unistd.h>
#include <stddef.h>

#include "net/netif.h"

int32_t network_get_num_ifaces(void)
{
    netif_t *netif = NULL;
    int32_t res = 0;
    while ((netif = netif_iter(netif)))
    {
        if (netif != NULL)
        {
            res++;
        }
    }
    return res;
}