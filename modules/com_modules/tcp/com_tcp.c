/*
 * conn_tcp.c
 *
 *  Created on: 20Oct 2016
 *      Author: Jie Deng
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


#include "com.h"
#include "com_tcp.h"
#include "hashmap.h"
//#include <slog.h>
#include <json.h>

void* thismodule = NULL;

/* com header implems */

char* com_init(void* module, const char* config_json)
{
	/* set the handle */
    thismodule = module;

    /* parse the json args */
    JSON* args_json = json_new(config_json);
    char* server_address = json_get_str(args_json, "address");
    json_free(args_json);

    /* init listen server on the specified address */
    char* server_addr = tcp_get_addr(server_address);
    int server_port = tcp_get_port(server_address);
    if (server_port == -1)
    {
        server_port = 0;
    }

    int serversock;              //, peersock;
    struct sockaddr_in serverin; //, peerin;

    serversock = socket(AF_INET, SOCK_STREAM, 0);
    if (serversock == -1)
    {
        return NULL;
    }

    serverin.sin_family = AF_INET;
    serverin.sin_addr.s_addr = INADDR_ANY;
    serverin.sin_port = htons(server_port);

    if (bind(serversock, (struct sockaddr*)&serverin, sizeof(serverin)) < 0)
    {
        free(server_addr);
        return NULL;
    }

    free(server_addr);
    listen(serversock, 3);
    
    tcp_run_accept_thread(serversock);

    return server_address;
}

int com_connect(const char* server_address)
{
    if (!tcp_is_addr(server_address))
    {
       return -1;
    }

    char* server_addr = tcp_get_addr(server_address);
    int server_port = tcp_get_port(server_address);

    int peersock;
    struct sockaddr_in peerin;

    peersock = socket(AF_INET, SOCK_STREAM, 0);
    if (peersock == -1)
    {
        return -2;
    }

    peerin.sin_addr.s_addr = inet_addr(server_addr);
    peerin.sin_family = AF_INET;
    peerin.sin_port = htons(server_port);

    if (connect(peersock, (struct sockaddr*)&peerin, sizeof(peerin)) < 0)
    {
        free(server_addr);
        return -3;
    }
    free(server_addr);
    tcp_run_receive_thread(peersock);

    return peersock;
}

int com_connection_close(int conn)
{
    return close(conn);
}


int com_send(int conn, const void *data, unsigned int size)
{
    if (conn <= 0) {
        return -1;
    }

    int allBytesSent; /* sum of all sent sizes */
    ssize_t sentSize; /* one shot sent size */

    allBytesSent = 0;
    while (allBytesSent < size)
    {
        sentSize = send(conn, data + allBytesSent, size - allBytesSent, 0);
        /*if(varSize - allBytesSent < 512)
                sentSize = send(conn , data+allBytesSent , varSize - allBytesSent , 0);
        else
                sentSize = send(conn , data+allBytesSent , 512 , 0);*/
        if (sentSize < 0)
        {
        	break;
        }
        allBytesSent += sentSize;
    }
    return (int)allBytesSent;
}

int com_send_data(int conn, const char* msg)
{
    if (conn <= 0) {
        return -1;
    }

    const uint32_t varSize = strlen(msg);
    return (int)com_send(conn, (void*)msg, varSize);
}

int com_set_on_data( void (*handler)(void*, int, const char*) )
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
	return tcp_is_addr(full_address);
}

int com_is_bridge(void)
{
	return 1;
}

/* com_tcp.h functions */

int tcp_is_addr(const char* full_address)
{
	char* addr = tcp_get_addr(full_address);
	int port = tcp_get_port(full_address);

	if (addr == NULL || port < 0)
		return 0;

	/* checks the addr */
	struct sockaddr_in sa;
	int is_addr = inet_pton(AF_INET, addr, &(sa.sin_addr));

	/* checks the port */
	int is_port = (port > 1 && port <= 65535);

	free(addr);

	return is_addr && is_port;
}


char *tcp_get_addr(const char * full_address)
{
	if (full_address == NULL)
		return NULL;

	char *addr = strdup(full_address);
	char *pos = strchr(addr,':'); //strpbrk

	if (pos == NULL)
	{
		free(addr);
		return NULL;
	}

	pos[0] = '\0';

	return addr;
}

