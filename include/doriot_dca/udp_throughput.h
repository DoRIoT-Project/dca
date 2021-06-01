/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

/**
 * @defgroup doriot_dca DoRIoT Data Collection Agent
 * @ingroup  doriot
 * @brief
 * @{
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 */
#ifdef __cplusplus
extern "C" {
#endif

/** gets network throughput for each neighbors */
int db_measure_network_throughput(void);

/** starts server thread on specified port (1883) */
int db_start_udp_server(int port);

#ifdef __cplusplus
}
#endif
