/*
 * com_mqtt.c
 *
 *  Created on: 21 Apr 2017
 *      Author: Raluca Diaconu
 */

#include "com_mqtt.h"

#include "com.h"
#include <mosquitto.h>

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>/* for sleep */

#include <json.h>
#include <hashmap.h>
#include <utils.h>

void* thismodule = NULL;

HashMap* conn_table;      /* conn -> channel */
HashMap* mosq_table;      /* mosq -> channel */


JSON* args_json = NULL;
int   clean     = 1;
char* appid     = NULL;
#define TOPIC_SIZE 10
#define QoS 0

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
	channel->mosq = mosquitto_new(channel->clientid, clean, 0);

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

	//printf("channel new %d %s pub:%d, sub:%d\n", channel->fd, channel->topic, channel->publish, channel->subscribe);
	return channel;
}

void channel_free(_mqtt_channel* channel)
{
	if (channel == NULL)
		return;

	pthread_cancel(channel->listenthread);
	map_remove(conn_table, (void*)channel->fd_str);
	map_remove(mosq_table, (void*)channel->mosq);

	mosquitto_unsubscribe(channel->mosq, NULL, channel->topic);
	mosquitto_destroy(channel->mosq);

	free(channel->topic);
	free(channel->fd_str);
	free(channel->clientid);
	free(channel);
}



void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	/* set channel->connected flag */
	struct _mqtt_channel__* channel = NULL;

	channel = map_get(mosq_table, mosq);
	if (channel == NULL)
	{
		return;
		//some error
	}
	channel->connected = 1;
}

void disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	struct _mqtt_channel__* channel = NULL;

	channel = map_get(mosq_table, mosq);
	if (channel == NULL)
	{
		return;
		//some error
	}

	/* call disconnect handler from the core */
	if(on_disconnect_handler && channel->connected)
		(*on_disconnect_handler)(thismodule, channel->fd);

	//channel_free(channel);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct _mqtt_channel__* channel = NULL;
	char* data = NULL;
	JSON* json_to_core = NULL;
	JSON* outer_json_to_core = NULL;
	char* msg_to_core = NULL;

	channel = map_get(mosq_table, mosq);
	if (channel == NULL)/* nothing to do, wrong call */
	{
		return;
	}
	if (channel->connected == 0) /* not yet connected, ignore */
	{
		return;
	}
	data = (char*)malloc( 200 + message->payloadlen);

	sprintf(data, "%.*s", message->payloadlen, (char*) message->payload);
	json_to_core = json_new(NULL);
	json_set_json(json_to_core, "msg_json", json_new(data));
	//json_set_str(json_to_core, "msg", data);
	json_set_int(json_to_core, "status", 9);

	outer_json_to_core = json_new(NULL);
	json_set_json(outer_json_to_core, "msg_json", json_to_core);
	//json_set_str(outer_json_to_core, "msg", json_to_str(json_to_core));
	json_set_int(outer_json_to_core, "status", 9);

	msg_to_core = json_to_str(outer_json_to_core);
	//printf("-------- send to core %s\n", msg_to_core);

	/* send data to the core */
	if(on_data_handler)
		(*on_data_handler)(thismodule, channel->fd, msg_to_core);

	json_free(outer_json_to_core);
	json_free(json_to_core);
	free(data);
	free(msg_to_core);
}

char* com_init(void* module, const char* config_json)
{
	/* set the handle */
    thismodule = module;

    /* initialise connection table: connection id -> channel */
    conn_table = map_new(KEY_TYPE_STR);
    mosq_table = map_new(KEY_TYPE_PTR);

    /* parse the json args */
	args_json = json_new(config_json);
	appid     = json_get_str(args_json, "clientid");
    clean     = json_get_int(args_json, "clean_session");
    /* set default as clen session */
    if(clean != 0 && clean != 1)
    	clean = 1;

    /* init mosq structure */
	mosquitto_lib_init();

	json_free(args_json);

	return appid; // FIXME
}

/* address corresponds to topic */
int com_connect(const char *connect_config)
{
	JSON* connect_json = NULL;
    char* mqtt_host = NULL;
    int   mqtt_port = 0;
    char* topic     = NULL;
    int   publish   = 0;
    int   subscribe = 0;
    char* clientid  = NULL;

	_mqtt_channel* channel = NULL;

    /* parse the json args */
    connect_json = json_new(connect_config);
    mqtt_host    = json_get_str(connect_json, "host");
    mqtt_port    = json_get_int(connect_json, "port");
    topic        = json_get_str(connect_json, "topic");
    publish      = json_get_int(connect_json, "publish");
    subscribe    = json_get_int(connect_json, "subscribe");
	clientid     = json_get_str(connect_json, "clientid");

	channel = channel_new(mqtt_host, mqtt_port, topic, publish, subscribe, clientid);

	map_update(conn_table,  channel->fd_str,     (void*)channel );
	map_update(mosq_table, (void*)channel->mosq, (void*)channel );


    if (on_connect_handler != NULL)
        (*on_connect_handler)(thismodule, channel->fd);

	if(channel)
		return channel->fd;
	else
		return -1;
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
	_mqtt_channel* channel = NULL;
	char conn_str[20];
	int err = -1;

	JSON* msg_json = NULL;
	int msg_status = -1;

	sprintf(conn_str, "%d", conn);
	channel = map_get(conn_table, conn_str);
	if(channel == NULL)
	{
		return -1;
	}

	msg_json = json_new(data);
	msg_status = json_get_int(msg_json, "status");
	/* check if it's first message
	 * MSG_MAP is 5
	 * MSG_MAP_ACK is 6 */
	if(msg_status == 5 && channel->connected == 0)
	{
		JSON* ack_json = json_new(NULL);
		JSON* msg_json = json_new(NULL);
		JSON* ep_md_json = json_new(NULL);

		json_set_int(msg_json, "ack_code", 0);
		json_set_str(ep_md_json, "ep_id", "mqtt");
		json_set_json(msg_json, "ep_metadata", ep_md_json);
		json_set_int(ack_json, "status", 6);
		//json_set_str(ack_json, "msg", json_to_str(msg_json));
		json_set_json(ack_json, "msg_json", msg_json);

		//printf("send back: %s\n\n", json_to_str(ack_json));
		(*on_data_handler)(thismodule, channel->fd,
				json_to_str(ack_json));
		return 0;
	}

	if(channel->publish && msg_status == 9) /* MSG_MSG */
	{
		err = mosquitto_publish(channel->mosq, NULL, channel->topic, strlen(data), data, QoS, true);
	}
	else
	{
		err = -2;
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
	return 0;
}



/* mqtt functionality */

void* mqtt_listen_function(void* channel_ptr)
{
	int rc;
	struct _mqtt_channel__ *channel = (struct _mqtt_channel__*) channel_ptr;
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

void mqtt_run_listen_thread(struct _mqtt_channel__ *channel)
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

