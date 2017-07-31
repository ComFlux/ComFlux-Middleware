/*
 * control_tcp_sender.c
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

#include <json.h>

char receiver_addr[200] = "34.229.95.129";
unsigned int receiver_port = 1505;

unsigned int nb_msg = 500;

unsigned int time_total = 0;
unsigned int count_msg = 0;

char* file_to_str(const char* filename) {

	FILE* _file = fopen(filename, "r");

	if (_file == NULL)
		return NULL;

	/* get size of file */
	fseek(_file, 0L, SEEK_END);
	int size = (int) ftell(_file);
	rewind(_file);

	/* read file */
	char* var;
	var = (char*) malloc((size_t) size + 1);
	memset(var, 0, (size_t) size + 1);

	fread(var, 1, (size_t) size, _file);
	var[size] = '\0';
	fclose(_file);

	/* return text */
	return var;
}


int com_connect(const char* server_addr, int server_port)
{
    printf("Connecting to server %s:%d", server_addr, server_port);

    int peersock;
    struct sockaddr_in peerin;

    peersock = socket(AF_INET, SOCK_STREAM, 0);
    if (peersock == -1) {
    	printf("CONN: Could not create client socket.");
        return -2;
    }

    peerin.sin_addr.s_addr = inet_addr(server_addr);
    peerin.sin_family = AF_INET;
    peerin.sin_port = htons(server_port);

    if (connect(peersock, (struct sockaddr*)&peerin, sizeof(peerin)) < 0)
    {
    	printf("Could not connect to server %s:%d", server_addr, server_port);
        return -3;
    }
    //tcp_run_receive_thread(peersock);
    //receivethread = rcvthread;
    return peersock;
}

int com_send_data(int conn, const char* msg)
{
    if (conn <= 0) {
        printf("not established with (%d), can't send msg *%s*", conn, msg);
        return -1;
    }
    //printf("send to (%d) *%s*\n", conn, msg);

    const uint32_t varSize = strlen(msg);
    int allBytesSent; /* sum of all sent sizes */
    ssize_t sentSize; /* one shot sent size */
    //printf("About to send message using tcp on socket (%d)", conn);

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
        	printf("error sending msg on sock (%d)", conn);
        	break;
        }
        allBytesSent += sentSize;
    }
    return (int)allBytesSent;
}

int main(int argc, char *argv[])
{
	printf("argc: %d\n", argc);
	switch (argc)
	{
	case 1: break;
	case 2:
	{
		nb_msg=atoi(argv[1]);
		break;
	}
	case 3:
	{
		strcpy(receiver_addr, argv[1]);
		receiver_port = atoi(argv[2]);
		break;
	}
	case 4:
	{
		strcpy(receiver_addr, argv[1]);
		receiver_port = atoi(argv[2]);
		nb_msg=atoi(argv[3]);
		break;
	}
	default:
	{
		printf("Usage: ./control_tcp_sender [receiver_addr receiver_port] [nbmsg] \n"
				"\treceiver_addr      default 34.229.95.129;\n"
				"\treceiver_port      default 1505\n"
				"\tnbmsg              default 500\n");

		return -1;
	}
	}

	printf("\treceiver_addr: %s\n"
			"\treceiver_port: %d\n"
			"\tnbmsg    %d\n", receiver_addr, receiver_port, nb_msg);


	int conn = com_connect(receiver_addr, receiver_port);
	unsigned int i=0;

	/* build a message */
	JSON* msg_json = json_new(NULL);
	json_set_int(msg_json, "value", rand() % 10);
	json_set_str(msg_json, "date", "today");

	char* lorem = file_to_str("lorem.txt");
	json_set_str(msg_json, "lorem", lorem);

	char* data = json_to_str(msg_json);

	/* sleep */
	 struct timespec sleep_time;
	 sleep_time.tv_sec = 0;
	 sleep_time.tv_nsec=10000000L;

	com_send_data(conn, data);

	for(i=0; i<nb_msg; i++)
	{
		//sprintf(data, "{ \"status\": 9, \"msg\": \"{ \\\"status\\\": 9, \\\"msg\\\": \\\"{ \\\\\\\"value\\\\\\\": 9, \\\\\\\"datetime\\\\\\\": \\\\\\\"today\\\\\\\" }\\\", \\\"ep_id\\\": \\\"CiZTvUvpRY\\\", \\\"msg_id\\\": \\\"%d\\\" }\", \"msg_id\": \"%d\" }", i, i);
		//sprintf(data, "{date %d\n}", i);

		nanosleep(&sleep_time, NULL);
		com_send_data(conn, data);
		count_msg += 1;
		//time_total += clock();
	}

    close(conn);
	printf("done %d\n", count_msg);
	sleep(2);
        sleep(2);
        sleep(2);

	return 0;
}
