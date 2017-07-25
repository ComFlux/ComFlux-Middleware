/*
 * control_tcp_receiver.c
 *
 *  Created on: 25 Jul 2017
 *      Author: rad
 */


#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>/* for sleep */
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#include <sys/ioctl.h>
#include <netinet/in.h>

#ifdef __unix__
#include <linux/if.h>
#include <linux/if_tun.h>
#else
#include <net/if.h>
#endif

unsigned int total_msg = 500;

unsigned int started_flag = 0;
unsigned int stopped_flag = 0;

unsigned int time_start = 0;
unsigned int time_total = 0;
unsigned int count_msg = 0;

void* api_on_message(void* data)
{
	printf("callback\n");
	char* buf  = (char*) data;

	printf("%s\n", buf);
	if(started_flag == 0 && stopped_flag ==0)
	{
		started_flag = 1;
		time_start = clock();
		//return;
	}

	if(started_flag == 1 && stopped_flag == 0
			&& count_msg>=total_msg)
	{
		stopped_flag = 1;
	}

	if(started_flag == 1 && stopped_flag ==0)
	{
		count_msg += 1;
		time_total += clock();
	}

	return NULL;
}

#define BUFFER_FINAL 	0
#define BUFFER_JSON  	1
#define BUFFER_STR_2 	2
#define BUFFER_ESC_2 	3
#define BUFFER_STR_1 	4
#define BUFFER_ESC_1 	5

typedef struct _BUFFER{
	char* data;
	int size;

	int buffer_state;
	int brackets;
}BUFFER;

BUFFER* api_buffer;

void buffer_set(BUFFER* buffer, const char* new_buf, int new_start, int new_end);
void buffer_reset(BUFFER* buffer);
void buffer_update(BUFFER* buffer, const char* new_data, int new_size);


char* tcp_receive_message(int _conn);
void tcp_run_accept_thread(int serversock);
void* tcp_accept_function(void* serversock);
void* tcp_accept_function(void* serversock);
void tcp_run_receive_thread(int conn);
void* tcp_receive_function(void* conn);

void init(const char *server_addr, int server_port)
{
	if (server_port == -1)
	    {
	        server_port = 0;
	    }

	    int serversock;              //, peersock;
	    struct sockaddr_in serverin; //, peerin;

	    serversock = socket(AF_INET, SOCK_STREAM, 0);
	    if (serversock == -1)
	    {
	        printf("Could not create server socket.\n");
	        return;
	    }

	    serverin.sin_family = AF_INET;
	    serverin.sin_addr.s_addr = INADDR_ANY;
	    serverin.sin_port = htons(server_port);

	    if (bind(serversock, (struct sockaddr*)&serverin, sizeof(serverin)) < 0)
	    {
	        printf("Bind to %s:%d failed.", server_addr, server_port);
	        return;
	    }

	    listen(serversock, 3);

	    tcp_run_accept_thread(serversock);

}

char* tcp_receive_message(int _conn)
{
    uint32_t varSize;
    //int allBytesRecv; /* sum af all received packets */
    int recvSize;     /* one pack received size */
    unsigned char recvEscape;
    char* buf;
    printf("About to receive message using tcp on socket (%d)", _conn);

        /* reading msg */
        //allBytesRecv = 0;
        buf = (char*)malloc((512 + 1) * sizeof(char));
        memset(buf, '\0', (512 + 1));
        recvSize = recv(_conn, buf, 512, 0);
        if (recvSize <= 0) {
            printf("Recv msg failed from (%d). closing connection ", _conn);
            // connection_close(_conn);

            close(_conn);
            return NULL;
        }

       printf("received %d total bytes on sock (%d): *%s*", recvSize, _conn, buf);

        return buf;
}

void tcp_run_accept_thread(int serversock)
{
    int err;
    int* serversock_ptr = (int*)malloc(sizeof(int));
    *serversock_ptr = serversock;
    printf("Waiting for incoming connections for server (%d).", serversock);
    pthread_t listenthread;
    err = pthread_create(&listenthread, NULL, &tcp_accept_function, (void*)serversock_ptr);
    if (err != 0) {
        printf("can't create listen thread"); //: %s", strerror(err));
        return;
    }
    err = pthread_detach(listenthread);
    if (err != 0) {
        printf("Could not detach listen thread ");
        return;
    }
    //slog(SLOG_INFO, SLOG_INFO, "Listen thread created successfully.");
}

