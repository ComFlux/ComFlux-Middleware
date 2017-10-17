/*
 * conn_rest.h
 *
 *  Created on: Mar 15, 2017
 *      Author: jie
 */

#ifndef MODULES_COM_MODULES_REST_CONN_REST_H_
#define MODULES_COM_MODULES_REST_CONN_REST_H_

char* load_base_module(const char* lib_path, const char* cfgfile);
void (*on_data_handler)(void*, int, const void*, unsigned int);
void (*on_connect_handler)(void*, int);
void (*on_disconnect_handler)(void*, int);

void on_message_for_base(void*, int, const void*, unsigned int);
void on_connect_for_base(void*, int);
void on_disconnect_for_base(void*, int);

/* helper functions to figure out the address passed */
int   tcp_get_port(const char *full_address);
char* tcp_get_addr(const char *full_address);
int   tcp_is_addr(const char *full_address);

/* multithreading helper functions */
void  tcp_run_accept_thread(int serversock);
void* tcp_accept_function(void* conn);
void  tcp_run_receive_thread(int conn);
void* tcp_receive_function(void* conn);

char* tcp_receive_message(int _conn);

/* rest structure  for multithreading*/
struct _rest_peer__;

struct _rest_peer__* rest_new(const char* host, int fd, int client);
void rest_free(struct _rest_peer__* peer);
void rest_run_listen_thread(struct _rest_peer__ *channel);

#endif /* MODULES_COM_MODULES_REST_CONN_REST_H_ */
