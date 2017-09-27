/*
 * com_mqtt_bridge.c
 *
 *  Created on: 20 Apr 2017
 *      Author: Raluca Diaconu
 */

#include "com_mqtt_bridge.h"

#include "com.h"
#include <mosquitto.h>

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>/* for sleep */

#include <json.h>
#include <hashmap.h>
#include <utils.h>

HashMap* conn_table;      /* conn      -> channel */
HashMap* topic_in_table;  /* topic_in  -> channel */
//HashMap* channel_table;   /* channel -> conn  */

void* thismodule = NULL;

struct mosquitto *mosq;

#define TOPIC_SIZE 10

char* thistopic; /* corresponds to server_addr */

typedef struct _mqtt_channel__
{
	int fd;
	char* topic_in;
	char* topic_out;
	char* fd_str;
	int connected;
}_mqtt_channel;

_mqtt_channel* channel_new(const char* topic_in, const char* topic_out)
{
	static int conn_counter = 0;
	_mqtt_channel* channel;

	channel = (_mqtt_channel*) malloc(sizeof(_mqtt_channel));
	channel->fd = ++conn_counter;
	channel->connected = 0;
	/* receive messages on this topic */
	if(topic_in)
	{
		channel->topic_in = strdup(topic_in);
		mosquitto_subscribe(mosq, NULL, channel->topic_in, 2);
	}
	else
		channel->topic_in = NULL;
	/* send messages on this topic */
	if(topic_out)
		channel->topic_out = strdup(topic_out);
	else
		channel->topic_out = NULL;

	channel->fd_str = (char*)malloc(20*sizeof(char));
	sprintf(channel->fd_str, "%d", channel->fd);

	printf("channel new %d %s %s\n", channel->fd, channel->topic_in, channel->topic_out);
	return channel;
}

void channel_free(_mqtt_channel* channel)
{
	map_remove(conn_table,     (void*)channel->fd_str);
	map_remove(topic_in_table, (void*)channel->topic_in);

	free(channel->topic_in);
	free(channel->topic_out);
	free(channel->fd_str);
	free(channel);
}

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//printf("connect to mqt broker, result=%d\n", result);
}

void disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//printf("disconnect from mqtt broker, result=%d\n", result);

	//mosquitto_destroy(mosq);
	//mosquitto_lib_cleanup();
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	//printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	_mqtt_channel* channel = NULL;
	char* buf = NULL;
	char new_topic_in[TOPIC_SIZE+1];
	char new_topic_out[TOPIC_SIZE+1];

	/* check if it is a new connection coming on listen topic */
	if(strcmp(message->topic, thistopic)==0)
	{
		//message is the out_toipic on which the otherone awaits
		memset(new_topic_in,  '\0', TOPIC_SIZE+1);
		memset(new_topic_out, '\0', TOPIC_SIZE+1);
		strncpy(new_topic_in,  randstring(TOPIC_SIZE), 5); //
		strncpy(new_topic_out, (char*) message->payload, TOPIC_SIZE);
		channel = channel_new(new_topic_in, new_topic_out);
		channel->connected = 1;
		map_update(conn_table,     channel->fd_str,          (void*)channel );
		map_update(topic_in_table, (void*)channel->topic_in, (void*)channel );

		if( mosquitto_publish(mosq, NULL, channel->topic_out, strlen(channel->topic_in), channel->topic_in, 1, true)
					== MOSQ_ERR_SUCCESS)
		{
			if(on_connect_handler != NULL)
			{
				(*(on_connect_handler))(thismodule, channel->fd);
			}
			else
			{

			}
		}

		goto final;
	}

	/* check if topic is correct */
	channel = (_mqtt_channel*)map_get(topic_in_table, message->topic);

	/* bad message on this channel; ignore */
	if(channel == NULL)
	{
		goto final;
	}

	/* finalize 3 step hanshake */
	if(channel->connected == 0)
	{
		if(message->payload)
		{
			channel->topic_out = (char*)malloc((TOPIC_SIZE + 1)*sizeof(char));
			memset(channel->topic_out, '\0', TOPIC_SIZE+1);
			strncpy(channel->topic_out, (char*) message->payload, TOPIC_SIZE);
			channel->connected = 1;
		}
		if(on_connect_handler != NULL)
		{
			(*(on_connect_handler))(thismodule, channel->fd);
		}

		goto final;
	}

    /* apply message handler */
    if (on_data_handler != NULL)
    {
    	/* check valid data */
    	buf = (char*) malloc((message->payloadlen + 1)*sizeof(char));
    	if(buf== NULL || message->payload == NULL)
    		return;

    	memset(buf, '\0', message->payloadlen+1);
    	strncpy(buf, message->payload, message->payloadlen);

    	/* send data to the core */
        (*on_data_handler)(thismodule, channel->fd, buf);

        goto final;
    }

    final:{
    	 free(buf);
    }
}

