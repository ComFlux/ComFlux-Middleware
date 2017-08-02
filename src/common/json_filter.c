/*
 * json_query.h
 *
 *  Created on: 3 Aug 2016
 *      Author: Raluca Diaconu
 */

#include "json_filter.h"

#include "array.h"
#include "hashmap.h"
#include <string.h>
#include <utils.h>

typedef struct __json_path{
	int type;
	char* prop;
	char* sign;
	char* value;
}_json_path;

_json_path* _json_path_new(const char* path)
{
	_json_path* jp = (_json_path*)malloc(sizeof(_json_path));
	char* path_dup = strdup_null(path);

	jp->prop = strdup_null(strtok (path_dup, " "));

	jp->sign = strdup_null(strtok (NULL, " "));

	jp->value = strdup_null(strtok (NULL, " "));

	jp->type = 0;
	int n = strlen(jp->value);
	if(n>=2)
		if((jp->value)[0] == '\'' && (jp->value)[n-1] == '\'')
		{
			char* temp = jp->value;
			jp->value = malloc(n-1);
			strncpy(jp->value, temp+1, n-2);
			jp->value[n-2] = '\0';
			jp->type = 1;
			free(temp);
		}

	return jp;
}

void _json_path_free(_json_path* jp)
{
	if(jp == NULL)
		return;

	free(jp->prop);
	free(jp->sign);
	free(jp->value);
	free(jp);
}

int _has_path(JSON* json, _json_path* path)
{
	/* if number */
	if(path->type == 0)
	{
		double ref;
		sscanf(path->value, "%lf", &ref);
		float value = json_get_float(json, path->prop);
		if(path->sign[0] == '=')
			return (value == ref);
		if(path->sign[0] == '>')
			return (value > ref);
		if(path->sign[0] == '<')
			return (value < ref);
		if(path->sign[0] == '!')
			return (value != ref);
		return 0;
	}

	/* if string */
	char* value = json_get_str(json, path->prop);

	int cmp = strcmp(value, path->value);
	free(value);

	return (cmp == 0);
}

/* one json; one query */
int json_filter_validate_one(JSON *json, char *path)
{
	if (json == NULL)
		return 0;
	if (path == NULL)
		return 1;

	_json_path*  jp = _json_path_new(path);

	int validate_value = _has_path(json, jp);
	_json_path_free(jp);

	return validate_value;
}

/* one json; many queries in conjunction  from an array*/
int json_filter_validate_array(JSON *json, Array *filters)
{
	if (json == NULL)
		return 0;
	if (filters == NULL)
		return 1;

	int i;
	for(i = 0; i< array_size(filters); i++)
	{
		char *path = array_get(filters, i);
		/* check if one filter failed */
		if (json_filter_validate_one(json, path) == 0)
			return 0;
	}
	/* all filters succeeded */
	return 1;
}

/* one json; many queries in conjunction  from a json array*/
int json_filter_validate(JSON *json, JSON *filter)
{
	if (json == NULL)
		return 0;
	if (filter == NULL)
		return 1;

	/* extract array of query paths */
	Array *paths = json_get_array(filter, NULL);

	int validate_value = json_filter_validate_array(json, paths);
	array_free(paths);

	return validate_value;
}


/* return all elems in the array that satisfy query */
Array* json_filter_array(Array* elems, JSON* filter)
{
	if (elems == NULL)
		return NULL;

	Array *results = array_new(ELEM_TYPE_PTR);
	if (filter == NULL)
		return results;

	int i;
	for(i= 0; i< array_size(elems); i++)
	{
		JSON *elem = array_get(elems, i);
		if(json_filter_validate(elem, filter))
			array_add(results, elem);
	}

	return results;
}

/* return all elems in the array that satisfy query */
Array* json_filter_json(JSON* json, const char *path, JSON* filter)
{
	Array *elems = json_get_array(json, path);

	if (elems == NULL)
		return NULL;

	Array *results = array_new(ELEM_TYPE_PTR);
	if (filter == NULL)
		return results;

	int i;
	for(i= 0; i< array_size(elems); i++)
	{
		JSON *elem = array_get(elems, i);
		if(json_filter_validate(elem, filter))
			array_add(results, elem);
	}

	return results;
}
