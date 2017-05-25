/*
 * default_eps.c
 *
 *  Created on: 1 Aug 2016
 *      Author: rad
 */

#include "default_eps.h"

#include "message.h"
#include "endpoint.h"
#include "hashmap.h"
#include "core_module.h"
#include "rdcs.h"
#include "json.h"
#include "json_builds.h"
#include "manifest.h"
#include "state.h"
#include <slog.h>
#include <utils.h>

/* global stuff declared in core.c */
//extern HashMap *endpoints;
extern HashMap *locales;

extern LOCAL_EP *ep_reg_rdc;
extern LOCAL_EP *ep_lookup;

LOCAL_EP *default_ep_map;
LOCAL_EP *default_ep_map_lookup;
LOCAL_EP *default_ep_md;
LOCAL_EP *default_ep_terminate;

LOCAL_EP *load_com_module_ep;
LOCAL_EP *load_access_module_ep;
LOCAL_EP *set_credentials_ep;

extern HashMap* rdcs;

/* handlers */

void map_handler(MESSAGE* msg)
{
	JSON* map_json = json_new(msg->msg);

	Array* ep_array = json_get_array(map_json, "endpoint");
	JSON* ep_json = json_new(NULL);
	json_set_array(ep_json, NULL, ep_array);

	char* address = json_get_str(map_json, "address");

	Array* ep_query_array = json_get_array(map_json, "ep_query");
	Array* cpt_query_array = json_get_array(map_json, "cpt_query");
	JSON* ep_query_json = json_new(NULL);
	json_set_array(ep_query_json, NULL, ep_query_array);
	JSON* cpt_query_json = json_new(NULL);
	json_set_array(cpt_query_json, NULL, cpt_query_array);

	LOCAL_EP *ep_map = endpoint_query(ep_json);

	int result;

	if(ep_map)
		result = core_map_all_modules(ep_map, address, ep_query_json, cpt_query_json);
	else
		result = -1;
	JSON *fc_res_json = json_new(NULL);
	json_set_int(fc_res_json, "return_value", result);

	//message_send_json(msg->msg_id, fc_res_json, msg->conn, MSG_RESP_LAST);

	ep_unmap_all(default_ep_map);
}

void map_lookup_handler(MESSAGE* msg)
{
	JSON *map_json = json_new(msg->msg);

	Array* ep_array = json_get_array(map_json, "endpoint");
	JSON* ep_json = json_new(NULL);
	json_set_array(ep_json, NULL, ep_array);

	JSON* ep_query = json_get_json(map_json, "ep_query");
	JSON* cpt_query = json_get_json(map_json, "cpt_query");
	//JSON* query_json = json_new(NULL);
	//json_set_array(query_json, NULL, ep_query_array, cpt_query_array, 0);


	LOCAL_EP *ep_map = endpoint_query(ep_json);


	int result;

	if(ep_map)
	{
		core_map_lookup(ep_map, ep_query, cpt_query, 0);
		result = 0;
	}
	else
		result = -1;
	JSON *fc_res_json = json_new(NULL);

	json_set_int(fc_res_json, "return_value", result);

	ep_unmap_all(default_ep_map_lookup);
}

void unmap_handler(MESSAGE* msg)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "CORE: map handler");
	JSON *unmap_json = json_new(msg->msg);
	JSON *ep_json = json_get_json(unmap_json, "endpoint");
	char* ep_name = json_get_str(ep_json, "ep_name");
	//char* address = json_get_str(map_json, "address");

	LOCAL_EP *ep_unmap = NULL;
	LOCAL_EP *lep;
	int i;
	Array *_keys = map_get_keys(locales);

	for(i=0;i<array_size(_keys); i++)
	{
		lep = map_get(locales, array_get(_keys, i));
		if (strcmp(lep->ep->name, ep_name) == 0)
		{
			ep_unmap = lep;
			break;
		}
	}

	core_unmap_all(ep_unmap->ep->id);
}

void lookup_handler(MESSAGE* msg)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "CORE: lookup handler %s", message_to_str(msg));

	COM_MODULE* module = com_get_module(msg->module);
	if(module == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"CORE: lookup handler, bad module name %s",
				msg->module);
		return;
	}

	STATE* state_ptr = states_get(module, msg->conn);
	if(state_ptr == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"CORE: lookup handler, bad connection %s:%d",
				msg->module, msg->conn);
		return;
	}

	RDC* r = rdcs_get_addr(module, state_ptr->addr);
	if(r == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"CORE: lookup handler, bad rdc addr %s:%d",
				module->name, state_ptr->addr);
		return;
	}

	JSON* lookup_json = json_new(msg->msg);
	Array *results_array  = json_get_jsonarray(lookup_json, "results");


	if(r->state != RDC_STATE_LOOKUP)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"CORE: lookup handler, rdc was not called for lookup, check addresses");
		return;
	}

	array_free(r->lookup_result);
	r->lookup_result = results_array; //json_get_array(lookup_json, NULL);

	r->state = RDC_STATE_REG;
	ep_unmap_all(ep_lookup);
}

