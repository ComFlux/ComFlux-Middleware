/*
 * com.h
 *
 *  Created on: 15 Mar 2017
 *      Author: Raluca Diaconu
 */

#ifndef COM_TCP_H_
#define COM_TCP_H_

/* helper functions to figure out the address passed */
int   tcp_get_port(const char *full_address);
char* tcp_get_addr(const char *full_address);
int   tcp_is_addr(const char *full_address);

/* multithreading helper functions */
void  tcp_run_accept_thread(int serversock);
void* tcp_accept_function(void* conn);
void  tcp_run_receive_thread(int conn);
void* tcp_receive_function(void* conn);
/*void  tcp_stop_receive_thread(int conn);  <--not used / implemented */

/* the actual receive data: to be deleted */
char* tcp_receive_message(int _conn);

#endif