void* tcp_accept_function(void* serversock)
{
    int peersock;
    struct sockaddr_in peerin;

    // accept connection from an incoming client
    int c = sizeof(struct sockaddr_in);
    while (1) {
        peersock = accept(*((int*)serversock), (struct sockaddr*)&peerin, (socklen_t*)&c);
        if (peersock < 0) {
            printf("accept failed for (%d).", *((int*)serversock));
            return NULL; // TODO
        }
        printf("connection accpeted by socket (%d) ", peersock);
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

    err = pthread_create(&rcvthread, NULL, &tcp_receive_function, (void*)conn_ptr);
    if (err != 0) {
        printf("can't create receive thread  for (%d).", conn);
        return;
    }
    err = pthread_detach(rcvthread);
    if (err != 0) {
        printf("Could not detach rcv thread for (%d) ", conn);
        return;
    }
    printf("Receive thread created successfully for (%d).", *conn_ptr);
}

void* tcp_receive_function(void* conn)
{
    int _conn = *((int*)conn);
    if (_conn <= 0) {
        printf("not established with (%d), can't recv", _conn);
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
        buffer_update(api_buffer, buf, strlen(buf));

        free(buf); /* TODO: it can't be passed btw threads */

    } while (1);
    return NULL;
}


int main()
{
	api_buffer = (BUFFER*) malloc(sizeof(BUFFER));
	api_buffer->data = NULL;
	api_buffer->size = 0;
	api_buffer->buffer_state = 0;
	api_buffer->brackets = 0;

	init("127.0.0.1", 1505);

	int i;

	while(stopped_flag == 0)
	{
		sleep(2);

		printf("\n\n nb msg received: %d \ntotal time received %d \n", count_msg, time_total - time_start);
		printf("avg:  %f\n", (time_total - time_start)/(float)count_msg);
	}

	sleep(1);
	printf("Total: ");
	printf("\n\n nb msg received: %d \ntotal time received %d \n", count_msg, time_total - time_start);
	printf("avg:  %f\n", (time_total - time_start)/(float)count_msg);

	return 0;
}



void buffer_set(BUFFER* buffer, const char* new_buf, int new_start, int new_end)
{
	char* old_buf = buffer->data;
	int new_size = new_end-new_start;
	buffer->data = (char*) malloc(buffer->size + new_size + 1);
	memcpy(buffer->data,             old_buf,           buffer->size);
	memcpy(buffer->data+buffer->size, new_buf+new_start, new_size   );
	buffer->size = buffer->size + new_size;
	buffer->data[buffer->size] = '\0';

	//printf("----buffer set %d %d %d: %s\n\n", new_start, new_end, buffer->size, buffer->data);
	free(old_buf);
}

void buffer_reset(BUFFER* buffer)
{
	if(buffer->data)
		free(buffer->data);

}

void buffer_update(BUFFER* buffer, const char* new_data, int new_size)
{
	int i=0;
	int word_start = i;
	int word_end = i;
	for(i=0; i<new_size; i++)
	{
		switch(buffer->buffer_state){
			case BUFFER_FINAL:
				switch (new_data[i])
				{
					case '{':
						buffer->brackets += 1;
						word_start = i;
						buffer->buffer_state = BUFFER_JSON;
						break;
					/* ignore spaces and new lines */
					case ' ': case '\n': case '\r':
						break;
					default:
						//slog(SLOG_ERROR, SLOG_ERROR, "%c", new_data[i]);
						continue;

				}
				break;

			case BUFFER_JSON:
				switch (new_data[i])
				{
					case '{':
						buffer->brackets += 1;
						//data->buffer_state = 1;
						break;
					case '}':
						buffer->brackets -= 1;

						if(buffer->brackets == 0) // finished 1 word
						{
							buffer->buffer_state = BUFFER_FINAL;
							word_end = i+1;
							buffer_set(buffer, new_data, word_start, word_end);
							/* apply the callback for this connection */

							//printf(" -- %s\n", buffer->data);
							pthread_t api_on_msg_thread;
							pthread_create(&api_on_msg_thread, NULL, api_on_message, strdup(buffer->data));

							//api_on_message(buffer->data);

							buffer->size = 0;
							word_start = i+1;
						}

						break;
					case '\"':
						buffer->buffer_state = BUFFER_STR_2;
						break;
					case '\'':
						buffer->buffer_state = BUFFER_STR_1;
						break;

					default:
						continue;
				}
				break;
			case BUFFER_STR_1:
				switch (new_data[i])
				{
					case '\\':
						buffer->buffer_state = BUFFER_ESC_1;
						break;
					case '\'':
						buffer->buffer_state = BUFFER_JSON;
						break;
					default:
						continue;
				}
				break;
			case BUFFER_ESC_1:
				buffer->buffer_state = BUFFER_STR_1;
				break;
			case BUFFER_STR_2:
				switch (new_data[i])
				{
					case '\\':
						buffer->buffer_state = BUFFER_ESC_2;
						break;
					case '\"':
						buffer->buffer_state = BUFFER_JSON;
						break;
					default:
						continue;
				}
				break;
			case BUFFER_ESC_2:
				buffer->buffer_state = BUFFER_STR_2;
				break;
			default: //impossible
				return;
		}
	}

	if(word_start<new_size && buffer->buffer_state != BUFFER_FINAL)
		buffer_set(buffer, new_data, word_start, new_size);
}
