/*
 * sockpair.c
 *
 *  Created on: 15 Mar 2017
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

#include <slog.h>
#include <hashmap.h>

#include "com.h"
#include "sockpair.h"

#include <json.h>

void* thismodule;


/* file descriptors for the sockpair */
int fds[2];

/* the party that initiates (api) : 0
 * the one that connects (core): 1
 */
int my_conn = -1;

int recv_threads_runs = 0;

/* address */
char* address =NULL;

/* functions called in the same thread
 * server: after accept
 * client: init function
 */

/* com.h functions */

/* init creates the sockpair  and returns error */
char* com_init(void* module, const char* config_json)
{
	/* parse configs */
    JSON* args_json = json_new(config_json);
    int is_server = json_get_int(args_json, "is_server");

    /* set the handle */
    thismodule = module;

	if(is_server)
	{
		my_conn = 1;
		socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);
		if (!recv_threads_runs)
			sockpair_run_receive_thread(fds[my_conn]);
	}
	else
	{
		my_conn = 0;
		fds[0] = json_get_int(args_json, "fd");

		if (!recv_threads_runs)
			sockpair_run_receive_thread(fds[my_conn]);
	}

	json_free(args_json);
	address = (char*) malloc(10*sizeof(char));
	sprintf(address, "%d:%d",my_conn, fds[my_conn]);
	return address;
}
/*
 * this function is not implemented because it is 1-1
 */
int com_connect(const char *addr)
{
	/* return the other one */
	if(addr == NULL)
		return fds[my_conn];
	else
		return fds[(my_conn+1)%2];
}

int com_connection_close(int conn)
{
	if(conn == fds[0] || conn == fds[1])
		return close(conn);
	else
		return -1;
}


/* alt functions using escape char */
unsigned char escape = '#';

int com_send_data(int conn, const char* msg)
{
	if(conn <= 0)
	{
		slog(SLOG_ERROR,
			 "SOCKPAIR: not established with (%d), can't send msg *%s*",
			 conn, msg);
		return -1;
	}
	slog(SLOG_INFO, "SOCKPAIR: send to (%d) *%s*", conn, msg);

	const uint32_t varSize = strlen (msg);
	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */
	int escapeSent;

	allBytesSent = send(conn, &varSize, sizeof (varSize), 0);
	escapeSent = send(conn, &escape, 1, 0);
	if (allBytesSent != sizeof (uint32_t) || escapeSent != 1)
	{
		slog(SLOG_ERROR,
			 "SOCKPAIR: error sending size on sock (%d): %s",
			 conn, strerror(errno));

        if (on_disconnect_handler)
            (*on_disconnect_handler)(NULL, conn);
        else
        	close(conn);

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
			slog(SLOG_ERROR,
				 "SOCKPAIR: error sending msg on sock (%d)",
				 conn);
			break;
		}
		allBytesSent += sentSize;
	}

	return (int) allBytesSent;
}


int com_send(int conn, const void* msg, unsigned int size)
{
	if(conn <= 0)
	{
		slog(SLOG_ERROR,
			 "SOCKPAIR: not established with (%d), can't send msg *%s*",
			 conn, msg);
		return -1;
	}
	slog(SLOG_INFO, "SOCKPAIR: send to (%d) *%s*", conn, msg);
	//printf("SOCKPAIR: send to (%d) %d, *%*s*\n", conn, size, size, msg);

	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */

	allBytesSent = 0;
	while(allBytesSent < size)
	{
		sentSize = send(conn , msg+allBytesSent , size - allBytesSent , 0);

		if (sentSize < 0)
		{
			slog(SLOG_ERROR,
				 "SOCKPAIR: error sending msg on sock (%d)",
				 conn);
			break;
		}
		allBytesSent += sentSize;
	}

	return (int) allBytesSent;
}

char* sockpair_receive_message(int _conn)
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
			slog(SLOG_WARN, "SOCKPAIR: Recv size failed from (%d). Code %d. Closing connection ", _conn, recvSize);

            if (on_disconnect_handler)
                (*on_disconnect_handler)(NULL, _conn);
            else
            	close(_conn);

			return NULL;
		}
		if (recvSize != sizeof (uint32_t))
		{
			slog(SLOG_WARN,
				 "SOCKPAIR: error receiving size on sock (%d), val: %d; continue",
				 _conn, recvSize);
			continue;
		}

		/* reading escape */
		recvSize = recv(_conn , &recvEscape, 1, 0);
		if(recvEscape != escape)
		{
			slog(SLOG_WARN,
				"SOCKPAIR: On sock (%d), did not receive correct escape code %c; ignoring size %d",
				 _conn, recvEscape, recvSize);
			continue;
		}
		if(recvSize <= 0)
		{
			slog(SLOG_WARN, "SOCKPAIR: Recv escape failed from (%d). closing connection ", _conn);

            if (on_disconnect_handler)
                (*on_disconnect_handler)(NULL, _conn);
            else
            	close(_conn);

			return NULL;
		}
		if (recvSize != 1) // Should never get here
		{
			slog(SLOG_WARN,
				 "SOCKPAIR: error receiving escape on sock (%d), val: %d; continue",
				 _conn, recvSize);
			continue;
		}

		slog(SLOG_INFO, "SOCKPAIR: Expecting %d bytes on sock (%d).", varSize, _conn);

		/* reading msg */
		allBytesRecv = 0;
		buf = (char*)malloc((varSize+1)*sizeof(char));
		memset(buf, '\0', (varSize+1));

		while(allBytesRecv < varSize)
		{
			recvSize = recv(_conn , buf+allBytesRecv , varSize , 0);
			if(recvSize == -1)
			{
				slog(SLOG_WARN, "SOCKPAIR: Recv msg failed from (%d). closing connection ", _conn);

	            if (on_disconnect_handler)
	                (*on_disconnect_handler)(NULL, _conn);
	            else
	            	close(_conn);

				return NULL;
			}
			allBytesRecv += recvSize;
		}

		slog(SLOG_INFO, "SOCKPAIR: received %d total bytes on sock (%d): *%s*", allBytesRecv, _conn, buf);
		return buf;

	}while(1);
}

