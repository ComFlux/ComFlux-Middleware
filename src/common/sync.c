/*
 * conn.c
 *
 *  Created on: 1 Apr 2016
 *      Author: Raluca Diaconu
 */

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>

#include <sys/ioctl.h>
#include <netinet/in.h>

#ifdef __unix__
	#include <linux/if.h>
	#include <linux/if_tun.h>
#else
	#include <net/if.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <hashmap.h>
#include "sync.h"
//#include <slog.h>

void* module;
/* functions called in the same thread
 * server: after accept
 * client: init function
 */
void (*on_message_handler)(void*, int, const char*);
void (*on_connect_handler)(void*, int);
void (*on_disconnect_handler)(void*, int);

int sync_init(int fds[2])
{
	sync_close(fds);
	int sockpair_err = socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);
	if(sockpair_err)
	{
		//slog(SLOG_ERROR, "SYNC: init error: %d.", sockpair_err);
		goto final;
	}

	const struct timeval sock_timeout={.tv_sec=5, .tv_usec=0};
	setsockopt(fds[1], SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));
	if(sockpair_err)
	{
		//slog(SLOG_ERROR, "SYNC: init setopt error: %d.", sockpair_err);
		goto final;
	}

	final:{
		return sockpair_err;
	}
}

void sync_close(int fds[2])
{
	if(fds[0]>0)
		close(fds[0]);
	if(fds[1]>0)
		close(fds[1]);
}

unsigned char escape = '#';

char* sync_wait(int _conn)
{
	uint32_t varSize;
	int allBytesRecv; /* sum af all received packets */
	int recvSize; /* one pack received size */
	unsigned char recvEscape;

	char* buf;

	do{
		/* reading size */
		recvSize = recv(_conn , (char*)&varSize, sizeof(uint32_t), 0);
		if(recvSize <= 0)
		{
			//slog(SLOG_WARN, "SYNC wait: Recv size failed from (%d). Code %d. Closing connection ", _conn, recvSize);

/*            if (on_disconnect_handler)
                (*on_disconnect_handler)(NULL, _conn);
            else
                shutdown(_conn,SHUT_RDWR);
*/
			return NULL;
		}
		if (recvSize != sizeof (uint32_t))
		{
			//slog(SLOG_WARN,
			//	 "SYNC wait: error receiving size on sock (%d), val: %d; continue",
			//	 _conn, recvSize);
			continue;
		}

		/* reading escape */
		recvSize = recv(_conn , &recvEscape, 1, 0);
		if(recvEscape != escape)
		{
			//slog(SLOG_WARN,
			//	"SYNC wait: On sock (%d), did not receive correct escape code %c; ignoring size %d",
			//	 _conn, recvEscape, recvSize);
			continue;
		}
		if(recvSize <= 0)
		{
			//slog(SLOG_WARN, "SYNC wait: Recv escape failed from (%d). closing connection ", _conn);

/*            if (on_disconnect_handler)
                (*on_disconnect_handler)(NULL, _conn);
            else
            	shutdown(_conn, SHUT_RDWR);
*/
			return NULL;
		}
		if (recvSize != 1) // Should never get here
		{
			//slog(SLOG_WARN,
			//	 "SYNC wait: error receiving escape on sock (%d), val: %d; continue",
			//	 _conn, recvSize);
			continue;
		}


		/* reading msg */
		allBytesRecv = 0;
		buf = (char*)malloc((varSize+1)*sizeof(char));
		memset(buf, '\0', (varSize+1));

		while(allBytesRecv < varSize)
		{
			recvSize = recv(_conn , buf+allBytesRecv , varSize , 0);
			if(recvSize == -1)
			{
				//slog(SLOG_WARN, "SYNC wait: Recv msg failed from (%d). closing connection ", _conn);

/*	            if (on_disconnect_handler)
	                (*on_disconnect_handler)(NULL, _conn);
	            else
	            	shutdown(_conn,SHUT_RDWR);
*/
				return NULL;
			}
			allBytesRecv += recvSize;
		}

		return buf;

	}while(1);
}

int sync_trigger(int conn, const char *msg)
{
	if(conn <= 0)
	{
		return -1;
	}

	const uint32_t varSize = strlen (msg);
	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */
	int escapeSent;

	allBytesSent = send(conn, &varSize, sizeof (varSize), 0);
	escapeSent = send(conn, &escape, 1, 0);
	if (allBytesSent != sizeof (uint32_t) || escapeSent != 1)
	{
        if (on_disconnect_handler)
            (*on_disconnect_handler)(NULL, conn);
        else
        	shutdown(conn,SHUT_RDWR);

		return -1;
	}

	allBytesSent = 0;
	while(allBytesSent < varSize)
	{
		sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
		/*if(varSize - allBytesSent < 512)
			sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
		else
			sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
		if (sentSize < 0)
		{
			break;
		}
		allBytesSent += sentSize;
	}

	return (int) allBytesSent;
}





pthread_t rcvthread;


/* helper functions */
void * receive_function(void *conn)
{
	int _conn = *((int*)conn);
	if(_conn <= 0)
	{
		return NULL;
	}

	char* buf;

	do{
		/* read message */
		buf = sync_wait(_conn);

		/* recv failed or disconnected */
		if(buf == NULL)
			return NULL;

		/* apply message handler in a different thread */
		if (on_message_handler != NULL)
			(*on_message_handler)(module, _conn, buf);

		free(buf); /* TODO: it can't be passed btw threads */

	}while(1);
	return NULL;
}

void run_receive_thread(int conn)
{
	//if (!initiated) init();
	//pthread_t rcvthread;
	int err;
	int *conn_ptr = (int*)malloc(sizeof(int));
	*conn_ptr = conn;

	//if(on_connect_handler != NULL)
	//	(*on_connect_handler)(module, conn);

	err = pthread_create(&rcvthread, NULL, &receive_function, (void*)conn_ptr);
	if (err != 0)
	{
		//slog(SLOG_ERROR, "CONN sockpair: can't create receive thread  for (%d).", conn);
		return ;
	}
	err = pthread_detach(rcvthread);
	if ( err != 0 )
	{
		//slog(SLOG_ERROR, "CONN sockpair: Could not detach rcv thread for (%d) ", conn);
		return ;
	}

}

