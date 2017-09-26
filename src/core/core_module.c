/*
 * core_functions.c
 *
 *  Created on: 3 Apr 2016
 *      Author: Raluca Diaconu
 */

#include "core_module.h"

#include "endpoint.h"
#include "protocol.h"
#include "json.h"
#include "json_builds.h" /* for validate_msg(...) */
#include "message.h"
#include "state.h"
#include "rdcs.h"
#include "sync.h"

#include "com_wrapper.h"
#include "access_wrapper.h"

#include <utils.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


//TODO: REMOVE:
#include "manifest.h"

extern HashMap* locales;

extern STATE* app_state;

extern HashMap* rdcs;

extern int map_sync_pipe[2];

extern void core_on_data(COM_MODULE* module, int conn, const char* data);
extern void core_on_connect(COM_MODULE* module, int conn);
extern void core_on_disconnect(COM_MODULE* module, int conn);

int core_register_endpoint(const char* json)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
	JSON* ep_json = json_new(json);
    LOCAL_EP *lep = ep_local_new(ep_json, NULL);
    json_free(ep_json);
    if (!lep)
    	return -2;

    return 0;
}

void core_remove_endpoint(const char* ep_id)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    LOCAL_EP* lep = map_get(locales, (void*)ep_id);
    if (!lep)
        return;

    ep_local_remove(lep);
}

int core_map(LOCAL_EP* lep, COM_MODULE* com_module, const char* addr, JSON* ep_query, JSON* cpt_query)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (!lep)
        return EP_NO_EXIST;

    /* result from thread sync */
    char* result = NULL;
    /* new connection */
    int map_conn = -1;
    /* the new connection state */
    STATE* state_ptr = NULL;

    /* init sync between threads */
    sync_init(map_sync_pipe);

    /* attempt to connect; response is in another thread */
    map_conn = (*(com_module->fc_connect))(addr);

    /* wait for hello response from on_connect */
    result = sync_wait(map_sync_pipe[1]);

    if (map_conn <= 0)
        return EP_ERROR; // TODO: better return value

    /* wait for access ok from auth message */
    result = sync_wait(map_sync_pipe[1]);

    /* at this moment there must be a connection state instantiated */
    state_ptr = states_get(com_module, map_conn);
    if(state_ptr == NULL)
    {
    	return -1;
    }

    core_proto_map_to(state_ptr, lep, ep_query, cpt_query);
    result = sync_wait(map_sync_pipe[1]);

    if (state_ptr->flag != 0 || state_ptr->state == STATE_BAD)
    {
        ;
    }
    else
    {
        state_ptr->addr = strdup(addr);
    }

    return state_ptr->flag;
}

int core_map_all_modules(LOCAL_EP* lep, const char* addr, JSON* ep_query, JSON* cpt_query)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    int result = -1;

    int i;
    COM_MODULE* com_module;
    char* com_module_name;
    Array* com_modules_names = map_get_keys(com_modules);
    // change with ep->modules
    for (i = 0; i < map_size(com_modules); i++) {
        com_module_name = array_get(com_modules_names, i);
        com_module = map_get(com_modules, (void*)com_module_name);
        result = core_map(lep, com_module, addr, ep_query, cpt_query);
        if (result == 0)
            break;
    }

    array_free(com_modules_names);
    return result;
}

void core_map_lookup(LOCAL_EP* lep, JSON* ep_query, JSON* cpt_query, int max_maps)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (!lep)
        return;

    /* build the map query */
    JSON* map_query = json_build_map(lep, ep_query, cpt_query);

    /* build the message query_msg that is sent to all rdcs */
    char* query_str = json_to_str(map_query);
    MESSAGE* query_msg = message_new(query_str, MSG_REQ);
    free(query_str);

    /* send this query to all rdcs */
    // rdc_lookup_all(NULL, map_query);
    char* rdc_addr;
    RDC* r;
    int i;
    Array* keys = map_get_keys(rdcs);

    for (i = 0; i < array_size(keys); i++)
    {
        rdc_addr = array_get(keys, i);
        r = map_get(rdcs, (void*)rdc_addr);

        /* map to the rdc's lookup ep  TODO: add query*/
        rdc_lookup(r, query_msg);
        int maps = 0;
        int j;

        for (j = 0; j < array_size(r->lookup_result); j++)
        {
        	JSON* ep = array_get(r->lookup_result, j);
        	Array* all_addrs = json_get_jsonarray(ep, "com_modules");
        	int k;
			for (k = 0; k < array_size(all_addrs); k++)
			{
				JSON* module_addr = array_get(all_addrs, k);
				char* module_name = json_get_str(module_addr, "name");
				COM_MODULE* module = com_get_module(module_name);
				char* addr = json_get_str(module_addr, "address");
				if (core_map(lep, module, addr, ep_query, cpt_query) == 0)
					return;
			}

            // core_map(lep, tcp_module, addr, ep_query, cpt_query);
            //core_map_all_modules(lep, addr, ep_query, cpt_query);

            // Limit the number of maps performed to max_maps.
            maps++;
            if (max_maps > 0 && maps == max_maps) {
                break;
            }
        }
    }

    message_free(query_msg);
}

