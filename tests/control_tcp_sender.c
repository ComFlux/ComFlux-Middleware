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

char receiver_addr[200] = "34.229.95.129";
unsigned int receiver_port = 1505;

unsigned int nb_msg = 500;

unsigned int time_total = 0;
unsigned int count_msg = 0;


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
	char data[250];

	unsigned int i=0;

	sprintf(data, "{date %d\n}", i);

	com_send_data(conn, data);

	for(i=0; i<nb_msg; i++)
	{
		sprintf(data, "{ \"status\": 9, \"msg\": \"{ \\\"status\\\": 9, \\\"msg\\\": \\\"{ \\\\\\\"value\\\\\\\": 9, \\\\\\\"datetime\\\\\\\": \\\\\\\"today\\\\\\\" }\\\", \\\"ep_id\\\": \\\"CiZTvUvpRY\\\", \\\"msg_id\\\": \\\"%d\\\" }\", \"msg_id\": \"%d\" }", i, i);
		//sprintf(data, "{date %d\n}", i);

		com_send_data(conn, data);
		count_msg += 1;
		//time_total += clock();
	}

	printf("done %d\n", count_msg);
	sleep(2);

	return 0;
}
