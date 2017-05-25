/*
 * json_query.h
 *
 *  Created on: 3 Aug 2016
 *      Author: Raluca Diaconu
 */

#ifndef COMMON_JSON_FILTER_H_
#define COMMON_JSON_FILTER_H_

#include "json.h"

/* does @json satisfy the @filter? */
int json_filter_validate_one(JSON *json, char *filter);

/* does @json satisfy each psth in array @filters? */
int json_filter_validate_array(JSON *json, Array *filters);

/*
 * one @json; many filters in conjunction from a json array
 *E.g.,...
 */
int json_filter_validate(JSON *json, JSON *filter);

/* return all elems in the array that satisfy @filter */
Array* json_filter_json(JSON* json, const char *path, JSON* filter);

#endif /* COMMON_JSON_FILTER_H_ */
