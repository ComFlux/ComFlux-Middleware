/*
 * sync.h
 *
 *  Created on: 1 Apr 2017
 *      Author: Raluca Diaconu
 */

#ifndef SYNC_H_
#define SYNC_H_

/*
 * initialises an ipc socket pair, placing their file descriptors in
 * the given array, and returns a status code with 0 being success
 */
int sync_init(int fds[2]);

void sync_close(int fds[2]);

/*
 * initialises a socket received from a different process
 * e.g.
 * - core receives from the component the app-mw comm channel
 * - component receives from the core a stream pipeline
 */
//int conn_init_fd(int fd);

/* to trigger a sync event between threads send a message on the pipe */
int sync_trigger(int conn, const char *msg);

/* receive an event or a message on a sync pipe */
char* sync_wait(int _conn);


/* wait for an exact message on sync pipe id
 * returns
 * 		-1 if timeout
 * 		1  if message doesn't match
 *  	0  in the ideal case */

#endif /* SYNC_H_ */
