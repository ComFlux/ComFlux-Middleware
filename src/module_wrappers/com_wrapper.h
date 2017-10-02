/*
 * com_wrapper.h
 *
 *  Created on: 18 Oct 2016
 *      Author: Raluca Diaconu
 */

#ifndef SRC_COMMON_COM_WRAPPER_H_
#define SRC_COMMON_COM_WRAPPER_H_

#include <hashmap.h>
#include <json.h>

/*
 * Com errors
 */
#define COM_OK					0
#define COM_LIB_NOT_FOUND		1
#define COM_FC_NOT_FOUND		10
#define COM_MODULE_NOT_FOUND	2

#define COM_ERROR	-1

typedef struct _COM_MODULE{
	char* name;
	JSON* metadata;
	void* handle;
    
	char* address;
    char* ifrname;
    int port;

	/*expects the following function signatures */
	char* (*fc_init)(void* module, const char* args_json);
	int   (*fc_connect)(const char *server_addr);

	int   (*fc_connection_close)(int conn);
	int   (*fc_send_data)(int conn, const char *msg);
	int   (*fc_send)(int conn, const void *ptr, unsigned int size);

	int   (*fc_set_on_data)(void (*handler)(void*, int, const char*));
	int   (*fc_set_on_connect)(void (*handler)(void*, int));
	int   (*fc_set_on_disconnect)(void (*handler)(void*, int));
	// where void* stands for module id

	int   (*fc_is_valid_address)(const char* full_address);
	int   (*fc_is_bridge)(void);

} COM_MODULE;

/*******************
 * module instance
 *******************/

/*
 * instantiate a module
 */
COM_MODULE* com_module_new(const char* lib_filename, const char* config_json);

/*
 * free a com module memory
 */
void com_module_free(COM_MODULE* module);



/* com modules' container functionality */

HashMap *com_modules;

/*
 * initialises tables
 */
int init_com_wrapper();

/*
 * loads a module given the so name
 */
int com_load_module(const char* filename, const char* config_json);

/*
 * unloads a module given the so name
 */
int com_unload_module(const char* filename);

COM_MODULE* com_get_module(const char* filename);


/**
 * com modules'common functionality: handlers that will be called for each module
 * with the name of the module.
 * @module - module name
 * init function initializes with a pointer to this module
 * all handlers deed to be passed a pointer to this module
 *
 * these functions are not yet used
 */
int com_init_wrapper(const char* modulename, const char* configfilename);
int com_connect_wrapper(const char* modulename, const char *remote_addr);
int com_connection_close_wrapper(const char* modulename, int conn);
int com_send_data_wrapper(const char* modulename, int conn, const char *data);
int com_set_on_data_wrapper(const char* modulename, void (*handler)(void*, int, const char*) );
int com_set_on_connect_wrapper(const char* modulename, void (*handler)(void*, int) );
int com_set_on_disconnect_wrapper(const char* modulename, void (*handler)(void*, int) );
int com_is_valid_address_wrapper(const char* modulename, const char* full_address);

/*
 * calls a different function, TODO
 */
void* call_module_function(COM_MODULE* modulename, const char* function, ...);


#endif /* SRC_COMMON_COM_WRAPPER_H_ */
