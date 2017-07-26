/*
 * control_mqtt_sender.c
 *
 *  Created on: 24 Jul 2017
 *      Author: rad
 */



#include <mosquitto.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>/* for sleep */
#include <time.h>
#include <stdio.h>
#include <string.h>


unsigned int nb_msg = 500;

unsigned int time_total = 0;
unsigned int count_msg = 0;


typedef struct _mqtt_channel__
{
	struct mosquitto *mosq;

	int publish;
	int subscribe;
	char* topic;
	char* clientid;

	int fd;
	char* fd_str;
	int connected;

	pthread_t listenthread;
}_mqtt_channel;

void mqtt_run_listen_thread(_mqtt_channel *channel);

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{}

void disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{}

_mqtt_channel* channel_new(const char* host, int port, const char* topic,
		int publish, int subscribe, char* clientid)
{
	static int conn_counter = 0;
	_mqtt_channel* channel;

	if(topic == NULL)
		return NULL;

	channel = (_mqtt_channel*) malloc(sizeof(_mqtt_channel));

	channel->topic = strdup(topic);
	channel->clientid = strdup(clientid);
	channel->publish = publish;
	channel->subscribe = subscribe;

	channel->connected = 0;

	/* mosq stuff */
	channel->mosq = mosquitto_new(channel->clientid, 1, 0);

	if(channel->mosq)
	{
		mosquitto_connect_callback_set(channel->mosq, connect_callback);
		mosquitto_message_callback_set(channel->mosq, message_callback);
		mosquitto_disconnect_callback_set(channel->mosq, disconnect_callback);
	}

	mqtt_run_listen_thread(channel);

	mosquitto_connect(channel->mosq, host, port, 60); // != MOSQ_ERR_SUCCESS FIXME
	if(channel->subscribe)
		mosquitto_subscribe(channel->mosq, NULL, topic, 2);

	channel->fd = ++conn_counter;
	channel->fd_str = (char*)malloc(20*sizeof(char));
	sprintf(channel->fd_str, "%d", channel->fd);

	printf("channel new %d %s pub:%d, sub:%d\n", channel->fd, channel->topic, channel->publish, channel->subscribe);
	return channel;
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
	default:
	{
		printf("Usage: ./control_mqtt_sender [nbmsg] \n"
				"\tnbmsg              default 500\n");

		return -1;
	}
	}
	printf("\tnbmsg    %d\n", nb_msg);

	mosquitto_lib_init();

	_mqtt_channel* channel = channel_new(
			"127.0.0.1", //mqtt_host
			1883,        //mqtt_port
			"test123",      //topic
			1,           //publish
			0,           //subscribe
			"sender"       //clientid
			);

	char data[250];

	unsigned int i=0;
	sprintf(data, "date %d\n", i);

	mosquitto_publish(channel->mosq, NULL, channel->topic, strlen(data), data, 0, true);

	for(i=0; i<nb_msg; i++)
	{
		//sprintf(data, "{ \"status\": 9, \"msg\": \"{ \\\"status\\\": 9, \\\"msg\\\": \\\"{ \\\\\\\"value\\\\\\\": 9, \\\\\\\"datetime\\\\\\\": \\\\\\\"today\\\\\\\" }\\\", \\\"ep_id\\\": \\\"CiZTvUvpRY\\\", \\\"msg_id\\\": \\\"%d\\\" }\", \"msg_id\": \"%d\" }", i, i);
		sprintf(data, "date %d\n", i);

		mosquitto_publish(channel->mosq, NULL, channel->topic, strlen(data), data, 0, true);
		count_msg += 1;
		time_total += clock();
	}

	printf("done %d\n", count_msg);
	sleep(1);


	return 0;
}

void* mqtt_listen_function(void* channel_ptr)
{
	int rc;
	_mqtt_channel *channel = (struct _mqtt_channel__*) channel_ptr;
	while(1)
	{
		rc = mosquitto_loop(channel->mosq, -1, 1); //0
		if(rc)
		{
		    /*connection error! try again*/
			sleep(10);
			mosquitto_reconnect(channel->mosq);
		}
	}

    return NULL;
}

void mqtt_run_listen_thread(_mqtt_channel *channel)
{
    //pthread_t listenthread;
    int err;

    err = pthread_create(&(channel->listenthread), NULL, mqtt_listen_function, (void*)channel);
    if (err != 0)
    {
        /* can't create listen thread */
        return;
    }
    err = pthread_detach(channel->listenthread);
    if (err != 0)
    {
        /* Could not detach listen thread */
        return;
    }
   /* Listen thread created successfully */
}
