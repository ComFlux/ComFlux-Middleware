/*
 * com.h
 *
 *  Created on: 15 Mar 2017
 *      Author: Raluca Diaconu
 */

#ifndef COM_H_
#define COM_H_

/*
 * listens NONBLOCK for connections as a server
 * returns server address
 */
char* com_init(void* module, const char* config_json);

/*
 * initiates a connection as a client to the server
 * returns connection id / socket
 */
int com_connect(const char *addr);

/*
 * close an open connection
 * and THEN call on_disconnect if it has been defined
 */
int com_connection_close(int conn);

/*
 * simple send string data to a connection
 * @conn: connection id / socket
 * @data: data to be transmitted
 */
int com_send(int conn, const void *data, unsigned int size);

/*
 * simple send string data to a connection
 * @conn: connection id / socket
 * @data: data to be transmitted
 */
int com_send_data(int conn, const char *data);


/*
 * register a handler to be called at incoming data
 * the handler is called each time some data is received
 */
int com_set_on_data( void (*handler)(void*,int, const char*) );

/*
 * register a handler to be called when a connection is opened
 * either as a client or server
 */
int com_set_on_connect( void (*handler)(void*,int) );

/*
 * register a handler to be called when a connection is closed
 * i.e., in conn_close, or conn is lost.
 */
int com_set_on_disconnect( void (*handler)(void*,int) );

/*
 * check if an adress is valid for this module
 * @full_address  the address
 */
int com_is_valid_address(const char* full_address);

/**
 * @return 0 if Higher level proto, e.g. REST, and doesn't support HELLO proto
 * 1 if transport layer and the other party implements the protocol, e.g. TCP, UDP, SSL
 */

int com_is_bridge(void);

void (*on_data_handler)(void*, int, const char*);
void (*on_connect_handler)(void*, int);
void (*on_disconnect_handler)(void*, int);

#endif