/* wrapper for ep_unmap_addr */
int core_unmap(LOCAL_EP* lep, const char* addr)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    ep_unmap_addr(lep, addr);

    return 0;
}

int core_unmap_connection(LOCAL_EP* lep, COM_MODULE* module, int conn)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    STATE* state_ptr = states_get(module, conn);
    if(! state_ptr)
    	return -1;

    ep_unmap_send(lep, state_ptr);

    return 0;
}

/* a wrapper for ep_unmap_all */
int core_unmap_all(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    ep_unmap_all(lep);

    return 0;
}

/* TODO */
int core_divert(LOCAL_EP* lep, const char* ep_id_from, const char* addr)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (!lep)
        return EP_NO_EXIST;

    int result;
    result = core_unmap(lep, ep_id_from);
    if (result)
        return result;
    result = core_map_all_modules(lep, addr, NULL, NULL);

    return result; /* just for the sake of symmetry */
}


int core_ep_send_message(LOCAL_EP* lep, const char* msg_id, const char* msg)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    if (!ep_can_send(lep->ep))
        return EP_NO_SEND;

    //JSON* msg_json = json_new(msg);

    MESSAGE* msg_msg = message_parse(msg);
    //TODO: make correct fcs for stream/src
    if (json_validate_message(lep, msg_msg->_msg_json))
        return EP_NO_VALID;

    ep_send_message(lep, msg_msg);

    json_free(msg_msg->_msg_json);
    message_free(msg_msg);

    return 0;
}


int core_ep_send_request(LOCAL_EP* lep, const char* req_id, const char* req)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (!(lep->ep->type == EP_REQ || lep->ep->type == EP_REQ_P || lep->ep->type == EP_RR)) {
        return EP_NO_RECEIVE;
    }
    JSON* req_json = json_new(req);

    ep_send_json(lep, req_json, req_id, MSG_REQ);

    json_free(req_json);

    return 0;
}

int core_ep_send_response(LOCAL_EP* lep, const char* req_id, const char* resp)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    if (!(lep->ep->type == EP_RESP || lep->ep->type == EP_RESP_P || lep->ep->type == EP_RR)) {
        return EP_NO_SEND;
    }

    JSON* resp_json = json_new(resp);

    // MESSAGE *resp_msg = message_parse(resp);
    // resp_msg->msg = strdup_null(resp);

    ep_send_json(lep, resp_json, req_id, MSG_RESP_LAST);

    json_free(resp_json);

    // message_free(resp_msg);
    return 0;
}

int core_ep_more_messages(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    if (lep->ep->type != EP_SNK && lep->ep->type != EP_SS)
        return -1;

    return array_size(lep->messages);
}

int core_ep_more_requests(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (lep->ep->type != EP_REQ && lep->ep->type != EP_REQ_P && lep->ep->type != EP_RR)
        return -1;

    return array_size(lep->messages);
}

int core_ep_more_responses(LOCAL_EP* lep, const char* req_id)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (lep->ep->type != EP_RESP && lep->ep->type != EP_RESP_P && lep->ep->type != EP_RR)
        return -1;

    int nb_msgs = 0;
    int i;
    for (i = 0; i < array_size(lep->responses); i++)
    {
        MESSAGE* msg = array_get(lep->responses, i);
        if (strcmp(msg->msg_id, req_id) == 0)
        {
            nb_msgs += 1;
        }
    }
    return nb_msgs;
}

MESSAGE* core_ep_fetch_message(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    if (lep->ep->type != EP_SNK && lep->ep->type != EP_SS)
        return NULL;


    // TODO
    if (array_size(lep->messages) <= 0)
        sleep(1);

    if (array_size(lep->messages) > 0)
    {
        MESSAGE* msg = array_get(lep->messages, 0);
        array_remove_index(lep->messages, 0);

        return msg;
    }
    else
        return NULL;
}

