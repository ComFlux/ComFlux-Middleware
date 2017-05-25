/*
 * conn_fifo.c
 *
 *  Created on: 15 Feb 2017
 *      Author: Raluca Diaconu
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <hashmap.h>
#include <slog.h>
#include <errno.h>
#include <stdio.h> /* for removing fifo */

#include "conn_fifo.h"

HashMap *ids;
void* _module;
int initiated = 0;

HashMap* stream_handlers;

void (*on_message_handler)(void*,int, const char*);
void (*on_connect_handler)(void*,int);
void (*on_disconnect_handler)(void*,int);


int fifo_init(void* module, const char* filename)
{
	ids = map_new(KEY_TYPE_STR);
	_module = module;

	on_message_handler = NULL;
	on_connect_handler = NULL;
	on_disconnect_handler = NULL;
	return 0;
}


int fifo_init_server(const char *fifo_name)
{
	if (!initiated)
		fifo_init(NULL, NULL);
		//return -1;
	remove(fifo_name);
	int i = mkfifo(fifo_name, S_IWUSR | S_IRUSR |
		    S_IRGRP | S_IROTH);
	int conn = open(fifo_name, O_RDWR | O_NONBLOCK);
	return conn;
}


int fifo_init_client(const char *fifo_name)
{
	if (!initiated)
		fifo_init(NULL, NULL);
		//return -1;
	int conn = open(fifo_name, O_RDONLY | O_NONBLOCK);

	if (conn <= 0)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"CONN FIFO: Error opening fifo %s: errno: %d.",
				fifo_name, conn);
		return conn;
	}
	return conn;
}

