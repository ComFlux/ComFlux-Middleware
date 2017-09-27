/*
 * rdcs.c
 *
 *  Created on: 21 Jul 2016
 *      Author: Raluca Diaconu
 */

#include "rdcs.h"
#include "hashmap.h"
#include "core_module.h"
#include <stdio.h>
#include <unistd.h>
#include "manifest.h"
#include <utils.h>

/* all RDCs */
HashMap *rdcs = NULL;

extern LOCAL_EP *ep_reg_rdc;
extern LOCAL_EP *ep_lookup;

extern int rdc_register_pipe[2];

RDC* rdc_new(COM_MODULE* module, const char* addr)
{
	RDC *r = (RDC*)malloc(sizeof(RDC));

	r->addr = strdup_null(addr);
	r->id = (char*) malloc(105*sizeof(char));
	sprintf(r->id, "%s:%s", module->name, addr);

	r->lookup_result = array_new(ELEM_TYPE_PTR);

	r->state = RDC_STATE_UNREG;
	r->flag = 0;

	r->module = module;
	r->state_ptr = NULL;

	return r;
}

void rdc_free(RDC* r)
{
	if(r == NULL)
		return;

	map_remove(rdcs, r->addr);
	array_free(r->lookup_result);
	//state_free(r->state_ptr); this should be null

	free(r->addr);
	free(r->id);

	free(r);
}


//RDC* rdc_get_conn(COM_MODULE* module, int conn){}

int rdcs_init()
{
	rdcs = map_new(KEY_TYPE_STR);
	return (rdcs == NULL);
}

RDC* rdcs_get_addr(COM_MODULE* module, const char* addr)
{
	char full_addr[105];
	sprintf(full_addr, "%s:%s", module->name, addr);
	RDC *r = map_get(rdcs, full_addr);
	return r;
}

int rdcs_set_addr(COM_MODULE* module, const char* addr, RDC* r)
{
	if(r == NULL)
	{
		slog(SLOG_INFO, "rdc is null");
		return -1;
	}

	char* addr_str=(char*) malloc(105*sizeof(char));
	sprintf(addr_str, "%s:%s", module->name, addr);
	slog(SLOG_INFO, "inserting  rdc %s", addr_str);
	return map_insert(rdcs, addr_str, r);
}

int rdcs_add(RDC* r)
{
	if(r == NULL)
	{
		slog(SLOG_INFO, "rdc is null");
		return -1;
	}

	char* addr_str=(char*) malloc(105*sizeof(char));
	if(r->module == NULL)
		sprintf(addr_str, "(null):%s", r->addr);
	else
		sprintf(addr_str, "%s:%s", r->module->name, r->addr);
	slog(SLOG_INFO, "inserting  rdc %s", addr_str);
	return map_insert(rdcs, addr_str, r);
}

MESSAGE* rdc_build_register_message(int flag)
{
	MESSAGE* register_msg;
	JSON* register_json = json_new(NULL);
	json_set_int(register_json, "flag", flag);

	JSON* manifest = manifest_get(MANIFEST_SHORT);
	json_set_json(register_json, "manifest", manifest);
	register_msg = message_new(json_to_str(register_json), MSG_MSG);
	json_free(register_json);
	json_free(manifest);

	return register_msg;
}

MESSAGE* rdc_build_register_addr_message(const char* addr, int flag)
{
	MESSAGE* register_msg;
	JSON *metadata = json_new(NULL);
	json_set_int(metadata, "flag", flag);

	JSON* target = json_new(NULL);
	json_set_str(target, "address", addr);
	json_set_json(metadata, "metadata", target);

	register_msg = message_new(json_to_str(metadata), MSG_MSG);
	json_free(metadata);

	return register_msg;
}

