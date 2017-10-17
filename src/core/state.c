/*
 * state.c
 *
 *  Created on: 6 Jul 2016
 *      Author: Raluca Diaconu
 */

#include "state.h"

#include "message.h"
#include <hashmap.h>
#include <stdio.h>

extern STATE* app_state;

#define BUFFER_FINAL 	0
#define BUFFER_JSON  	1
#define BUFFER_STR_2 	2
#define BUFFER_ESC_2 	3
#define BUFFER_STR_1 	4
#define BUFFER_ESC_1 	5

BUFFER* buffer_new(STATE* state)
{
	BUFFER* buffer = (BUFFER*) malloc(sizeof(BUFFER));

	buffer->data = NULL;
	buffer->size = 0;

	buffer->buffer_state = 0;
	buffer->brackets = 0;

	buffer->state = state;

	return buffer;
}

void buffer_free(BUFFER* buffer)
{
	if(buffer->data)
		free(buffer->data);
	buffer->data = NULL;
	buffer->size = 0;

	buffer->buffer_state = 0;
	buffer->brackets = 0;
}

void buffer_reset(BUFFER* buffer)
{
	if(buffer->data)
		free(buffer->data);

	buffer->data = (char*)(malloc(1));
	buffer->data[0]='\0';
}

void buffer_set(BUFFER* buffer, const void* new_buf, unsigned int new_start, unsigned int new_end)
{
	/*printf("----old %d *%*s* \n"
			"----new %d *%*s*\n\n",
			buffer->size, buffer->size, buffer->data,
			new_end-new_start, new_end-new_start, new_buf+new_start);
	*/
	char* old_buf = buffer->data;
	int new_size = new_end-new_start;
	buffer->data = (char*) malloc(buffer->size + new_size + 1);
	memcpy(buffer->data,             old_buf,           buffer->size);
	memcpy(buffer->data+buffer->size, new_buf+new_start, new_size   );
	buffer->size = buffer->size + new_size;
	buffer->data[buffer->size] = '\0';

	free(old_buf);
}

void buffer_app_set(BUFFER *buffer)
{
	static int first = 1;
	if (first)
	{
		first = 0;
		return;
	}

	char module_id[5], function_id[18], return_type[4], msg_id[11];
	void* arg = NULL;
	int cmd=0;
	sscanf(buffer->data, "{{%d}{%4s}{%17s}{%3s}{%10s}",
			&cmd, module_id, function_id, return_type, msg_id);

	char arg_void[buffer->size-46];
	memcpy(arg_void, buffer->data+48, buffer->size-48);
	arg_void[buffer->size-48]='\0';

	//printf (">>>> %d--%s--%s--%s--%s\n",
	//		cmd,module_id, function_id, return_type, msg_id);
	//printf (">>>>%s--\n", arg_void);


	Array *arg_array=array_new(ELEM_TYPE_STR);
	int size=0;
	char* arg1;
	char* head=arg_void;


	sscanf(head, "{%010d}", &size);
	while(size){
		arg1=(char*)malloc(size*sizeof(char)+1);
		memcpy(arg1, head+12, size);
		arg1[size]='\0';
		//sscanf(arg_void, "%*s", size, arg1);

		//printf ("   >>>>size: %d--\n", size);
		//printf ("   >>>>text: %s--\n", arg1);

		array_add(arg_array, arg1);
		head = head+size+12;

		size = 0;
		sscanf(head, "{%010d}", &size);
	};

	core_on_component_message(NULL, msg_id,
			module_id, function_id, return_type,
			arg_array);

}

