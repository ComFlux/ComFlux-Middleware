/*
 * endpoint_base.h
 *
 *  Created on: 7 Jun 2016
 *      Author: Raluca Diaconu
 */

#ifndef COMMON_ENDPOINT_BASE_H_
#define COMMON_ENDPOINT_BASE_H_

/* the base definitions for endopoints
 * this describes endpoint related definitions functions and flags
 */

#include "array.h"
#include "json.h"

struct _MESSAGE;

typedef struct _ENDPOINT{
	/* TODO: read-only */
	char *id;

	char *name;
	char *description; /* metadata */
	unsigned int type	:4;

	char *msg;
	char *resp;

	/* when msg is valid for module fcs or internal to the core
	 * relevant when an ep is invoked from exterior
	 * the def handler should be sent-to-app
	 * if its different add that fc as a handler*/
	void (*handler)(struct _MESSAGE*);

	/* other data that can be linked to the ep; eg, LOCAL_EP in core */
	void *data;

	/* if 0 -> messages are sent immediately
	 * if 1 -> messages are queued until get_message / get_response
	 */
	unsigned int queuing	:1;
}ENDPOINT;


/* flags for the EP type in the structure above */
#define EP_SRC			1
#define EP_SNK			2

#define EP_REQ			3
#define EP_RESP			4

#define EP_REQ_P		5
#define EP_RESP_P		6

#define EP_SS			7
#define EP_RR			8
#define EP_RR_P			9

#define EP_STR_SRC		10
#define EP_STR_SNK		11

#define EP_NONE			0


/* error codes for EP accessing and sending messages */
#define EP_OK 			0 /* operation succeeded */
#define EP_NO_EXIST 	1 /* wrong EP name */
#define EP_NO_ACCESS 	2 /* access not granted to perform operation */
#define EP_NO_SEND 		3 /* could not send on EP_SNK, EP_REP */
#define EP_NO_RECEIVE 	4 /* EP_SRC did not receive */
#define EP_NO_VALID 	5 /* message passed is not valid cf schema */
#define EP_DOESNT_MATCH_TYPE 	6 /* EP types are different */
#define EP_DOESNT_MATCH_HASH 	7 /* Hash values for msg or resp are different */
#define EP_ERROR		-1 /* Other error */

ENDPOINT* endpoint_init(const char* name, const char *description, int type,
						const char *msg_str, const char *resp_str,
						void (*callback_function)(struct _MESSAGE*),
						const char* id);

char *generate_id();

void endpoint_free(ENDPOINT* ep);

/*
 * EP type-related functionality
 */

/* transforms a string in a code */
int get_ep_type_int(const char *ep_type);

/* transforms a code into a string  */
char* get_ep_type_str(unsigned int ep_type);

/* transforms the type code into the string of the matching ep */
char* get_ep_type_matching_str(unsigned int ep_type);

/* checks if can send to the ep */
int ep_can_send(ENDPOINT *ep);

/* checks if can receive from the ep */
int ep_can_receive(ENDPOINT *ep);


#endif /* COMMON_ENDPOINT_BASE_H_ */
