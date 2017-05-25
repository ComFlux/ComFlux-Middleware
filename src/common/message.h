/*
 * message.h
 *
 *  Created on: 11 Apr 2016
 *      Author: Raluca Diaconu
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "json.h"
#include "endpoint_base.h"
#include <slog.h>

/* constants for status: max val 15 */
#define MSG_NONE		0

/* core 2 core comm */
#define MSG_HELLO		1
#define MSG_HELLO_ACK	2

#define MSG_AUTH		3
#define MSG_AUTH_ACK	4

#define MSG_MAP			5
#define MSG_MAP_ACK		6
#define MSG_UNMAP		7
#define MSG_UNMAP_ACK	8

#define MSG_MSG			9
#define MSG_REQ			10
#define MSG_RESP_NEXT	11
#define MSG_RESP_LAST	12
#define MSG_STREAM		13
#define MSG_STREAM_CMD	14

/* app 2 core */
#define MSG_CMD			15 /* other command */


typedef struct _MESSAGE{
	char * msg_id; 		/* general message_id */
	ENDPOINT *ep;		/* source or destination */
	char * msg; 		/* actual message */

	 /* int value if the message status as defined above
	 	e.g., last message for resp+, req message, src/snk msg
	 */
	unsigned int status	:6;

	int conn;
	char* module;
} MESSAGE;


MESSAGE* message_new(const char *msg_, unsigned int status_);
MESSAGE* message_new_id(const char* msg_id, const char *msg_, unsigned int status_);

void message_free(MESSAGE *msg);

MESSAGE* message_parse(const char *msg);
JSON* message_to_json(MESSAGE *msg);
char* message_to_str(MESSAGE *msg);

/*
 * These functions concern message ordering across the application.
 */
char*  message_generate_id();

/* build various messages in the protocol */
/* TODO */

/* msg status in str for helping log */
const char* message_status_to_str(int msg_status);

#endif /* MESSAGE_H_ */