MESSAGE* core_ep_fetch_request(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (lep->ep->type != EP_REQ && lep->ep->type != EP_REQ_P && lep->ep->type != EP_RR)
        return NULL;


    if (array_size(lep->messages) <= 0)
        sleep(1);

    if (array_size(lep->messages) > 0)
    {
        MESSAGE* msg = array_get(lep->messages, 0);
        state_send_message(app_state, msg);
        array_remove_index(lep->messages, 0);

        return msg;
    }
    else
        return NULL;

    return NULL;
}

MESSAGE* core_ep_fetch_response(LOCAL_EP* lep, const char* req_id)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (lep->ep->type != EP_RESP && lep->ep->type != EP_RESP_P && lep->ep->type != EP_RR)
        return NULL;

    if (array_size(lep->responses) <= 0)
        sleep(1);

    int i;
    for (i = 0; i < array_size(lep->responses); i++)
    {
        MESSAGE* msg = array_get(lep->responses, i);
        if (strcmp(msg->msg_id, req_id) == 0)
        {
            state_send_message(app_state, msg); // TODO: send to app
            array_remove_index(lep->responses, i);

            return msg;
        }
    }
    return NULL;
}


void core_ep_stream_start(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
	if(lep == NULL)
		return;

    if(lep->ep->type != EP_STR_SRC && lep->ep->type != EP_STR_SNK)
    	return;

    //build a MSG_STR_CMD message with flag set on STOP
    JSON* msg_json = json_new(NULL);
    json_set_int(msg_json, "command", 1);
    ep_send_json(lep, msg_json, message_generate_id(), MSG_STREAM_CMD);

    lep->flag = 1;
}

void core_ep_stream_stop(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
	if(lep == NULL)
		return;

    if(lep->ep->type != EP_STR_SRC && lep->ep->type != EP_STR_SNK)
    	return;

    //build a MSG_STR_CMD message with flag set on STOP
    JSON* msg_json = json_new(NULL);
    json_set_int(msg_json, "command", 0);
    ep_send_json(lep, msg_json, message_generate_id(), MSG_STREAM_CMD);

    lep->flag = 0;
}

void core_ep_stream_send(LOCAL_EP* lep, const char* msg)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
	if(lep == NULL)
		return;

    if(lep->ep->type != EP_STR_SRC || lep->flag == 0)
    	return;


    //build a MSG_STREAM message
    JSON* msg_json = json_new(NULL);
    json_set_str(msg_json, "stream", msg);

    MESSAGE* msg_msg = message_new_json(msg_json, MSG_STREAM);

    ep_send_message(lep, msg_msg);
    printf("sent\n\n");

    json_free(msg_json);
    message_free(msg_msg);


    //ep_send_json(lep, msg_json, message_generate_id(), MSG_STREAM);


    //lep->flag = 1;
}

void core_ep_set_access(LOCAL_EP* lep, const char* subject)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    if (lep == NULL || subject == NULL) {
        //slog(SLOG_WARN, SLOG_WARN, "CORE FUNC: null arguments ignored");
        return;
    }

    //TODO: restore
    //acl_add_access(lep->id, subject, ACL_OP_MAP);
}

void core_ep_reset_access(LOCAL_EP* lep, const char* subject)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    if (lep == NULL || subject == NULL) {
        //slog(SLOG_WARN, SLOG_WARN, "CORE FUNC: null arguments ignored");
        return;
    }

    //TODO: restore
    //acl_rm_access(lep->id, subject, ACL_OP_MAP);
}

int core_add_manifest(const char* msg)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    MESSAGE* md_msg = message_parse(msg);
    JSON* md_json = md_msg->_msg_json;

    int ret = manifest_update(md_json);

    //json_free(md_json);
    message_free(md_msg);

    return ret;
}

// TODO: rename with get_own_metadata
// TODO: add function get_metadata(char* addr)
char* core_get_manifest()
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    JSON* elem_md = manifest_get(MANIFEST_SHORT);
    char* md_str = json_to_str(elem_md);
    json_free(elem_md);
    return md_str;
}

int core_add_rdc(COM_MODULE* module, const char* addr)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
	if(module == NULL)
	{
		return -1;
	}


    RDC* r = rdc_new(module, addr);
    rdcs_set_addr(module, addr, r);

    return (r==NULL);
}

void core_rdc_register(COM_MODULE* com_module, const char* addr)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (addr == NULL && com_module == NULL)
    {
        rdc_register_all(RDC_REGISTER);
        return;
    }
    if (addr == NULL || com_module == NULL)
    {
        return;
    }
    if ((*(com_module->fc_is_valid_address))(addr))
        rdc_register_addr_all(addr, RDC_REGISTER);
    else
        ;
}

