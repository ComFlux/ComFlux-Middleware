/*
 * access_wrapper.c
 *
 *  Created on: 2 Nov 2016
 *      Author: Raluca Diaconu
 */

#include "access_wrapper.h"

#include <hashmap.h>
#include <slog.h>
#include <utils.h>

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

/* error string modified if errors appear in this module
 * e.g. if functions are not found */
char* error;

/* helper function for instantiating and loading a module */
int load_all_access_functions(ACCESS_MODULE *module)
{
    module->fc_init = dlsym(module->handle, "access_init");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_authenticate = dlsym(module->handle, "access_authenticate");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_disconnect = dlsym(module->handle, "access_disconnect");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_has_access = dlsym(module->handle, "access_has_access");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_has_access_credential = dlsym(module->handle, "access_has_access_credential");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_add_access = dlsym(module->handle, "access_add_access");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_rm_access = dlsym(module->handle, "access_rm_access");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_get_credential = dlsym(module->handle, "access_get_credential");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }
    module->fc_set_credential = dlsym(module->handle, "access_set_credential");
    if ((error = dlerror()) != NULL)
    {
    	return -1;
    }

    return 0;
}

ACCESS_MODULE* access_module_new(const char* filename, const char* config_json)
{
    ACCESS_MODULE* module = (ACCESS_MODULE*)malloc(sizeof(ACCESS_MODULE));

    JSON* args_json = json_new(config_json);
    module->metadata = json_get_json(args_json, "metadata");
    module->name = json_get_str(module->metadata, "name");
    char* cred = json_get_str(args_json, "credential");
    json_free(args_json);

    /* open lib file */
    module->handle = dlopen(filename, RTLD_LAZY);
    if (!module->handle)
    {
		if ((error = dlerror()) != NULL)
		{
			slog(SLOG_ERROR, SLOG_ERROR, "ACCESS_WRAPPER: Failed loading library.\n"
					"\tError: %s", error);
		}
		else
		{
			slog(SLOG_ERROR, SLOG_ERROR, "ACCESS_WRAPPER: Failed loading library.");
		}
		return NULL;
	}

	/* load all functions */
	if(load_all_access_functions(module) != 0)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"ACCESS_WRAPPER: some functions were not found while loading %s library. Err: %s",
				filename, error);

		access_module_free(module);
		return NULL;
	}

    (*(module->fc_init))(module, config_json);

    (*(module->fc_set_credential))(cred);

    return module;
}

void access_module_free(ACCESS_MODULE* module)
{
    if (module == NULL)
        return;

    free(module->name);
    json_free(module->metadata);
    dlclose(module->handle);
    free(module);
}

/********************/

int init_access_wrapper()
{
    /* key is the name of the module */
    access_modules = map_new(KEY_TYPE_STR);
    return (access_modules == NULL);
}

int access_no_auth()
{
	if(access_modules == NULL)
		return 1;
	if(map_size(access_modules) == 0)
		return 1;

	return 0;
}

int access_load_module(const char* filename, const char* config_json)
{
    ACCESS_MODULE* module = (ACCESS_MODULE*)malloc(sizeof(ACCESS_MODULE));

    JSON* args_json = json_new(config_json);
    module->metadata = json_get_json(args_json, "metadata");
    module->name = json_get_str(module->metadata, "name");
    char* cred = json_get_str(args_json, "credential");
    json_free(args_json);

    /* open lib file */
    module->handle = dlopen(filename, RTLD_LAZY);
    if (!module->handle)
    {
		if ((error = dlerror()) != NULL)
		{
			slog(SLOG_ERROR, SLOG_ERROR, "ACCESS_WRAPPER: Failed loading library.\n"
					"\tError: %s", error);
		}
		else
		{
			slog(SLOG_ERROR, SLOG_ERROR, "ACCESS_WRAPPER: Failed loading library.");
		}
		return ACCESS_LIB_NOT_FOUND;
	}

	/* load all functions */
	if(load_all_access_functions(module) != 0)
	{
		slog(SLOG_ERROR, SLOG_ERROR,
				"ACCESS_WRAPPER: some functions were not found while loading %s library. Err: %s",
				filename, error);

		access_module_free(module);
		return ACCESS_FC_NOT_FOUND;
	}

    (*(module->fc_init))(module, config_json);
    (*(module->fc_set_credential))(cred);

    return map_update(access_modules, (void*)module->name, (void*)module);
}

int access_unload_module(const char* filename)
{
    ACCESS_MODULE* module = access_get_module(filename);
    if (module == NULL)
        return ACCESS_FC_NOT_FOUND;

    map_remove(access_modules, (void*)filename);
    access_module_free(module);

    return 0;
}

ACCESS_MODULE* access_get_module(const char* modulename)
{
    return map_get(access_modules, (void*)modulename);
}

/* functionality applied to module called by name */

int access_init_wrapper(const char* modulename, const char* filename)
{
    ACCESS_MODULE* module = access_get_module(modulename);
    if (module == NULL)
        return ACCESS_MODULE_NOT_FOUND;

    return (*(module->fc_init))(module, filename);
}


typedef void* (*func_pt)(void*, ...);

void* access_module_call_function(ACCESS_MODULE* module, const char* function, ...)
{
	char* error;
	func_pt fc_;
	fc_ = dlsym(module->handle, "function");
	if ((error = dlerror()) != NULL)
	{
		return NULL;
	}
	va_list args;
	va_start(args, function);
	void* result = fc_(args);
	va_end(args);

	return result;
}