void buffer_update(BUFFER* buffer, const void* new_data, unsigned int new_size) //size should be fixed?
{
	unsigned int i=0;
	unsigned int word_start = i;
	unsigned int word_end = i;

	for(i=0; i<new_size; i++)
	{
		switch(buffer->buffer_state){
			case BUFFER_FINAL:
				switch (((char*)new_data)[i])
				{
					case '{':
						buffer->brackets += 1;
						word_start = i;
						buffer->buffer_state = BUFFER_JSON;
						break;
					/* ignore spaces and new lines */
					case ' ': case '\n': case '\r':
						break;
					default:
						//slog(SLOG_ERROR, "%c", new_data[i]);
						continue;

				}
				break;

			case BUFFER_JSON:
				switch (((char*)new_data)[i])
				{
					case '{':
						buffer->brackets += 1;
						//data->buffer_state = 1;
						break;
					case '}':
						buffer->brackets -= 1;

						if(buffer->brackets == 0) // finished 1 word
						{
							buffer->buffer_state = BUFFER_FINAL;
							word_end = i+1;
							buffer_set(buffer, new_data, word_start, word_end);
							/* apply the callback for this connection */
							if(buffer->state == app_state)
							{
								buffer_app_set(buffer);
							}
							else
							{
								MESSAGE* msg = message_parse(buffer->data);
								JSON* js=msg->_msg_json;
								(*buffer->state->on_message)(buffer->state, msg);
								json_free(js);
								message_free(msg);
							}

							buffer_reset(buffer);
							buffer->size = 0;
							word_start = i+1;
						}

						break;
					case '\"':
						buffer->buffer_state = BUFFER_STR_2;
						break;
					case '\'':
						buffer->buffer_state = BUFFER_STR_1;
						break;

					default:
						continue;
				}
				break;
			case BUFFER_STR_1:
				switch (((char*)new_data)[i])
				{
					case '\\':
						buffer->buffer_state = BUFFER_ESC_1;
						break;
					case '\'':
						buffer->buffer_state = BUFFER_JSON;
						break;
					default:
						continue;
				}
				break;
			case BUFFER_ESC_1:
				buffer->buffer_state = BUFFER_STR_1;
				break;
			case BUFFER_STR_2:
				switch (((char*)new_data)[i])
				{
					case '\\':
						buffer->buffer_state = BUFFER_ESC_2;
						break;
					case '\"':
						buffer->buffer_state = BUFFER_JSON;
						break;
					default:
						continue;
				}
				break;
			case BUFFER_ESC_2:
				buffer->buffer_state = BUFFER_STR_2;
				break;
			default: //impossible
				return;
		}
	}

	if(word_start<new_size && buffer->buffer_state != BUFFER_FINAL)
		buffer_set(buffer, new_data, word_start, new_size);
}



STATE* state_new(COM_MODULE* module, int conn, int state)
{
	if(module == NULL || conn<0)
		return NULL;

	STATE* state_ptr = (STATE*)malloc(sizeof(STATE));

	state_ptr->module = module;
	state_ptr->access_module = NULL;
	state_ptr->conn = conn;
	state_ptr->state = state;
	state_ptr->addr = NULL;
	state_ptr->lep = NULL;

	state_ptr->tokens = array_new(ELEM_TYPE_STR);

	state_ptr->cpt_manifest = NULL;
	state_ptr->ep_metadata = NULL;

	state_ptr->flag = 0; /* the good flag */
	state_ptr->is_auth = 0;
	state_ptr->am_auth = 0;

	state_ptr->buffer = buffer_new(state_ptr);
	state_ptr->on_message = NULL;
	return state_ptr;
}


void state_free(STATE* state)
{
	if(state == NULL)
		return;
	json_free(state->cpt_manifest);
	json_free(state->ep_metadata);
	array_free(state->tokens);
	free(state->addr);
	//data_free(state->data);

	buffer_free(state->buffer);
	free(state);
}


int state_send_message(STATE* state, MESSAGE* msg)
{
//	if(state == NULL)
//		return STATE_BAD;

	char* msg_str = message_to_str(msg);

	int result = (*(state->module->fc_send_data))(state->conn, msg_str);

	free(msg_str);

	return result;
}

int state_send_json(STATE* state, const char* id, JSON* json, int status)
{
	if(state == NULL)
		return STATE_BAD;

	MESSAGE* msg = message_new_id_json(id, json, status);
	int result = state_send_message(state, msg);

	message_free(msg);

	return result;
}
const char* state_get_str(int state)
{
	switch (state) {
		case STATE_HELLO_S:
			return "HELLO_S";
		case STATE_HELLO_ACK_S:
			return "HELLO_ACK_S";
		case STATE_HELLO_2:
			return "HELLO_2";
		case STATE_AUTH:
			return "STATE_AUTH";
		case STATE_AUTH_ACK:
			return "STATE_AUTH_ACK";
		case STATE_AUTH_2:
			return "STATE_AUTH_2";
		case STATE_MAP:
			return "MAP";
		case STATE_MAP_ACK:
			return "MAP_ACK";
		case STATE_EXT_MSG:
			return "EXT_MSG";
		case STATE_APP_MSG:
			return "APP_MSG";
		case STATE_FIRST_MSG:
			return "FIRST_MSG";
		case 0:
			return "BAD";
		default:
			return "Unknown";
	}
}


int init_states()
{
	/* key is the name of the module */
	conn_state = map_new(KEY_TYPE_STR);
	return (conn_state == NULL);
}

STATE* states_get(COM_MODULE* module, int conn)
{
	/*slog(SLOG_INFO, "getting state, size: %d", map_size(conn_state));*/

	char conn_str[105];
	sprintf(conn_str, "%s:%d", module->name, conn);
	STATE *state_ptr = map_get(conn_state, conn_str);
	/*slog(SLOG_INFO, "state(%s:%d) = (%d:%s)", module->name, conn,
				state_ptr->state, state_get_str(state_ptr->state));*/

	return state_ptr;
}

int states_set(COM_MODULE* module, int conn, STATE* state)
{
	if(state == NULL)
	{
		return -1;
	}

	char* conn_str=(char*) malloc(105*sizeof(char));
	sprintf(conn_str, "%s:%d", module->name, conn);

	return map_insert(conn_state, conn_str, state);
}
