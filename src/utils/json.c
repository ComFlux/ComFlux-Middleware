/*
 * json1.c
 *
 *  Created on: 30 Jan 2017
 *      Author: Raluca Diaconu
 */


#include <string.h>

#include "json.h"
#include "utils.h"
#include "file.h"
#include <errno.h>


#include <json-c/json_tokener.h>

/* deep duplication */
struct json_object* _json_elem_dup(struct json_object* json_elem)
{
	if(json_elem == NULL)
		return NULL;

	struct json_object* new_json_elem = NULL;

	const char* json_elem_str = json_object_to_json_string_ext(json_elem, JSON_C_TO_STRING_SPACED);
	struct json_tokener* jtok = json_tokener_new();
	if(json_elem_str)
		new_json_elem = json_tokener_parse_ex(jtok, json_elem_str, strlen(json_elem_str));
	json_tokener_free(jtok);

	return new_json_elem;
}

JSON* _json_dup(JSON* json)
{
	if(json == NULL)
		return NULL;

	JSON *new_json = (JSON*)malloc(sizeof(JSON));
	new_json->elem_json = _json_elem_dup(json->elem_json);

	return new_json;
}



JSON* json_new(const char* msg)
{
	JSON *json = (JSON*)malloc(sizeof(JSON));
	struct json_tokener* jtok = json_tokener_new();

	if(msg == NULL)
	{
		/* NULL in json_tokener_parse throws seg fault */
		json->elem_json = json_tokener_parse_ex(jtok, "{}", 2);
	}
	else
	{
		json->elem_json = json_tokener_parse_ex(jtok, msg, strlen(msg));
	}
	json_tokener_free(jtok);

	return json;
}

void json_free(JSON* json)
{
	if(json == NULL)
	{
		return;
	}

	if(json->elem_json != NULL)
	{
		json_object_put(json->elem_json);
	}

	free(json);
	json = NULL;
}

void json_set_int(JSON* parent, const char* prop, int val)
{
	if(prop == NULL)
		return;

	struct json_object *son_elem = json_object_new_int(val);
	json_object_object_add(parent->elem_json, prop, son_elem);
}

void json_set_float(JSON* parent, const char* prop, float val)
{
	if(prop == NULL)
		return;

	struct json_object *son_elem = json_object_new_double(val);
	json_object_object_add(parent->elem_json, prop, son_elem);
}

void json_set_str(JSON* parent, const char* prop, const char* val)
{
	if(prop == NULL || val == NULL)
		return;

	struct json_object *son_elem = json_object_new_string(val);
	json_object_object_add(parent->elem_json, prop, son_elem);
}

void json_set_json(JSON* parent, const char* prop, JSON* val)
{
	if(prop == NULL || val == NULL)
		return;

	struct json_object* val_elem_dup = _json_elem_dup(val->elem_json);
	json_object_object_add(parent->elem_json, prop, val_elem_dup);
}


void json_set_array(JSON* parent, const char* prop, Array* val)
{
	struct json_object *son_elem = json_object_new_array();

	if(val->elem_type == ELEM_TYPE_INT) // TODO: not implemented
	{

	}
	if(val->elem_type == ELEM_TYPE_STR)
	{
		int i;
		char* elem;
		for(i=0; i<array_size(val); i++)
		{
			elem = array_get(val, i);
			json_object_array_add(son_elem, json_object_new_string(elem));
		}
	}
	if(val->elem_type == ELEM_TYPE_PTR) //JSON object
	{
		int i;
		JSON *json_iter;
		for(i=0; i<array_size(val); i++)
		{
			json_iter = array_get(val, i);
			struct json_object *json_iter_elem = _json_elem_dup(json_iter->elem_json);
			json_object_array_add(son_elem, json_iter_elem);
		}
	}

	if(prop != NULL)
		json_object_object_add(parent->elem_json, prop, son_elem);
	else
		parent->elem_json = son_elem;
}

int json_get_int(JSON* json, const char* prop)
{
	if(json == NULL)
		return -13;

	struct json_object *son_elem;
	json_object_object_get_ex(json->elem_json, prop, &son_elem);
	if(! json_object_is_type(son_elem, json_type_int))
		return -13;

	return json_object_get_int(son_elem);
}

