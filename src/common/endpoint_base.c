/*
 * endpoint_base.c
 *
 *  Created on: 8 Jun 2016
 *      Author: Raluca Diaconu
 */


#include "endpoint_base.h"

#include "hashmap.h"
#include "array.h"
#include <utils.h>

#include <string.h>
#include "environment.h" /* only for EP_UID_SIZE */

struct _MESSAGE;

char *randstring(size_t length);

ENDPOINT* endpoint_init(const char* name, const char *description, int type,
						const char *msg_str, const char *resp_str,
						void (*callback_function)(struct _MESSAGE*),
						const char* id)
{
	ENDPOINT *ep;

	/* basic param init */
	ep = (ENDPOINT*) calloc(1, sizeof(ENDPOINT));

	if(name)
		ep->name = strdup(name);
	if(description)
		ep->description = strdup(description);
	ep->handler = callback_function;


	if(id == NULL)
		ep->id = generate_id();
	else
		ep->id = strdup(id);

	if(type <= 0 || type >= 12)
	{
		//slog(SLOG_ERROR, "ENDPOINT: wrong ep_type: %d ", type);
		endpoint_free(ep);
		return NULL;
	}
	ep->type = type;

	/* schema strings management */
	if(type == EP_STR_SRC || type == EP_STR_SNK)
	{
		ep->msg = NULL;
	}
	else if(msg_str == NULL)
	{
		//slog(SLOG_ERROR, "ENDPOINT: no msg specified");
		printf("no msg specified\n");
		endpoint_free(ep);
		return NULL;
	}
	if(msg_str)
		ep->msg = strdup(msg_str);

	if(	type == EP_REQ || type == EP_RESP ||
		type == EP_REQ_P || type == EP_RESP_P ||
		type == EP_RR || type == EP_RR_P)
	{
		if( resp_str == NULL)
		{
			//slog(SLOG_ERROR, "ENDPOINT: no response msg specified");
			endpoint_free(ep);
			return NULL;
		}
		if(resp_str)
			ep->resp = strdup(resp_str);
	} else
	{
		ep->resp = NULL;
	}

	return ep;
}


void endpoint_free(ENDPOINT* ep)
{
	if(ep == NULL)
		return;

	free(ep->id);
	free(ep->name);
	free(ep->description);
	free(ep->data);
	free(ep->msg);
	free(ep->resp);
}



char* get_ep_type_str(unsigned int ep_type)
{
	char *ep_type_str = (char*)malloc(8*sizeof(char));

	if(ep_type == EP_SRC)
	{
		strcpy(ep_type_str, "src");
	}
	else if(ep_type == EP_SNK)
	{
		strcpy(ep_type_str, "snk");
	}
	else if(ep_type == EP_REQ)
	{
		strcpy(ep_type_str, "req");
	}
	else if(ep_type == EP_RESP)
	{
		strcpy(ep_type_str, "resp");
	}
	else if(ep_type == EP_REQ_P)
	{
		strcpy(ep_type_str, "req+");
	}
	else if(ep_type == EP_RESP_P)
	{
		strcpy(ep_type_str, "resp+");
	}
	else if(ep_type == EP_RR)
	{
		strcpy(ep_type_str, "rr");
	}
	else if(ep_type == EP_RR_P)
	{
		strcpy(ep_type_str, "rr+");
	}
	else if(ep_type == EP_SS)
	{
		strcpy(ep_type_str, "ss");
	}
	else if(ep_type == EP_STR_SRC)
	{
		strcpy(ep_type_str, "str_src");
	}
	else if(ep_type == EP_STR_SNK)
	{
		strcpy(ep_type_str, "str_snk");
	}
	else
	{
		ep_type_str = NULL;
	}
	return ep_type_str;
}

char* get_ep_type_matching_str(unsigned int ep_type)
{
	char *ep_type_str = (char*)malloc(8*sizeof(char));

	if(ep_type == EP_SRC)
	{
		strcpy(ep_type_str, "snk");
	}
	else if(ep_type == EP_SNK)
	{
		strcpy(ep_type_str, "src");
	}
	else if(ep_type == EP_REQ)
	{
		strcpy(ep_type_str, "resp");
	}
	else if(ep_type == EP_RESP)
	{
		strcpy(ep_type_str, "req");
	}
	else if(ep_type == EP_REQ_P)
	{
		strcpy(ep_type_str, "resp+");
	}
	else if(ep_type == EP_RESP_P)
	{
		strcpy(ep_type_str, "req+");
	}
	else if(ep_type == EP_RR)
	{
		strcpy(ep_type_str, "rr");
	}
	else if(ep_type == EP_RR_P)
	{
		strcpy(ep_type_str, "rr+");
	}
	else if(ep_type == EP_SS)
	{
		strcpy(ep_type_str, "ss");
	}
	else if(ep_type == EP_STR_SRC)
	{
		strcpy(ep_type_str, "str_snk");
	}
	else if(ep_type == EP_STR_SNK)
	{
		strcpy(ep_type_str, "str_src");
	}
	else
	{
		ep_type_str = NULL;
	}
	return ep_type_str;
}

int get_ep_type_int(const char* ep_type)
{
	int ep_type_int = 0;

	if(strcmp(ep_type, "src") == 0 )
	{
		ep_type_int = EP_SRC;
	}
	else if(strcmp(ep_type, "snk") == 0)
	{
		ep_type_int = EP_SNK;
	}
	else if(strcmp(ep_type, "req") == 0)
	{
		ep_type_int = EP_REQ;
	}
	else if(strcmp(ep_type, "resp") == 0)
	{
		ep_type_int = EP_RESP;
	}
	else if(strcmp(ep_type, "req+") == 0)
	{
		ep_type_int = EP_REQ_P;
	}
	else if(strcmp(ep_type, "resp+") == 0)
	{
		ep_type_int = EP_RESP_P;
	}
	else if(strcmp(ep_type, "rr") == 0)
	{
		ep_type_int = EP_RR;
	}
	else if(strcmp(ep_type, "rr+") == 0)
	{
		ep_type_int = EP_RR_P;
	}
	else if(strcmp(ep_type, "ss") == 0)
	{
		ep_type_int = EP_SS;
	}
	else if(strcmp(ep_type, "str_src") == 0)
	{
		ep_type_int = EP_STR_SRC;
	}
	else if(strcmp(ep_type, "str_snk") == 0)
	{
		ep_type_int = EP_STR_SNK;
	}
	return ep_type_int;
}

int ep_can_send(ENDPOINT *ep)
{
	if(	ep->type == EP_SRC ||
		ep->type == EP_REQ ||
		ep->type == EP_REQ_P ||
		ep->type == EP_RR ||
		ep->type == EP_RR_P ||
		ep->type == EP_SS ||
		ep->type == EP_STR_SRC)
			return 1;
	return 0;
}


int ep_can_receive(ENDPOINT *ep)
{
	if(	ep->type == EP_SNK ||
		ep->type == EP_RESP ||
		ep->type == EP_RESP_P ||
		ep->type == EP_RR ||
		ep->type == EP_RR_P ||
		ep->type == EP_SS ||
		ep->type == EP_STR_SNK)
			return 1;
	return 0;
}

char *generate_id()
{
	return randstring(EP_UID_SIZE);
}


