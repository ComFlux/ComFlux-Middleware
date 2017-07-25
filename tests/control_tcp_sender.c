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
    printf("send to (%d) *%s*\n", conn, msg);

    const uint32_t varSize = strlen(msg);
    int allBytesSent; /* sum of all sent sizes */
    ssize_t sentSize; /* one shot sent size */
    printf("About to send message using tcp on socket (%d)", conn);

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

int main()
{
	int conn = com_connect("127.0.0.1", 1505);
	char data[250];

	unsigned int i=0;

	for(i=0; i<nb_msg; i++)
	{
		//sprintf(data, "{ \"status\": 9, \"msg\": \"{ \\\"status\\\": 9, \\\"msg\\\": \\\"{ \\\\\\\"value\\\\\\\": 9, \\\\\\\"datetime\\\\\\\": \\\\\\\"today\\\\\\\" }\\\", \\\"ep_id\\\": \\\"CiZTvUvpRY\\\", \\\"msg_id\\\": \\\"%d\\\" }\", \"msg_id\": \"%d\" }", i, i);
		sprintf(data, "{date %d\n}", i);

		com_send_data(conn, data);
		count_msg += 1;
		time_total += clock();
	}

	printf("done %d\n", count_msg);
	sleep(1);

	return 0;
}
