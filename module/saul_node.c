/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

/**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
  * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
  */

#include <doriot_dca/saul_node.h>
#include "saul_reg.h"

int32_t saul_get_num_sensors(void)
{
  saul_reg_t *dev = saul_reg;
  int i = 0;
  while (dev)
  {
    uint8_t cat = dev->driver->type & SAUL_CAT_MASK;
    if (cat == SAUL_CAT_SENSE)
    {
      i++;
    }
    dev = dev->next;
  }
  return i;
}

int32_t saul_get_num_actuators(void)
{
  saul_reg_t *dev = saul_reg;
  int i = 0;
  while (dev)
  {
    uint8_t cat = dev->driver->type & SAUL_CAT_MASK;
    if (cat == SAUL_CAT_ACT)
    {
      i++;
    }
    dev = dev->next;
  }
  return i;
}