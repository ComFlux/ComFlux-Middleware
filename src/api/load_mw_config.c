/*
 * load_mw_config.c
 *
 *  Created on: 1 Mar 2017
 *      Author: Raluca Diaconu
 */

#include "load_mw_config.h"

#include <json.h>
#include <middleware.h>
#include <array.h>

#include <unistd.h> //for chdir
#include <limits.h> //for PATH_MAX

JSON *config_json = NULL, *app_json = NULL, *core_json = NULL;

char config_absolute_path[PATH_MAX+1];

int load_mw_config(const char* cfg_file_path)
{
	config_absolute_path[0] = '\0';
	realpath(cfg_file_path, config_absolute_path);

	/* get config json from file */
	config_json = json_load_from_file(cfg_file_path);
	if(config_json == NULL)
	{
		return -1;
	}

	/*	slog config for app and core */
	app_json = json_get_json(config_json, "app_config");
	core_json = json_get_json(config_json, "core_config");

	// TODO: validate against a schema
	if(core_json == NULL)
	{
		return -2;
	}
	if(app_json == NULL)
	{
		return -3;
	}

	return 0;
}

int change_home_dir(const char* path)
{
	/* home directory for app */
	char* home_dir = json_get_str(app_json, "home_dir");
	return chdir(home_dir);
}

int load_libs_array(Array* libs_array,
		int (*load_module)(const char*, const char*))
{
	int i, load_result, total = 0;
	JSON* com_lib_json;
	char *cfg_path, *lib_path;
	for(i=0; i<array_size(libs_array); i++)
	{
		com_lib_json = array_get(libs_array, i);
		cfg_path = json_get_str(com_lib_json, "cfg_path");
		lib_path = json_get_str(com_lib_json, "lib_path");
		if(cfg_path != NULL && lib_path != NULL)
		{
			load_result = load_module(lib_path, cfg_path);
			if(load_result == 0)
				total += 1;
		}
	}
	return total;
}


Array* config_get_com_libs_array()
{
	/* if the load failed, default value is NULL */
	if (core_json == NULL)
		return NULL;

	return json_get_jsonarray(core_json, "com_libs");
}

Array* config_get_access_libs_array()
{
	/* if the load failed, default value is NULL */
	if (core_json == NULL)
		return NULL;

	return json_get_jsonarray(core_json, "access_libs");
}


int config_get_app_log_lvl()
{
	/* if the load failed, default value is 0 */
	if (app_json == NULL)
		return 0;

	return json_get_int(app_json, "log_level");
}

int config_get_core_log_lvl()
{
	/* if the load failed, default value is 0 */
	if (core_json == NULL)
		return 0;

	return json_get_int(core_json, "log_level");
}
char* app_log_file = NULL;
char* config_get_app_log_file()
{
	/* if the load failed, default value is NULL */
	if (app_json == NULL)
		return NULL;

	if(app_log_file == NULL)
		app_log_file = (json_get_str(app_json, "log_file"));

	return app_log_file;
}

char* config_get_core_log_file()
{
	/* if the load failed, default value is NULL */
	if (core_json == NULL)
		return NULL;

	return json_get_str(core_json, "log_file");
}


int config_load_com_libs()
{
	/* if the load failed, return an error code */
	if (core_json == NULL)
		return -2;

	/* load com modules */
	int total, libs_array_size;
	Array* com_libs = config_get_com_libs_array();
	total =0;
	libs_array_size = array_size(com_libs);
	if(com_libs && libs_array_size)
	{
		total = load_libs_array(com_libs, &mw_load_com_module);
		//slog(SLOG_INFO, SLOG_INFO,
		//		"%d out of %d com modules successfully loaded",
		//		total, libs_array_size);
	}

	if(total == libs_array_size)
		return 0;
	else
		return -1;
}


int config_load_access_libs()
{
	/* if the load failed, return an error code */
	if (core_json == NULL)
		return -2;

	/* load access control modules */
	int total, libs_array_size;
	Array* access_libs = config_get_access_libs_array();
	total =0;
	libs_array_size = array_size(access_libs);
	if(access_libs && libs_array_size)
	{
		total = load_libs_array(access_libs, &mw_load_access_module);
		//slog(SLOG_INFO, SLOG_INFO,
		//		"%d out of %d access modules successfully loaded",
		//		total, libs_array_size);
	}

	if(total == libs_array_size)
		return 0;
	else
		return -1;
}

char* config_get_absolute_path()
{
	return config_absolute_path;
}