int tcp_get_port(const char * full_address)
{
	int port;

    if (full_address == NULL)
        return -1;

	char *addr = strdup(full_address);
	char *pos = strchr(addr,':'); //strpbrk

	if (pos == NULL)
		port = 0;

	int scanned = sscanf(pos+1, "%d", &port);

	free(addr);
	if (scanned < 1)
		port = 0;

	return port;
}

char* tcp_receive_message(int _conn)
{
    uint32_t varSize;
    //int allBytesRecv; /* sum af all received packets */
    int recvSize;     /* one pack received size */
    unsigned char recvEscape;
    char* buf;
    //slog(SLOG_INFO, "TCP: About to receive message using tcp on socket (%d)", _conn);

        /* reading msg */
        //allBytesRecv = 0;
        buf = (char*)malloc((512 + 1) * sizeof(char));
        memset(buf, '\0', (512 + 1));
        recvSize = recv(_conn, buf, 512, 0);
        if (recvSize <= 0) {
            //slog(SLOG_WARN, "TCP: Recv msg failed from (%d). closing connection ", _conn);
            // connection_close(_conn);
            if (on_disconnect_handler)
                (*on_disconnect_handler)(thismodule,_conn);
            else
                com_connection_close(_conn);
            return NULL;
        }

        //slog(SLOG_INFO, "TCP: received %d total bytes on sock (%d): *%s*", recvSize, _conn, buf);

        return buf;
}


void tcp_run_accept_thread(int serversock)
{
    int err;
    int* serversock_ptr = (int*)malloc(sizeof(int));
    *serversock_ptr = serversock;

    pthread_t listenthread;
    err = pthread_create(&listenthread, NULL, &tcp_accept_function, (void*)serversock_ptr);
    if (err != 0) {
        //slog(SLOG_ERROR, "TCP: can't create listen thread"); //: %s", strerror(err));
        return;
    }
    err = pthread_detach(listenthread);
    if (err != 0)
    {
        return;
    }

}

void* tcp_accept_function(void* serversock)
{
	int _serversock = *((int*)serversock);
    free(serversock);

    int peersock;
    struct sockaddr_in peerin;

    // accept connection from an incoming client
    int c = sizeof(struct sockaddr_in);
    while (1) {
        peersock = accept(_serversock, (struct sockaddr*)&peerin, (socklen_t*)&c);
        if (peersock < 0) {
            //slog(SLOG_ERROR, "TCP: accept failed for (%d).", *((int*)serversock));
            return NULL; // TODO
        }
        //slog(SLOG_INFO, "TCP: connection accpeted by socket (%d) ", peersock);
        tcp_run_receive_thread(peersock);
    }
    return NULL;
}

void tcp_run_receive_thread(int conn)
{
    pthread_t rcvthread;
    int err;
    int* conn_ptr = (int*)malloc(sizeof(int));
    *conn_ptr = conn;

    if (on_connect_handler != NULL)
        (*on_connect_handler)(thismodule,conn);

    err = pthread_create(&rcvthread, NULL, &tcp_receive_function, (void*)conn_ptr);
    if (err != 0) {
        //slog(SLOG_ERROR, "TCP: can't create receive thread  for (%d).", conn);
        return;
    }
    err = pthread_detach(rcvthread);
    if (err != 0) {
        //slog(SLOG_ERROR, "TCP: Could not detach rcv thread for (%d) ", conn);
        return;
    }
    //slog(SLOG_INFO, "TCP: Receive thread created successfully for (%d).", *conn_ptr);
}

void* tcp_receive_function(void* conn)
{
    int _conn = *((int*)conn);
    free(conn);
    if (_conn <= 0) {
        //slog(SLOG_ERROR, "TCP: not established with (%d), can't recv", _conn);
        return NULL;
    }
    char* buf;
    do {
        /* read message */
        buf = tcp_receive_message(_conn);

        /* recv failed or disconnected */
        if (buf == NULL)
            return NULL;

        /* apply message handler */
        if (on_data_handler != NULL)
            (*on_data_handler)(thismodule,_conn, buf);

        free(buf); /* TODO: it can't be passed btw threads */

    } while (1);
    return NULL;
}