int fifo_send_message(int conn, const char* msg)
{
    unsigned char escape = '#';
    if (conn <= 0) {
        slog(SLOG_ERROR, SLOG_ERROR, "CONN FIFO: (%d) not opened, can't send msg *%s*", conn, msg);
        return -1;
    }
    slog(SLOG_INFO, SLOG_INFO, "CONN FIFO: send to (%d) *%s*", conn, msg);

    const uint32_t varSize = strlen(msg);
    int allBytesSent; /* sum of all sent sizes */
    ssize_t sentSize; /* one shot sent size */
    int escapeSent;

    allBytesSent = write(conn, &varSize, sizeof(varSize));
    escapeSent = write(conn, &escape, 1);
    if (allBytesSent != sizeof(uint32_t) || escapeSent != 1) {
        slog(SLOG_ERROR, SLOG_ERROR, "CONN FIFO: error sending size on fifo (%d): error %d", conn, escapeSent);
        // fifo_close(conn);
        if (on_disconnect_handler)
            (*on_disconnect_handler)(_module, conn);
        else
            fifo_close(conn);
        return -1;
    }

    allBytesSent = 0;
    while (allBytesSent < varSize) {
        sentSize = write(conn, msg + allBytesSent, varSize - allBytesSent);
        /*if(varSize - allBytesSent < 512)
                sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
        else
                sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
        if (sentSize < 0) {
            slog(SLOG_ERROR, SLOG_ERROR, "CONN FIFO: error sending msg on fifo (%d)", conn);
            break;
        }
        allBytesSent += sentSize;
    }
    return (int)allBytesSent;
}

char* fifo_receive_message(int _conn)
{
    unsigned char escape = '#';
    uint32_t varSize;
    int allBytesRecv; /* sum af all received packets */
    int recvSize;     /* one pack received size */
    unsigned char recvEscape;
    char* buf;

    do {
        /* reading size */
    	//read(_conn, buf, MAX_BUF);
		recvSize = read(_conn, (char*)&varSize, sizeof(uint32_t));
		if (recvSize <= 0) {
			slog(SLOG_WARN, SLOG_WARN, "CONN FIFO: Recv size failed from (%d). closing connection ", _conn);
			// fifo_close(_conn);
			if (on_disconnect_handler)
				(*on_disconnect_handler)(_module,_conn);
			else
				fifo_close(_conn);
			return NULL;
		}
		if (recvSize != sizeof(uint32_t)) {
			slog(SLOG_WARN, SLOG_WARN, "CONN: error receiving size on fifo (%d), val: %d; continue", _conn, recvSize);
			continue;
		}
        /* reading escape */
        recvSize = read(_conn, &recvEscape, 1);
        if (recvEscape != escape) {
            slog(SLOG_WARN,
                 SLOG_WARN,
                 "CONN FIFO: On (%d), did not receive correct escape code %c; ignoring size %d",
                 _conn,
                 recvEscape,
                 recvSize);
            continue;
        }
        if (recvSize <= 0) {
            slog(SLOG_WARN, SLOG_WARN, "CONN FIFO: Recv escape failed from (%d). closing connection ", _conn);
            // fifo_close(_conn);
            if (on_disconnect_handler)
                (*on_disconnect_handler)(_module,_conn);
            else
                fifo_close(_conn);
            return NULL;
        }
        if (recvSize != 1) // Should never get here
        {
            slog(SLOG_WARN, SLOG_WARN, "CONN FIFO: error receiving escape on fifo (%d), val: %d; continue", _conn, recvSize);
            continue;
        }

        slog(SLOG_INFO, SLOG_INFO, "CONN FIFO: Expecting %d bytes on fifo (%d).", varSize, _conn);

        /* reading msg */
        allBytesRecv = 0;
        buf = (char*)malloc((varSize + 1) * sizeof(char));
        memset(buf, '\0', (varSize + 1));

        while (allBytesRecv < varSize) {
            recvSize = read(_conn, buf + allBytesRecv, varSize);
            if (recvSize == -1) {
                slog(SLOG_WARN, SLOG_WARN, "CONN FIFO: Recv msg failed from (%d). closing connection ", _conn);
                // fifo_close(_conn);
                if (on_disconnect_handler)
                    (*on_disconnect_handler)(_module,_conn);
                else
                    fifo_close(_conn);
                return NULL;
            }
            allBytesRecv += recvSize;
        }

        slog(SLOG_INFO, SLOG_INFO, "CONN FIFO: received %d total bytes on sock (%d): *%s*", allBytesRecv, _conn, buf);
        return buf;
    } while (1);
}

void* fifo_receive_function(void* conn)
{
    int _conn = *((int*)conn);
    if (_conn <= 0) {
        slog(SLOG_ERROR, SLOG_ERROR, "CONN: not established with (%d), can't recv", conn);
        return NULL;
    }
    char* buf;
    do {
        /* read message */
        buf = fifo_receive_message(_conn);

        /* recv failed or disconnected */
        if (buf == NULL)
            return NULL;

        /* apply message handler in a different thread */
        if (on_message_handler != NULL)
            (*on_message_handler)(_module, _conn, buf);

        free(buf); /* TODO: it can't be passed btw threads */

    } while (1);
    return NULL;
}

void fifo_run_receive_thread(int conn)
{
	//if (!initiated) init();
	pthread_t rcvthread;
	int err;
	int *conn_ptr = (int*)malloc(sizeof(int));
	*conn_ptr = conn;

	//if(on_connect_handler != NULL)
	//	(*on_connect_handler)(module, conn);

	err = pthread_create(&rcvthread, NULL, &fifo_receive_function, (void*)conn_ptr);
	if (err != 0)
	{
		slog(1, SLOG_ERROR, "CONN sockpair: can't create receive thread  for (%d).", conn);
		return ;
	}
	err = pthread_detach(rcvthread);
	if ( err != 0 )
	{
		slog(1, SLOG_ERROR, "CONN sockpair: Could not detach rcv thread for (%d) ", conn);
		return ;
	}
	slog(4, SLOG_INFO, "CONN sockpair: Receive thread created successfully for (%d).", *conn_ptr);

}

int fifo_close(int conn)
{
	return close(conn);
}

int fifo_set_on_message(void (*handler)(void*,int, const char*))
{
    on_message_handler = handler;
    return 0;
}

int fifo_set_on_connect(void (*handler)(void*,int))
{
    on_connect_handler = handler;
    return 0;
}

int fifo_set_on_disconnect(void (*handler)(void*,int))
{
    on_disconnect_handler = handler;
    return 0;
}
