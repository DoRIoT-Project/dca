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
 * @brief    Shows running processes
 *
 * @author  Frank Engelhardt <fengelha@ovgu.de>
 * @author  Divya Sasidharan <divya.sasidharan@st.ovgu.de>
 * @author  Adarsh Raghoothaman <adarsh.raghoothaman@st.ovgu.de>
 */
#ifdef __cplusplus
extern "C" {
#endif

/*thread for calculating udp throughput*/
void *_udp_server_thread(void *args);
/* gets network throughput for each neighbors*/
int network_throughput(void);
/*starts server thread on specified port (1883)*/
int udp_server(int port);

#ifdef __cplusplus
}
#endif