float json_get_float(JSON* json, const char* prop)
{

	if(json == NULL)
		return -13.0;

	struct json_object *son_elem;
	json_object_object_get_ex(json->elem_json, prop, &son_elem);

	/* convert int to floating point */
	if(json_object_is_type(son_elem, json_type_int))
		return (float)json_object_get_int(son_elem);

	/* return the floating point */
	if(! json_object_is_type(son_elem, json_type_double))
		return -13.0;

	return json_object_get_double(son_elem);
}

char* json_get_str(JSON* json, const char* prop)
{
	if(json == NULL)
		return NULL;

	struct json_object *son_elem;
	json_object_object_get_ex(json->elem_json, prop, &son_elem);
	if(! json_object_is_type(son_elem, json_type_string))
		return NULL;

	return strdup_null(json_object_get_string(son_elem));
}

JSON* json_get_json(JSON* json, const char* prop)
{
	if(json == NULL)
		return NULL;

	JSON* son = (JSON*)malloc(sizeof(JSON));
	son->elem_json = NULL;

	struct json_object *inner_json_elem;
	json_object_object_get_ex(json->elem_json, prop, &(inner_json_elem));

	if(!json_object_is_type(inner_json_elem, json_type_object) &&
	   !json_object_is_type(inner_json_elem, json_type_array))
	{
		json_free(son);
		return NULL;
	}

	son->elem_json = _json_elem_dup(inner_json_elem);

	return son;
}

void json_merge(JSON* parent, JSON* val)
{
	if(val == NULL || parent == NULL)
	{
		return;
	}
	if(val->elem_json == NULL)
	{
		return;
	}
	if(parent->elem_json == NULL)
		parent->elem_json = json_tokener_parse("{}");

	struct json_object* val_elem = (val->elem_json);

	struct json_object_iter iter;
	json_object_object_foreachC(val_elem, iter)
	{
		json_object_object_add(parent->elem_json, iter.key, _json_elem_dup(iter.val));
	}
}

char* json_to_str(JSON* json)
{
	char *result = NULL;

	if (json == NULL)
		goto final;

	if (json->elem_json == NULL)
	{
		json = NULL;
		goto final;
	}

	result = strdup_null(json_object_to_json_string_ext(json->elem_json, JSON_C_TO_STRING_SPACED));
	final:{
		return result;
	}
}

char* json_to_str_pretty(JSON* json)
{
	char *result = NULL;

	if (json == NULL)
		goto final;

	if (json->elem_json == NULL)
	{
		json = NULL;
		goto final;
	}

	result = strdup_null(json_object_to_json_string_ext(json->elem_json, JSON_C_TO_STRING_PRETTY));
	final:{
		return result;
	}
}


///////////////// Dhruv's RDC functionality

JSON *json_get_next(JSON *json, const char *prop, JSON *prev)
{
	// Not implemented
	return NULL;
}

JSON *json_get_first(JSON *json, const char* prop)
{
	return json_get_next(json, prop, NULL);
}

Array* 	json_get_array(JSON* json, const char* prop)
{
	Array *new_array = array_new(ELEM_TYPE_STR);

	if(json == NULL || json->elem_json == NULL)
		return new_array;

	struct json_object *son;
	if(prop == NULL)
	{
		if(json_object_is_type(json->elem_json, json_type_array))
		{
			son = (json->elem_json);
		}
		else
		{
			json_object_object_get_ex(json->elem_json, "", &son);
		}
	}
	else
	{
		json_object_object_get_ex(json->elem_json, prop, &son);
	}

	struct json_object *elem;
	int array_lenght = json_object_array_length(son);

	int i;
	for(i = 0; i< array_lenght; i++)
	{
	 	elem = json_object_array_get_idx(son, i);
	 	array_add(new_array, (void*)(json_object_get_string(elem)));
	}

	return new_array;
}

