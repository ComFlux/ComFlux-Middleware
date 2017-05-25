/*
 * conn_udp.h
 *
 *  Created on: 28Oct 2016
 *      Author: Jie Deng
 */

#ifndef CONN_UDP_H_
#define CONN_UDP_H_

/* init this module */
int udp_init();

/* helper functions to figure out the address passed */
int   udp_get_port(const char *full_address);
char* udp_get_addr(const char *full_address);
int   udp_is_addr(const char *full_address);

/* multithreading helper functions */
void udp_run_receive_thread(int conn);
void* udp_receive_function(void* conn);
void udp_stop_receive_thread(int conn);

/*void  tcp_stop_receive_thread(int conn);  <--not used / implemented */

/* the actual receive data: to be deleted */
char* udp_receive_message(int _conn);



#endif
