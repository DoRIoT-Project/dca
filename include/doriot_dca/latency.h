/**
 * @file latency.h
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 * @brief
 * @version 0.1
 * @date 2020-12-13
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef DORIOT_DCA_LATENCY_H
#define DORIOT_DCA_LATENCY_H

#ifdef __cplusplus
extern "C" {
#endif

#define _SEND_NEXT_PING (0xEF48)
#define _PING_FINISH (0xEF49)
#define CKTAB_SIZE (64U * 8) /* 64 byte * 8 bit/byte */
#define DEFAULT_COUNT (3U)
#define DEFAULT_DATALEN (sizeof(uint32_t))
#define DEFAULT_ID (0x53)
#define DEFAULT_INTERVAL_USEC (1U * US_PER_SEC)
#define DEFAULT_TIMEOUT_USEC (1U * US_PER_SEC)

/** gets network latency and packetloss for each neighbors */
int db_measure_network_latency(void);

#ifdef __cplusplus
}
#endif

#endif