char* com_init(void* module, const char* config_json)
{
    JSON* args_json = NULL;
    char* mqtt_host = NULL;
    int   mqtt_port = 0;
    char* clientid  = NULL;
    int   clean     = 1;
    char* tmp_topic = NULL;

    int   subscribe = 1;
    int   publish   = 1;

	/* set the handle */
    thismodule = module;

    /* initialise connection table: connection id -> topic */
    conn_table = map_new(KEY_TYPE_STR);
    topic_in_table = map_new(KEY_TYPE_STR);

    /* parse the json args */
    args_json = json_new(config_json);
    mqtt_host = json_get_str(args_json, "host");
    mqtt_port = json_get_int(args_json, "port");
    clientid  = json_get_str(args_json, "clientid");
    clean     = json_get_int(args_json, "clean_session");
    /* set default as clen session */
    if(clean != 0 && clean != 1)
    	clean = 1;
    tmp_topic     = json_get_str(args_json, "topic");

    subscribe = json_get_int(args_json, "subscribe");
    if(subscribe != 0 && subscribe != 1)
    	subscribe = 1;
    publish   = json_get_int(args_json, "publish");
    if(publish != 0 && publish != 1)
    	publish = 1;

	mosquitto_lib_init();
	mosq = mosquitto_new(clientid, clean, 0);

	if(mosq)
	{
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);
		mosquitto_disconnect_callback_set(mosq, disconnect_callback);

		if (mosquitto_connect(mosq, mqtt_host, mqtt_port, 60) != MOSQ_ERR_SUCCESS)
			goto final;

		if(subscribe && tmp_topic)
		{
			thistopic = strdup(tmp_topic);
			mosquitto_subscribe(mosq, NULL, thistopic, 2);
			mqtt_run_listen_thread();
		}
	}

	final:{
	    json_free(args_json);
	    free(mqtt_host);
	    free(clientid);

	    return thistopic;
	}
}

/* address corresponds to topic */
int com_connect(const char *topic)
{
	_mqtt_channel* channel = NULL;
	char* data = randstring(TOPIC_SIZE);//"snk_A";
	int result = -1;

	channel = channel_new(data, NULL);

	map_update(conn_table,     channel->fd_str,          (void*)channel );
	map_update(topic_in_table, (void*)channel->topic_in, (void*)channel );

	if( mosquitto_publish(mosq, NULL, topic, TOPIC_SIZE, data, 1, true)
			== MOSQ_ERR_SUCCESS)
	{
		result = channel->fd;
	}
	else
	{
		channel_free(channel);
		result = -1;
	}
	free(data);
	return result;
}


int com_connection_close(int conn)
{
	_mqtt_channel* channel = NULL;
	char  conn_str[20];

	sprintf(conn_str, "%d", conn);
	channel = (_mqtt_channel*)map_get(conn_table, conn_str);

	if(channel == NULL)
		return -1;

	channel_free(channel);

	return 0;
}

/*
 * publish data on the conn topic
 */
int com_send_data(int conn, const char *data)
{
	return com_send(conn, (void*)data, strlen(data));
}

int com_send(int conn, void *data, unsigned int size)
{
	_mqtt_channel* channel = NULL;
	char conn_str[20];
	int err = -1;

	sprintf(conn_str, "%d", conn);
	channel = map_get(conn_table, conn_str);

	if(channel != NULL)
	{
		//printf("---send ok: (%d:%s)\n", channel->fd, channel->topic_out);
		err = mosquitto_publish(mosq, NULL, channel->topic_out, size, data, 1, true);
	}
	else
	{
		err = -1;
	}

	return err;
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

/*
 * In mqtt Topics are not something that really exist
 * until the moment a message is published to one.
 * This function always returns 1;
 */
int com_is_valid_address(const char* full_address)
{
	return 1;
}

/**
 * Because connection is 1 to many, this is a non bridge module.
 */

int com_is_bridge(void)
{
	return 1;
}



/* mqtt functionality */

void* mqtt_listen_function(void* null)
{
	int rc;
	while(1)
	{
		rc = mosquitto_loop(mosq, -1, 1); //0
		if(rc)
		{
		    /*connection error! try again*/
			sleep(10);
			mosquitto_reconnect(mosq);
		}
	}

    return NULL;
}

void mqtt_run_listen_thread(void)
{
    pthread_t listenthread;
    int err;

    err = pthread_create(&listenthread, NULL, mqtt_listen_function, NULL);
    if (err != 0)
    {
        /* can't create listen thread */
        return;
    }
    err = pthread_detach(listenthread);
    if (err != 0)
    {
        /* Could not detach listen thread */
        return;
    }
   /* Listen thread created successfully */
}
