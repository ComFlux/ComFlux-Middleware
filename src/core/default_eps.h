/*
 * default_eps.h
 *
 *  Created on: 1 Aug 2016
 *      Author: rad
 */

#ifndef CORE_DEFAULT_EPS_H_
#define CORE_DEFAULT_EPS_H_


#include "message.h"

/* all default ep registration */
void register_default_endpoints();

/* handlers */
void map_handler(MESSAGE* msg);
void unmap_handler(MESSAGE* msg);
void lookup_handler(MESSAGE* msg);
void add_rdc_handler(MESSAGE* msg);

#endif /* CORE_DEFAULT_EPS_H_ */
