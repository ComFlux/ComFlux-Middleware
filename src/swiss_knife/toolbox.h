/*
 * toolbox.h
 *
 *  Created on: 15 Aug 2016
 *      Author: Raluca Diaconu
 */

#ifndef TOOLBOX_H_
#define TOOLBOX_H_

void terminate(const char* addr);

void add_rdc(const char* cpt_addr, const char* rdc_addr);

void map(const char* from_addr, const char* from_query, const char* dest_addr, const char* dest_query);

void map_lookup(const char* from_addr, const char* from_query, const char* dest_query);

void rdc_list(const char* rdc_addr, char* result); //, int lvl

void swiss_load_com_module(const char* target_addr,
		const char* com_module_path, const char* com_module_config);

void swiss_load_auth_module(const char* target_addr,
		const char* auth_module_path, const char* auth_module_config);



void one_shot_req(
		const char* addr,
		const char* req_schema_path, const char* resp_schema_path,
		const char* map_query,
		const char* req);

void one_shot_msg(
		const char* addr,
		const char* msg_schema_path,
		const char* map_query,
		const char* msg);

#endif /* TOOLBOX_H_ */
