/*
 * sockpair.h
 *
 *  Created on: 15 Mar 2017
 *      Author: Raluca Diaconu
 */

#ifndef COM_SOCKPAIR_H_
#define COM_SOCKPAIR_H_

/* handlers from the MW */
/* maybe should be moved tocom.h? not sure */
void (*on_data_handler)(void*, int, const char*);
void (*on_connect_handler)(void*, int);
void (*on_disconnect_handler)(void*, int);

/* multithreading helper functions */
void  sockpair_run_receive_thread(int conn);
void* sockpair_receive_function(void* conn);


#endif /* COM_SOCKPAIR_H_ */