void core_rdc_unregister(const char* addr)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    if (addr == NULL)
        rdc_register_all(RDC_UNREGISTER);
    else {
        RDC* r = map_get(rdcs, (void*)addr);
        if (r != NULL) {
            rdc_register_addr_all(addr, RDC_UNREGISTER);
            // TODO: change witth rdc state. or so
        }
    }
}

void core_add_filter(LOCAL_EP* lep, const char* filter)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    array_add(lep->filters, strdup_null(filter));
}

void core_reset_filter(LOCAL_EP* lep, Array* new_filters)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    array_free(lep->filters);

    if (new_filters != NULL)
        lep->filters = new_filters;
    else
        lep->filters = array_new(ELEM_TYPE_STR);
}

void core_terminate()
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    rdc_register_all(RDC_UNREGISTER);

    int i;
    LOCAL_EP* lep;
    char* key;
    Array* keys = map_get_keys(locales);
    for (i = 0; i < array_size(keys); i++)
    {
        key = array_get(keys, i);
        lep = map_get(locales, (void*)key);
        ep_unmap_all(lep);
        if(lep->fifo != 0)
        {
        	core_ep_stream_stop(lep);
        }
    }

    COM_MODULE* com_module;
    char* com_module_name;
    Array* com_modules_names = map_get_keys(com_modules);
    for (i = 0; i < map_size(com_modules); i++)
    {
        com_module_name = array_get(com_modules_names, i);
        com_module = map_get(com_modules, (void*)com_module_name);
        com_module_free(com_module);
    }

	close(lep->fifo);
	//unlink(lep->fifo_name);

    /* close app sockpair */
    (*(app_state->module->fc_connection_close))(app_state->conn);

    exit(0);
}


/*after connecting to the api */

int core_load_com_module(const char* lib_path, const char* config_json)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

    COM_MODULE* com_module = com_module_new(lib_path, config_json); // com_get_module(path);//change after socketpair with app

    if (com_module == NULL) {
        //slog(SLOG_ERROR, SLOG_ERROR, "CORE FUNC: could not load com module %s", lib_path);
        return -1;
    }

    (*(com_module->fc_set_on_data))((void (*)(void *, int, const char *))core_on_data);
    (*(com_module->fc_set_on_connect))((void (*)(void *, int))core_on_connect);
    (*(com_module->fc_set_on_disconnect))((void (*)(void *, int))core_on_disconnect);

    map_update(com_modules, (void*)com_module->name, (void*)com_module);

    return 0;
}

int core_load_access_module(const char* path, const char* config_json)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
    return access_load_module(path, config_json);
}

/* connections */

char* core_ep_get_all_connections(LOCAL_EP* lep)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);

	if(lep==NULL)
		return NULL;

	Array* result_array = array_new(ELEM_TYPE_PTR);
	JSON* result_json = json_new(NULL);
	int i;
	STATE* mapping_state;
	JSON* mapping_json;
	for(i=0; i<array_size(lep->mappings_states); i++)
	{
		mapping_state = array_get(lep->mappings_states, i);
		mapping_json = json_new(NULL);
		json_set_str(mapping_json, "module", mapping_state->module->name);
		json_set_int(mapping_json, "conn", mapping_state->conn);

		array_add(result_array, mapping_json);
	}

	json_set_array(result_json, "all_mappings", result_array);
	char* result = json_to_str(result_json);

	return json_to_str(result_json);
}

char* core_get_remote_manifest(COM_MODULE* module, int conn)
{
	slog(SLOG_DEBUG, "CORE: %s", __func__);
	if(module == NULL || conn ==0)
		return NULL;

	STATE* state = states_get(module, conn);
	if (state == NULL)
		return NULL;

	char* result=NULL;
	JSON* manifest = json_new(NULL);
	if(state->cpt_manifest)
		json_set_json(manifest, "component", state->cpt_manifest);
	if(state->ep_metadata)
		json_set_json(manifest, "endpoint", state->ep_metadata);
	result = json_to_str(manifest);

	return result;
}


/*
 * this function allocates memory and returns a pointer
 * don't forget to free
 */
char* _core_get_id(char id[50],
		const char* module_id, const char* function_id, const char* return_type)
{
    //char* id;
    //id = (char*)malloc(50 * sizeof(char));

    strcpy(id, module_id);
    strcat(id, function_id);
    strcat(id, return_type);

    return id;
}
