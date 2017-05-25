/*
 * conn_fifo.h
 *
 *  Created on: 15 Feb 2017
 *      Author: Raluca Diaconu
 */

#ifndef SRC_COMMON_CONN_FIFO_H_
#define SRC_COMMON_CONN_FIFO_H_

/* fifo ipc communication */



/*
 * initialises an ipc fifo, placing their file descriptors in
 * the given array, and returns a status code with 0 being success
 */
int fifo_init(void* module, const char* filename);

/*
 * initializes an ipc fifo with the given name
 * opens it RW and listens for messages on it
 * returns an integer id
 */
int fifo_init_server(const char *fifo_name);

/*
 * opens the ipc fifo RW with the given name
 * returns the id of the connection, 0 / -1 on error
 */
int fifo_init_client(const char *fifo_name);

/*
 * simple send string message to a connection
 * @conn: connection id / socket
 * @msg: message to be transmitted
 */
int fifo_send_message(int conn, const char *msg);

char* fifo_receive_message(int conn);

/*
 * register a handler to be called at incoming messages
 */
int fifo_set_on_message( void (*handler)(void*, int, const char*) );
/*
 * register a handler to be called when a connection is opened
 * either as a client or server
 */
int fifo_set_on_connect( void (*handler)(void*, int) );
/*
 * register a handler to be called when a connection is closed
 * i.e., in conn_close, or conn is lost.
 */
int fifo_set_on_disconnect( void (*handler)(void*, int) );


void fifo_run_receive_thread(int conn);

/*
 * closes the fifo and destroys it
 */
int fifo_close(int conn);


#endif /* SRC_COMMON_CONN_FIFO_H_ */