char* sockpair_receive_message2(int _conn, void* buf, int* size)
{
	uint32_t varSize;


	memset(buf, '\0', *size);

		/* reading escape */

	*size = recv(_conn , buf , 50 , 0);
	if(*size == -1)
	{
		slog(SLOG_WARN, "SOCKPAIR: Recv msg failed from (%d). closing connection ", _conn);

		if (on_disconnect_handler)
			(*on_disconnect_handler)(NULL, _conn);
		else
			close(_conn);

		return NULL;
	}

	slog(SLOG_INFO, "SOCKPAIR: received %d total bytes on sock (%d): *%s*", *size, _conn, buf);
	return buf;
}


//////
int com_send_data_alt(int conn, const char* msg)
{
    if (conn <= 0) {
        slog(SLOG_ERROR, "SOCKPAIR: connection not established with (%d), can't send msg *%s*", conn, msg);
        return -1;
    }
    slog(SLOG_INFO, "SOCKPAIR: send to (%d) *%s*\n", conn, msg);
    //printf("SOCKPAIR: send to (%d) *%s*\n", conn, msg);

    const uint32_t varSize = strlen(msg);
    int allBytesSent; /* sum of all sent sizes */
    ssize_t sentSize; /* one shot sent size */
    slog(SLOG_INFO, "SOCKPAIR: About to send message using tcp on socket (%d)", conn);

    allBytesSent = 0;
    while (allBytesSent < varSize)
    {
        sentSize = send(conn, msg + allBytesSent, varSize - allBytesSent, 0);
        /*if(varSize - allBytesSent < 512)
                sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
        else
                sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
        if (sentSize < 0)
        {
        	slog(SLOG_ERROR, "SOCKPAIR: error sending msg on sock (%d)", conn);
        	break;
        }
        allBytesSent += sentSize;
    }
    return (int)allBytesSent;
}

int com_set_on_data( void (*handler)(void*, int, const void*, unsigned int) )
{
    on_data_handler = handler;
    return (on_data_handler != NULL);
}


int com_set_on_connect( void (*handler)(void*, int) )
{
    on_connect_handler = handler;
    return (on_connect_handler != NULL);
}

int com_set_on_disconnect( void (*handler)(void*, int) )
{
    on_disconnect_handler = handler;
    return (on_disconnect_handler != NULL );
}

int com_is_valid_address(const char* full_address)
{
	//TODO
	return 0;
}

int com_is_bridge()
{
	return 1;
}

char* sockpair_receive_message_alt(int _conn)
{
    uint32_t varSize;
    //int allBytesRecv; /* sum af all received packets */
    int recvSize;     /* one pack received size */
    unsigned char recvEscape;
    char* buf;
    slog(SLOG_INFO, "SOCKPAIR: About to receive message using tcp on socket (%d)", _conn);

        /* reading msg */
        //allBytesRecv = 0;
        buf = (char*)malloc((512 + 1) * sizeof(char));
        memset(buf, '\0', (512 + 1));
        recvSize = recv(_conn, buf, 512, 0);
        if (recvSize <= 0) {
            slog(SLOG_WARN, "SOCKPAIR: Recv msg failed from (%d). closing connection ", _conn);
            // connection_close(_conn);
            if (on_disconnect_handler)
                (*on_disconnect_handler)(thismodule,_conn);
            else
                com_connection_close(_conn);
            return NULL;
        }
        //buf[recvSize] = '\n';
        slog(SLOG_INFO, "SOCKPAIR: received %d total bytes on sock (%d): *%s*", recvSize, _conn, buf);

        return buf;
}

/* sockpair.h functions */

void* sockpair_receive_function(void *conn)
{
	int _conn = *((int*)conn);
	free(conn);
	if(_conn <= 0)
	{
		slog(SLOG_ERROR,
			 "SOCKPAIR: not established with (%d), can't recv",
			 conn);
		return NULL;
	}

	char* buf;

	do{
		/* read message */
		void* buf = (char*)malloc(600*sizeof(char));
		int size = 600;
		sockpair_receive_message2(_conn, buf, &size);
		//printf("SOCKPAIR: received (%d) %d *%*s*\n", _conn, size, size, buf);

		/* recv failed or disconnected */
		if(buf == NULL)
			return NULL;

		/* apply message handler in a different thread */
		if (on_data_handler != NULL)
			(*on_data_handler)(thismodule, _conn, buf, (unsigned int)size);


	}while(1);
	return NULL;
}

void  sockpair_run_receive_thread(int conn)
{
	pthread_t rcvthread;

	int err;
	int *conn_ptr = (int*)malloc(sizeof(int));
	*conn_ptr = conn;

	//if(on_connect_handler != NULL)
	//	(*on_connect_handler)(thismodule, conn);

	err = pthread_create(&rcvthread, NULL, &sockpair_receive_function, (void*)conn_ptr);
	if (err != 0)
	{
		slog(SLOG_ERROR, "SOCKPAIR: can't create receive thread  for (%d).", conn);
		return ;
	}
	err = pthread_detach(rcvthread);
	if ( err != 0 )
	{
		slog(SLOG_ERROR, "SOCKPAIR: Could not detach rcv thread for (%d) ", conn);
		return ;
	}
	slog(SLOG_INFO, "SOCKPAIR: Receive thread created successfully for (%d).", *conn_ptr);
	recv_threads_runs = 1;
}