void rdc_register(RDC* r, MESSAGE* register_msg, int flag)
{
	if(r == NULL)
	{
		slog(SLOG_ERROR, "RDC: Can't register with null RDC");
		return;
	}

	if(flag != RDC_REGISTER && flag != RDC_UNREGISTER)
	{
		slog(SLOG_ERROR, "RDC: Unknown flag for register: %d", flag);
		return;
	}

	int free_register_msg = 0;
	if (register_msg == NULL)
	{
		register_msg = rdc_build_register_message(flag);
		free_register_msg = 1;
	}

	JSON* query_json = json_new("[\"ep_name = 'register'\"]");

	/* check if
	 * 		rdc has a module and
	 * 		address is correct to this module;
	 * if YES - map on that module + addr
	 * if NO  - map on all modules
	 */
	int register_map = -1;
	if(r->module != NULL && (*(r->module->fc_is_valid_address))(r->addr))
		register_map = core_map(ep_reg_rdc,
		                        r->module, r->addr,
		                        query_json, NULL);
	else
		register_map = core_map_all_modules(ep_reg_rdc,
		                                    r->addr,
		                                    query_json, NULL);

	if(register_map == 0)
	{
		register_msg->ep = ep_reg_rdc->ep;

	    //sync_init(rdc_register_pipe);
		core_ep_send_message(ep_reg_rdc, message_generate_id(), message_to_str(register_msg));

		//sync_wait(rdc_register_pipe[1]);
		//ep_unmap_all(ep_reg_rdc);
	}
	else
	{
		slog(SLOG_ERROR,
				"RDC: %s; Could not perform: %s",
				r->addr,
				(flag==RDC_REGISTER)?"register":"unregister");
	}

	if(free_register_msg)
		message_free(register_msg);
}

void rdc_register_all(int flag)
{
	MESSAGE* register_msg = rdc_build_register_message(flag);

	Array *keys = map_get_keys(rdcs);
	int i;
	char* rdc_addr;
	RDC* r;

	for (i = 0; i < array_size(keys); i++)
	{
		rdc_addr = array_get(keys, i);
		r = map_get(rdcs, rdc_addr);

		rdc_register(r, register_msg, flag);
	}

	message_free(register_msg);
}


// TODO: revise
void rdc_register_addr(RDC* r, const char* addr, int flag)
{
	MESSAGE* register_msg = rdc_build_register_addr_message(addr, flag);

	char* rdc_addr;
	rdc_register(r, register_msg, flag);

	message_free(register_msg);
}

void rdc_register_addr_all(const char* addr, int flag)
{
	MESSAGE* register_msg = rdc_build_register_addr_message(addr, flag);

	Array *keys = map_get_keys(rdcs);
	int i;
	char* rdc_addr;
	RDC* r;

	for(i=0; i<array_size(keys); i++)
	{
		rdc_addr = array_get(keys, i);
		r = map_get(rdcs, rdc_addr);

		rdc_register(r, register_msg, flag);
	}

    array_free(keys);
	message_free(register_msg);
}

MESSAGE* rdc_build_lookup_message(JSON* query)
{
	MESSAGE* lookup_msg = message_new(json_to_str(query), MSG_MSG);

	return lookup_msg;
}

void rdc_lookup(RDC* r, MESSAGE* lookup_query)//, JSON* ep_query, JSON* cpt_query
{
	JSON* query_json = json_new("[\"ep_name = 'lookup'\"]");
	//int lookup_map = core_map(ep_lookup, tcp_module, r->addr, query_json, NULL); // "[\"ep_name = '*'\"]"
	int lookup_map = core_map_all_modules(ep_lookup, r->addr, query_json, NULL); // "[\"ep_name = '*'\"]"
	json_free(query_json);

	r->state = RDC_STATE_LOOKUP;
	/* send the query */
	//core_ep_send_request(ep_lookup, generate_id(), message_to_str(query_msg));


	core_ep_send_request(ep_lookup, generate_id(), message_to_str(lookup_query));

	slog(SLOG_DEBUG, "CORE: Lookup request sent to %s", r->addr);

	int c = 0;
	while(r->state == RDC_STATE_LOOKUP && c < 5) {
		sleep(1);  // TODO: use lock.
        c += 1;
    }

	r->state = RDC_STATE_REG;
	slog(SLOG_DEBUG, "CORE: Lookup response size: %d", array_size(r->lookup_result));

}

void rdc_lookup_all(MESSAGE* lookup_msg, JSON* query)
{
	int free_register_msg = 0;
	if (lookup_msg == NULL)
	{
		lookup_msg = rdc_build_lookup_message(query);
		free_register_msg = 1;
	}

	Array *keys = map_get_keys(rdcs);
	int i;
	char* rdc_addr;
	RDC* r;
	for (i = 0; i < array_size(keys); i++)
	{
			rdc_addr = array_get(keys, i);
			r = map_get(rdcs, rdc_addr);
			rdc_lookup(r, lookup_msg);
	}

}
