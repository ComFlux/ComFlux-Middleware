/*
 * manifest.c
 *
 *  Created on: 12 May 2016
 *      Author: Raluca Diaconu
 */


#include "manifest.h"

#include "hashmap.h"
#include "array.h"
#include "endpoint.h"

#include <string.h>

/* in core.c */
extern HashMap *locales;

/* functions to get metadata */
Array* metadata_com_modules_array();
Array* metadata_access_modules_array();
Array* metadata_endpoints_array();


COMPONENT* cpt = NULL;

int manifest_update(JSON *json)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "%s: %s", __func__, json_to_str(json));
	if(cpt == NULL)
	{
		 cpt = (COMPONENT*)malloc(sizeof(COMPONENT));
		 cpt->metadata = json_new(NULL);
	}

	if(json != NULL)
		json_merge(cpt->metadata, json);

	return 0;
}


JSON* manigest_get_short()
{
	JSON *manifest = json_new(json_to_str(cpt->metadata));
	if (cpt != NULL)
	{
		json_set_json(manifest, "component", cpt->metadata);
		json_set_array(manifest, "com_modules", metadata_com_modules_array());
		json_set_array(manifest, "access_modules", metadata_access_modules_array());
		json_set_array(manifest, "endpoints", metadata_endpoints_array());
	}

	return manifest;
}

Array* metadata_com_modules_array()
{
	int i;
	Array* com_modules_md_array = array_new(ELEM_TYPE_PTR);
	Array* com_modules_keys_array = map_get_keys(com_modules);

	char* com_module_key = NULL;
	COM_MODULE* module = NULL;
	JSON* module_json = NULL;

	for(i=0; i<array_size(com_modules_keys_array); i++)
	{
		com_module_key = array_get(com_modules_keys_array, i);
		module = com_get_module(com_module_key);

		module_json = json_new(NULL);
		json_set_str(module_json, "com_module", module->name);
		json_set_str(module_json, "address", module->address);

		array_add(com_modules_md_array, module_json);
	}

	return com_modules_md_array;
}

Array* metadata_access_modules_array()
{
	int i;
	Array* access_modules_md_array = array_new(ELEM_TYPE_PTR);
	Array* access_modules_keys_array = map_get_keys(access_modules);

	char* access_module_key = NULL;
	ACCESS_MODULE* module = NULL;
	JSON* module_json = NULL;

	for(i=0; i<array_size(access_modules_keys_array); i++)
	{
		module_json = json_new(NULL);
		access_module_key = array_get(access_modules_keys_array, i);
		module = access_get_module(access_module_key);

		json_set_str(module_json, "access_module", module->name);
		//json_set_str(module_json, "credentials", module->);

		array_add(access_modules_md_array, module_json);
	}

	return access_modules_md_array;
}

Array* metadata_endpoints_array()
{
	int i;
	Array* ep_md_array = array_new(ELEM_TYPE_PTR);
	Array* ep_keys_array = map_get_keys(locales);

	char* ep_key = NULL;
	LOCAL_EP* ep = NULL;

	for(i=0; i<array_size(ep_keys_array); i++)
	{
		ep_key = array_get(ep_keys_array, i);
		ep = map_get(locales, ep_key);
		if(ep->is_visible == 1)
			array_add(ep_md_array, ep_local_to_json(ep));
	}

	return ep_md_array;
}

JSON* manifest_get(int lvl)
{
	slog(SLOG_DEBUG, SLOG_DEBUG, "%s:\n"
			"\t%d", __func__, lvl);

	JSON *manifest= json_new(NULL);

	if (cpt == NULL)
		goto final;

	json_merge(manifest, cpt->metadata);

	if(lvl >= MANIFEST_SIMPLE)
	{
		json_set_json(manifest, "component", cpt->metadata);
	}
	if(lvl >= MANIFEST_SHORT)
	{
		json_set_array(manifest, "com_modules", metadata_com_modules_array());
		json_set_array(manifest, "access_modules", metadata_access_modules_array());
	}
	if(lvl >= MANIFEST_FULL)
	{
		json_set_array(manifest, "endpoints", metadata_endpoints_array());
	}

	final:
	{
		return manifest;
	}
}
