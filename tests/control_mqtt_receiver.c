/*
 * control_mqtt_receiver.c
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

#include <sys/time.h>
#include <json.h>

#define QoS 0

unsigned int total_msg = 500;

unsigned int started_flag = 0;
unsigned int stopped_flag = 0;

struct timeval time_start;
double time_total = 0;
unsigned int count_msg = 0;

JSON* msg_schema;

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
{
	char* data  = malloc(message->payloadlen +1);
	strncpy(data, (char*) message->payload, message->payloadlen);

	JSON* msg_json = json_new(data);

	//printf("%.*s\n", message->payloadlen, (char*) message->payload);
	if(started_flag == 0 && stopped_flag ==0)
	{
		started_flag = 1;
		//time_start = time(NULL);//clock();
		gettimeofday(&time_start, NULL);
		//return;
	}
	if (!json_validate(msg_schema, msg_json))
	{
		if(started_flag == 1 && stopped_flag == 0
				&& count_msg>=total_msg)
		{
			stopped_flag = 1;
			//time_total = (time(NULL)-time_start);
			struct timeval t1;
			gettimeofday(&t1, NULL);
			time_total = (t1.tv_sec - time_start.tv_sec) * 1000.0;      // sec to ms
			time_total += (t1.tv_usec - time_start.tv_usec) / 1000.0;   // us to ms

			printf("Total: ");
			printf("\n\n nb msg received: total time received: avg \n");
			printf(" %d\t%lf\t%lf\n\n", count_msg, time_total, time_total/count_msg);

		}

		else if(started_flag == 1 && stopped_flag ==0)
		{
			count_msg += 1;
		}
	}
}

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
		mosquitto_subscribe(channel->mosq, NULL, topic, QoS);

	channel->fd = ++conn_counter;
	channel->fd_str = (char*)malloc(20*sizeof(char));
	sprintf(channel->fd_str, "%d", channel->fd);

	printf("channel new %d %s pub:%d, sub:%d\n", channel->fd, channel->topic, channel->publish, channel->subscribe);
	return channel;
}


int main(int argc, char *argv[])
{
	mosquitto_lib_init();

	printf("argc: %d\n", argc);
	msg_schema = json_load_from_file("datetime_value.json");
	printf("msg schema: %s\n", json_to_str_pretty(msg_schema));

	switch (argc)
	{
	case 1: break;
	case 2:
	{
		total_msg=atoi(argv[1]);
		break;
	}
	default:
	{
		printf("Usage: ./control_mqtt_sender [nbmsg] \n"
				"\tnbmsg              default 500\n");

		return -1;
	}
	}
	printf("\tnbmsg    %d\n", total_msg);

	_mqtt_channel* channel = channel_new(
			"54.154.142.51", //mqtt_host
			1884,        //mqtt_port
			"test123",      //topic
			0,           //publish
			1,           //subscribe
			"receiver"       //clientid
			);


	while(stopped_flag == 0)
    {
    	sleep(2);

    	//printf("\n\n nb msg received: %d \ntotal time received %lf \n", count_msg, ((double)time_total));
    	//printf("avg:  %lf\n", (time_total/(double)count_msg));
    }

	//sleep(1);
	mosquitto_disconnect(channel->mosq);

	printf("Total: ");
	printf("\n\n nb msg received: total time received: avg \n");
	printf(" %d\t%lf\t%lf\n\n", count_msg, time_total, time_total/count_msg);

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
