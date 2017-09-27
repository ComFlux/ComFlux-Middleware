/*
 * message.c
 *
 *  Created on: 11 Apr 2016
 *      Author: Raluca Diaconu
 */

#include "message.h"

#include "hashmap.h"
#include "endpoint.h"
#include <utils.h>

#include <stdio.h>

extern HashMap* endpoints;

MESSAGE* message_new(const char* msg_, unsigned int status_)
{
	MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
	message->_msg_json = json_new(msg_);
	//message->msg_str = NULL;//strdup_null(msg_);
	message->status = status_;

	message->msg_id = message_generate_id();

	message->ep = NULL;

	message->conn = 0;
	message->module = NULL;

	message->data = (void*)msg_;
	message->size = strlen(msg_);
	return message;
}

MESSAGE* message_new_json(JSON* msg_, unsigned int status_)
{
	MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
	message->_msg_json = msg_;
	//message->msg_str = NULL;//json_to_str(msg_);
	message->status = status_;

	message->msg_id = message_generate_id();

	message->ep = NULL;


	message->conn = 0;
	message->module = NULL;

	message->data = NULL;
	message->size = 0;

	return message;
}

MESSAGE* message_new_id(const char* msg_id, const char* msg_, unsigned int status_)
{
	MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
	message->msg_id = strdup_null(msg_id);
	message->_msg_json = json_new(msg_);
	//message->msg_str = NULL;//strdup_null(msg_);

	message->status = status_;

	message->ep = NULL;


	message->conn = 0;
	message->module = NULL;

	message->data = (void*)msg_;
	message->size = strlen(msg_);

	return message;
}

MESSAGE* message_new_id_json(const char* msg_id, JSON* msg_, unsigned int status_)
{
	MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
	message->msg_id = strdup_null(msg_id);
	message->_msg_json = msg_;
	//message->msg_str = NULL;//json_to_str(msg_);

	message->status = status_;

	message->ep = NULL;


	message->conn = 0;
	message->module = NULL;
printf("%s\n\n", message_to_str(message));
	return message;
}

void message_free(MESSAGE* msg)
{
	if(msg == NULL)
		return;

	if(msg->_msg_json)
	{
		//wjson_free(msg->_msg_json);
		msg->_msg_json = NULL;
	}

	//free(msg->msg_str);
	free(msg->msg_id);
	free(msg->module);
	free(msg);
}

MESSAGE* message_parse(const char* msg)
{
	if(msg == NULL)
		return NULL;
	JSON* json_msg = json_new(msg);
	MESSAGE* message = message_parse_json(json_msg);

	message->data = (void*)msg;
	message->size = strlen(msg);

	return message;
}

MESSAGE* message_parse_json(JSON* json_msg)
{
	if(json_msg == NULL)
		return NULL;

	MESSAGE* ret_msg = malloc(sizeof(MESSAGE));
	char* ep_id = json_get_str(json_msg, "ep_id");
	if (ep_id != NULL)
	{
		ret_msg->ep = (ENDPOINT*)map_get(endpoints, ep_id);
		free(ep_id);
	}
	else
		ret_msg->ep = NULL;
	ret_msg->msg_id = json_get_str(json_msg, "msg_id");
	ret_msg->_msg_json = json_get_json(json_msg, "msg_json");
	//ret_msg->msg_str = NULL;//json_get_str(json_msg, "msg");
	ret_msg->status = (unsigned int) json_get_int(json_msg, "status");

	char* module = json_get_str(json_msg, "module");
	int conn = json_get_int(json_msg, "conn");
	if (module != NULL && conn != 0)
	{
		ret_msg->conn = conn;
		ret_msg->module = module;
	}
	else
	{
		ret_msg->conn = 0;
		ret_msg->module = NULL;
	}
	json_free(json_msg);

	return ret_msg;
}

JSON* message_to_json(MESSAGE* msg)
{
	JSON *msg_json = json_new(NULL);

	json_set_int(msg_json, "status", (int)(msg->status));

	if(msg->_msg_json)
		json_set_json(msg_json, "msg_json", msg->_msg_json);

	//if(msg->msg_str)
	//	json_set_str(msg_json, "msg", msg->msg_str);

	if (msg->ep)
		json_set_str(msg_json, "ep_id", msg->ep->id);

	if(msg->msg_id)
		json_set_str(msg_json, "msg_id", msg->msg_id);


	if(msg->conn)
		json_set_int(msg_json, "conn", msg->conn);

	if(msg->module)
		json_set_str(msg_json, "module", msg->module);

	return msg_json;
}

char* message_to_str(MESSAGE* msg)
{
	JSON *msg_json = message_to_json(msg);

	char* js = json_to_str(msg_json);
	json_free(msg_json);
	return js;
}

char* message_generate_id()
{
	static int counter_messages=0;
	//static char id[10];
	char *id = (char*)malloc(11*sizeof(char));
	sprintf(id, "%d", counter_messages);
	counter_messages +=1;
	return id;
}

const char* message_status_to_str(int msg_status)
{
	static const char status_str[16][16]= {"MSG_NONE",
"MSG_HELLO",
"MSG_HELLO_ACK",
"MSG_AUTH",
"MSG_AUTH_ACK",
"MSG_MAP",
"MSG_MAP_ACK",
"MSG_UNMAP",
"MSG_UNMAP_ACK",
"MSG_MSG",
"MSG_REQ",
"MSG_RESP_NEXT",
"MSG_RESP_LAST",
"MSG_STREAM",
"MSG_STREAM_CMD",
"MSG_CMD"};

	if(msg_status<0 || msg_status >15)
		return NULL;

	return status_str[msg_status];
}
