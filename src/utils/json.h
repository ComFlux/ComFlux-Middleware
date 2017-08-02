/*
 * json.h
 *
 *  Created on: 4 Apr 2016
 *      Author: Raluca Diaconu
 */

#ifndef JSON_H_
#define JSON_H_
/*
 * This file encapsulates json
 * At the moment it is a wrapper onto WJELEMENT
 * These functions handle json messages and schemas.
 */

#include "array.h"
#include <schema_validator.h>
#include <instance_validator.h>

#define JSON_OK				0
#define JSON_INVALID_JSON	1
#define JSON_INVALID_SCHEMA	2
#define JSON_NOT_VALID		3
#define JSON_INVALID_FILE	4

#define JSON_ERROR			-1


typedef struct _JSON{
	struct json_object* elem_json;
} JSON;


JSON * json_new(const char* msg);
void json_free(JSON * json);
JSON* _json_dup(JSON* json);

/* setters */
/* function overloading does not work in C */
void 	json_set_int   (JSON* parent, const char* prop, int val);
void 	json_set_float (JSON* parent, const char* prop, float val);
void 	json_set_str   (JSON* parent, const char* prop, const char* val);
void 	json_set_json  (JSON* parent, const char* prop, JSON* val);
void 	json_set_json_dup  (JSON* parent, const char* prop, JSON* val);
void 	json_set_array (JSON* parent, const char* prop, Array* val);

/* getters */
int     json_get_int   (JSON* json, const char* prop);
float	json_get_float (JSON* json, const char* prop);
char* 	json_get_str   (JSON* json, const char* prop);
JSON*	json_get_json  (JSON* json, const char* prop);

/* returns an array with strings;
 * if no result is found, the array must be empty
 */
Array* 	json_get_array(JSON* json, const char *prop);

/* merge */
void json_merge(JSON* parent, JSON* val);

/* validation */
int json_validate(JSON* schema, JSON* json);


/*
 * Validates a string json message against a string schema.
 */
int json_schema_validate_str(const char *schema, const char * json);

/*
 * Validates a string json message against a schema at the filepath.
 */
int json_schema_validate_file(const char *filepath, const char * json);

char* json_to_str(JSON* json);
char* json_to_str_pretty(JSON* json);


/* loads json from file */
JSON* json_load_from_file(const char *filename);

///////////////// Dhruv's RDC functionality

JSON*	json_get_next(JSON* json, const char* prop, JSON *prev);
JSON*	json_get_first(JSON* json, const char* prop);

/* Returns an array of JSONs */
Array* 	json_get_jsonarray(JSON* json, const char *array_prop);

#endif /* JSON_H_ */
