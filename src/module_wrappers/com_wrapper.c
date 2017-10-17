/*
 * com_wrapper.c
 *
 *  Created on: 18 Oct 2016
 *      Author: Raluca Diaconu
 */

#include "com_wrapper.h"

#include <hashmap.h>
#include <slog.h>
#include <utils.h>

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h> /* for sleep */

/* error string modified if errors appear in this module
 * e.g. if functions are not found */
char* error;

/* helper function for instantiating and loading a module */
int load_all_com_functions(COM_MODULE *module)
{
	module->fc_init = dlsym(module->handle, "com_init");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_connect = dlsym(module->handle, "com_connect");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_connection_close = dlsym(module->handle, "com_connection_close");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_send_data = dlsym(module->handle, "com_send_data");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_send = dlsym(module->handle, "com_send");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_set_on_data = dlsym(module->handle, "com_set_on_data");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_set_on_connect = dlsym(module->handle, "com_set_on_connect");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_set_on_disconnect = dlsym(module->handle, "com_set_on_disconnect");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_is_valid_address = dlsym(module->handle, "com_is_valid_address");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}
	module->fc_is_bridge = dlsym(module->handle, "com_is_bridge");
//	if ((error = dlerror()) != NULL)
//	{
//		return -1;
//	}


	return 0;
}


COM_MODULE* com_module_new(const char* filename, const char* config_json)
{
	COM_MODULE* module = (COM_MODULE*) malloc(sizeof(COM_MODULE));

	JSON* args_json = json_new(config_json);
	if (args_json == NULL)
	{
		module->metadata = json_new(NULL);
		module->name = strdup_null(filename);
		//module->address = NULL;
	}
	else
	{
		//module->metadata = json_new(NULL);
		//module->name = strdup_null(filename);
		module->metadata = json_get_json(args_json, "metadata");
		module->name = json_get_str(module->metadata, "name");
		//module->address = json_get_str(args_json, "address");
		//json_free(module_me)
		json_free(args_json);
	}

	/* open lib file */
	module->handle = dlopen(filename, RTLD_LAZY);
	if (!module->handle)
	{
		if ((error = dlerror()) != NULL)
		{
			slog(SLOG_ERROR, "COM_WRAPPER: Failed loading library.\n"
					"\tError: %s", error);
		}
		else
		{
			slog(SLOG_ERROR, "COM_WRAPPER: Failed loading library.");
		}
		return NULL;
	}

	/* load all functions */
	if(load_all_com_functions(module) != 0)
	{
		slog(SLOG_ERROR,
			"COM_WRAPPER: some functions were not found while loading %s library. Err: %s",
			filename, error);

		com_module_free(module);
		return NULL;
	}

	module->address = (*(module->fc_init))(module, config_json);

	if(module->address == NULL)
	{
		sleep(1);
		module->address = (*(module->fc_init))(module, config_json);
	}

	if(module->address == NULL)
	{
		com_module_free(module);
		return NULL;
	}

	return module;
}

void com_module_free(COM_MODULE* module)
{
	if (module == NULL)
		return;

	free(module->name);
	json_free(module->metadata);
	dlclose(module->handle);
	free(module);
}

/* container functionality*/

int init_com_wrapper()
{
	/* key is the name of the module */
	com_modules = map_new(KEY_TYPE_STR);
	return (com_modules == NULL);
}

int com_load_module(const char* filename, const char* config_json)
{
	COM_MODULE* module = (COM_MODULE*) malloc(sizeof(COM_MODULE));

	JSON* args_json = json_new(config_json);
	if (args_json == NULL)
	{
		module->metadata = json_new(NULL);
		module->name = strdup_null(filename);
		//module->address = NULL;
	}
	else
	{
		module->metadata = json_get_json(args_json, "metadata");
		module->name = json_get_str(module->metadata, "name");
		//module->address = json_get_str(args_json, "address");
		json_free(args_json);
	}

	/* open lib file */
	module->handle = dlopen(filename, RTLD_LAZY);
	if (!module->handle)
	{
		if ((error = dlerror()) != NULL)
		{
			slog(SLOG_ERROR, "COM_WRAPPER: Failed loading library.\n"
					"\tError: %s", error);
		}
		else
		{
			slog(SLOG_ERROR, "COM_WRAPPER: Failed loading library.");
		}
		return COM_LIB_NOT_FOUND;
	}

	if(load_all_com_functions(module) != 0)
	{
		slog(SLOG_ERROR,
			"COM_WRAPPER: some functions were not found while loading %s library. Err: %s",
			filename, error);

		com_module_free(module);
		return COM_FC_NOT_FOUND;
	}

	module->address = (*(module->fc_init))(module, config_json);

	if(module->address == NULL)
	{
		sleep(1);
		module->address = (*(module->fc_init))(module, config_json);
	}

	if(module->address == NULL)
	{
		com_module_free(module);
		return COM_ERROR;
	}

	return map_update(com_modules, (void*)module->name, (void*)module);
}

int com_unload_module(const char* filename)
{
	COM_MODULE* module = com_get_module(filename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	map_remove(com_modules, (void*)filename);
	com_module_free(module);

	return 0;
}

COM_MODULE* com_get_module(const char* modulename)
{
	return map_get(com_modules, (void*)modulename);
}


/* all functionality applied to module called by name */

int com_init_wrapper(const char* modulename, const char* configfile)
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	char* new_address = (*(module->fc_init))(module, configfile);
	if(module->address)
		free(module->address);
	module->address = new_address;

	return (module->address != NULL);
}

int com_connect_wrapper(const char* modulename, const char* server_addr)
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_connect))(server_addr);
}

int com_connection_close_wrapper(const char* modulename, int conn)
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_connection_close))(conn);
}

int com_send_data_wrapper(const char* modulename, int conn, const char* data)
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_send_data))(conn, data);
}

int com_send_data_str_wrapper(const char* modulename, int conn, void* data, unsigned int size)
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_send))(conn, data, size);
}

int com_set_on_data_wrapper(const char* modulename,
		void (*handler)(void*, int, const void*, unsigned int))
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_set_on_data))(handler);
}

int com_set_on_connect_wrapper(const char* modulename, void (*handler)(void*, int))
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_set_on_connect))(handler);
}

int com_set_on_disconnect_wrapper(const char* modulename, void (*handler)(void*, int))
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_set_on_disconnect))(handler);
}

int com_is_valid_address_wrapper(const char* modulename, const char* full_address)
{
	COM_MODULE* module = com_get_module(modulename);
	if (module == NULL)
		return COM_MODULE_NOT_FOUND;

	return (*(module->fc_is_valid_address))(full_address);
}


typedef void* (*func_pt)(void*, ...);

void* com_module_call_function(COM_MODULE* module, const char* function, ...)
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
	void* result = fc_(module, args);
	va_end(args);

	return result;
}
