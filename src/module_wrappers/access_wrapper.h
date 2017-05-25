/*
 * access_wrapper.h
 *
 *  Created on: 2 Nov 2016
 *      Author: Raluca Diaconu
 */

#ifndef SRC_COMMON_ACCESS_WRAPPER_H_
#define SRC_COMMON_ACCESS_WRAPPER_H_

#include <hashmap.h>
#include <json.h>

/*
 * Access errors
 */
#define ACCESS_OK					0
#define ACCESS_LIB_NOT_FOUND		1
#define ACCESS_FC_NOT_FOUND			10
#define ACCESS_MODULE_NOT_FOUND		2

#define AUTH_ERROR	-1

typedef struct _ACCESS_MODULE{
	char* name;
	JSON* metadata;

	void *handle;

	/*expects the following function signatures */
	int   (*fc_init)        (void* module, const char* config_json);
	int   (*fc_authenticate)(void* credential, void* connection);
	int   (*fc_disconnect)  (void* connection);

	int   (*fc_has_access)  (void* endpoint, void* connection);
	int   (*fc_has_access_credential)
	                        (void* endpoint, void* credential);

    int   (*fc_add_access)  (void* endpoint, void* credential);
    int   (*fc_rm_access)   (void* endpoint, void* credential);

    //get token for authorization
   // char* (*fc_get_token) (void* subject);

    char* (*fc_get_credential)();
    int   (*fc_set_credential)(char* cred);

} ACCESS_MODULE;

/*******************
 * module instance
 *******************/

/*
 * instantiate a module
 */
ACCESS_MODULE* access_module_new(const char* lib_filename, const char* cfgfile);

/*
 * free an ACC module
 */
void access_module_free(ACCESS_MODULE* module);


/********************
 * modules' container functionality
 ********************/

HashMap *access_modules;

/*
 * initialises tables
 */
int init_access_wrapper();

/*
 * do not check auth messages if there is no loaded module
 */
int access_no_auth();

/*
 * loads a module given the so name
 */
int access_load_module(const char* filename, const char* cfgfile);

/*
 * unloads a module given the so name
 */
int access_unload_module(const char* filename);

ACCESS_MODULE* access_get_module(const char* filename);


/**
 * access control modules'common functionality: handlers that will be called for each module
 * with the name of the module.
 * @module - module name
 * init function initializes with a pointer to this module
 * all handlers deed to be passed a pointer to this module
 ********************/
int access_init_wrapper(const char* modulename,const char* filename);

/*
 * calls a different function, TODO
 */
//void* call_module_function(const char* module, const char* function, ...);

#endif /* SRC_COMMON_ACC_WRAPPER_H_ */
