/*
 * rdcs.h
 *
 *  Created on: 21 Jul 2016
 *      Author: Raluca Diaconu
 */

#ifndef CORE_RDCS_H_
#define CORE_RDCS_H_

#include "array.h"
#include "json.h"
#include "endpoint.h"
#include "state.h"

typedef struct _RDC{
	/* the key for storing RDC in some containers: module_name + addr */
	char* id;
	char* addr;
	COM_MODULE* module;

	STATE* state_ptr;
	int state;

	Array * lookup_result;

	int flag;
}RDC;

#define RDC_OK		0
#define RDC_NULL	1

#define RDC_STATE_REG		10
#define RDC_STATE_UNREG		11
#define RDC_STATE_LOOKUP	12

#define RDC_REGISTER 	4
#define RDC_UNREGISTER 	5



/* instantiate a new rdc woth the address and interface;
 * add it to the rdcs list
 */
RDC* rdc_new(COM_MODULE* module, const char* addr);

void rdc_free(RDC* r);


/* rdcs map fcs */

int rdcs_init();

RDC* rdcs_get_addr(COM_MODULE* module, const char* addr);

int rdcs_set_addr(COM_MODULE* module, const char* addr, RDC* r);

int rdcs_add(RDC* r);

//TODO: RDC* rdc_get_conn(COM_MODULE* module, int conn);

/* functions specifically dealing with RDCs */

/* connects with RDC @r on local default ep RDC;
 * if provided, sends @register_msg to all known rdcs;
 * othewise builds a message with @flag;
 * @flag can be RDC_REGISTER or RDC_UNREGISTER  */
void rdc_register(RDC* r, MESSAGE* register_msg, int flag);

/* rdc register to all known rdcs. */
void rdc_register_all(int flag);

/* rdc register @addr for on a specific RDC @r. */
void rdc_register_addr(RDC* r, const char* addr, int flag);

/* rdc register @addr for all known rdcs. */
void rdc_register_addr_all(const char* addr, int flag);

/* performs a lookup @query on RDC @r */
void rdc_lookup(RDC* r, MESSAGE* lookup_msg); //, JSON* query

void rdc_lookup_all(MESSAGE* lookup_msg, JSON* query);



MESSAGE* rdc_build_register_message(int flag);
MESSAGE* rdc_build_register_addr_message(const char* addr, int flag);

MESSAGE* rdc_build_lookup_message(JSON* query);

#endif /* CORE_RDCS_H_ */