Array* json_get_jsonarray(JSON* json, const char* array_prop)
{
	Array *const new_array = array_new(ELEM_TYPE_PTR);

	if(json == NULL)
		return new_array;

	struct json_object *array_json;
	json_object_object_get_ex(json->elem_json, array_prop, &array_json);
	if(array_json == NULL)
		return new_array;

	int array_lenght = json_object_array_length(array_json);

	int i;
	for(i = 0; i< array_lenght; i++)
	{
	 	JSON *son = (JSON*)malloc(sizeof(JSON));
	 	son->elem_json = json_object_array_get_idx(array_json, i);
	 	array_add(new_array, (void*)son);
	}

	return new_array;
}

JSON *json_load_from_file(const char *filename)
{
	FILE *_file = fopen( filename, "r" );

	if ( _file == NULL)
		return NULL;

	/* get size of file */
	fseek(_file, 0L, SEEK_END);
	int size = ftell(_file);
	rewind(_file);

	/* read file */
	char* var;
	var = (char*)malloc(size+1);

	fread (var, 1, size, _file);
	fclose( _file );

	/* return json
	if json is null, then there was a problem in parsing the file content*/
	JSON* json = json_new(var);
	free(var);

	return json;
}

/****************************/

int json_validate(JSON* schema, JSON* instance)
{
	if (!schema || !schema->elem_json)
	{
		//slog(SLOG_ERROR,
		//		"JSON: Invalid NULL schema");
		return JSON_INVALID_SCHEMA;
	}
	if (!json_validate_schema(schema->elem_json))
	{
		return JSON_INVALID_SCHEMA;
	}

	if (!instance || !instance->elem_json)
	{
		//slog(SLOG_ERROR,
		//		"JSON: Invalid NULL instance\n");
		return JSON_INVALID_JSON;
	}

    /* validate empty object
     * note that value returned by json_validate_instance is the number of errors! */
    if (json_validate_instance(instance->elem_json, schema->elem_json))
    {
    	/*printf(
    			"JSON not valid:\n "
    			"\tinstance:\t*%s*\n"
    			"\tschema\t*%s*\n",
				json_to_str_pretty(instance), json_to_str_pretty(schema));
	*/
    	return JSON_NOT_VALID;
    }

	/*slog(SLOG_WARN,
			"JSON not valid:\n "
			"\tinstance:\t*%s*\n"
			"\tschema\t*%s*",
			json_to_str_pretty(instance), json_to_str_pretty(schema));
	*/
	return JSON_OK;
}

int json_schema_validate_str(const char *schema, const char * instance)
{
	int return_value = JSON_OK;
	if (!schema)
	{
		//slog(SLOG_ERROR,
		//		"JSON: Invalid NULL schema");
		return_value = JSON_INVALID_SCHEMA;
		goto final;
	}

	if (!instance)
	{
		//slog(SLOG_ERROR,
		//		"JSON: Invalid NULL instance");
		return_value = JSON_INVALID_JSON;
		goto final;
	}

    struct json_object *schema_json, *instance_json;
    instance_json = json_tokener_parse(instance);
    schema_json = json_tokener_parse(schema);

    if(!schema_json)
    {
    	//slog(SLOG_ERROR, "JSON: Invalid schema: %s", schema);
    	return_value = JSON_INVALID_SCHEMA;
    	goto final;
    }

    if(!instance_json)
	{
		//slog(SLOG_ERROR, "JSON: Invalid instance: %s", instance);
    	return_value = JSON_INVALID_SCHEMA;
    	goto final;
	}

    /* validate empty object */
    if (!json_validate_instance(instance_json, schema_json))
    {
    	/*slog(SLOG_INFO,
    			"JSON valid:\n "
    			"\tinstance:\t*%s*\n"
    			"\tschema\t*%s*",
				json, schema);
		return JSON_OK;
    }
    else
    {
    	slog(SLOG_WARN,
    			"JSON not valid:\n "
    			"\tinstance:\t*%s*\n"
    			"\tschema\t*%s*",
				json, schema);*/
    	return_value = JSON_NOT_VALID;
    	goto final;
    }

    final:{
    	return return_value;
    }
}

int json_schema_validate_file(const char *schema_filepath, const char * instance)
{
	if(!schema_filepath)
	{
		//slog(SLOG_ERROR, "JSON: Invalid schema file path: \"%s\" ", schema_filepath);
		return JSON_INVALID_FILE;
	}

	char *schema = text_load_from_file(schema_filepath);

	return json_schema_validate_str(schema, instance);
}