/* sends manifest then disconnects */
void md_handler(MESSAGE* msg)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "CORE: md handler %s", message_to_str(msg));

	/* message can be empty */
	JSON *md_json = manifest_get(MANIFEST_FULL);

	MESSAGE *md_msg = message_new(json_to_str(md_json), 0);
	md_msg->msg_id = msg->msg_id;

	MESSAGE* resp_msg = message_new(message_to_str(md_msg), MSG_RESP_LAST);
	resp_msg->msg_id = msg->msg_id;
	char* resp_str = message_to_str(resp_msg);

	//ep_send_str_message(default_ep_md, resp_str); //TODO: check
	int r = ep_send_json(default_ep_md,
			json_new(resp_msg->msg),
			resp_msg->msg_id,
			MSG_RESP_LAST);

	free(resp_str);
	message_free(resp_msg);

	json_free(md_json);

	ep_unmap_all(default_ep_md);
}

void add_rdc_handler(MESSAGE* msg)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "CORE: add rdc handler %s", message_to_str(msg));
	JSON *add_rdc_json = json_new(msg->msg);
	char* rdc_module_name = strdup_null(json_get_str(add_rdc_json, "module"));
	char* rdc_addr = strdup_null(json_get_str(add_rdc_json, "address"));

	COM_MODULE* module = com_get_module(rdc_module_name);

	core_add_rdc(module, rdc_addr);

	json_free(add_rdc_json);
}

void terminate_handler(MESSAGE* msg)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "CORE: terminate handler %s", message_to_str(msg));
	core_terminate();
}

void load_com_module_ep_handler(MESSAGE* msg)
{
	JSON* msg_json = json_new(msg->msg);
	char* module_path = json_get_str(msg_json, "module_path");
	char* config_path = json_get_str(msg_json, "config_path");

	core_load_com_module(module_path, config_path);
}

void load_access_module_ep_handler(MESSAGE* msg)
{
	JSON* msg_json = json_new(msg->msg);
	char* module_path = json_get_str(msg_json, "module_path");
	char* config_pth = json_get_str(msg_json, "config_pth");

	core_load_com_module(module_path, config_pth);
}

void set_credentials_ep_handler(MESSAGE* msg)
{

}

/* all default ep registration */
void register_default_endpoints()
{
	JSON* from_file_json;

	from_file_json = json_load_from_file("default_eps/reg_rdc.json");
	ep_reg_rdc = ep_local_new(from_file_json, NULL);
	ep_reg_rdc->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/lookup_ep.json");
	ep_lookup = ep_local_new(from_file_json, &lookup_handler);
	ep_lookup->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/map_ep.json");
	default_ep_map = ep_local_new(from_file_json, &map_handler);
	default_ep_map->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/map_lookup_ep.json");
	default_ep_map_lookup = ep_local_new(from_file_json, &map_lookup_handler);
	default_ep_map_lookup->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/unmap_ep.json");
	ep_local_new(from_file_json, &unmap_handler)->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/add_rdc_ep.json");
	ep_local_new(from_file_json, &add_rdc_handler)->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/md_ep.json");
	default_ep_md = ep_local_new(from_file_json, &md_handler);
	default_ep_md->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/terminate.json");
	default_ep_terminate = ep_local_new(from_file_json, &terminate_handler);
	default_ep_terminate->is_default = 1;
	json_free(from_file_json);

	/* new endpoint for modules */

	from_file_json = json_load_from_file("default_eps/load_com_module_ep.json");
	load_com_module_ep = ep_local_new(from_file_json, &load_com_module_ep_handler);
	load_com_module_ep->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/load_access_module_ep.json");
	load_access_module_ep = ep_local_new(from_file_json, &load_access_module_ep_handler);
	load_access_module_ep->is_default = 1;
	json_free(from_file_json);

	from_file_json = json_load_from_file("default_eps/set_credentials_ep.json");
	set_credentials_ep = ep_local_new(from_file_json, &set_credentials_ep_handler);
	set_credentials_ep->is_default = 1;
	json_free(from_file_json);
}

