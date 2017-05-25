/*
 * simple_acc.c
 *
 *  Created on: 10 Nov 2016
 *      Author: Raluca Diaconu
 */

#include "simple_acc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>
#include <hashmap.h>
#include "auth.h"

int initiated;
void* auth_module;
HashMap* acc_table; /* hashmap<"endpony name",hashmap<"peer_credential1":"" ; "peer_credential1":"", > > */
char* credential;
HashMap* cred_table;//

int acl_add_object(void* object);
int acl_rm_object(void* object);

void (*on_access_granted)(void*, const char*);
void (*on_access_denied)(void*, const char*);
void (*on_change)(void*, const char*);

int access_init(void* module, const char* config_json)
{
    auth_module = module;
    acc_table = map_new(KEY_TYPE_STR);
    cred_table = map_new(KEY_TYPE_PTR);//

    if (acc_table)
        initiated = 1; // TRUE;
    if (config_json != NULL) {
        JSON* file = json_new(config_json);
        access_set_credential(json_get_str(file, "credential"));
        char* str = json_get_str(file, "access_granted");
        char* rest = str;
        char* token;
        while ((token = strtok_r(rest, ",", &rest))) {
            access_add_access(NULL, token);
        }
    }
    return initiated;
}
int access_authenticate(void* cpt_cred, void* connection)
{
	map_update(cred_table, connection, cpt_cred);//
    return access_has_access(NULL, cpt_cred);
}

void access_disconnect(void* connection) //
{
	map_remove(cred_table, connection);
}

int access_has_access(void* endpoint, void* connection)//
{
	void* this_cred = NULL;
	if(endpoint == NULL)
		return -1;
	if(connection == NULL)
		return -1;

	this_cred = map_get(cred_table, connection);
	if(this_cred == NULL)
		return -1;

	return access_has_access_credential(endpoint, this_cred);
}

int access_has_access_credential(void* endpoint, void* __credential)
{   
    HashMap* object_list;
    if (endpoint == NULL) {
        object_list = (HashMap*)map_get(acc_table, "component");
        // printf("hha %d\n", map_size(object_list));
    } else {
        object_list = (HashMap*)map_get(acc_table, endpoint);
    }

    if (object_list == NULL)
        return -1;

    return map_contains(object_list, __credential);
}
int access_add_access(void* object, void* subject)
{
    HashMap* object_list;
    if (object == NULL) { // default (component level) acl
        if (!map_contains(acc_table, "component")) {
            acl_add_object("component");
        }
        object_list = (HashMap*)map_get(acc_table, "component");
    } else if (map_contains(acc_table, object)) {
        object_list = (HashMap*)map_get(acc_table, object);
    } else {
        acl_add_object(object);
        object_list = (HashMap*)map_get(acc_table, object);
    }

    if (object_list == NULL)
        return -1;

    int operation_value = map_insert(object_list, subject, "");
    if (operation_value != 0)
        return 0;

    return 1;
}
int acl_add_object(void* object)
{
    if (map_contains(acc_table, object)) {
        return 1;
    }

    HashMap* object_list = map_new(KEY_TYPE_STR);
    // inherent from the conponent
    if (strcmp((char*)object, "component") != 0) { // ep acl
        HashMap* parent = (HashMap*)map_get(acc_table, "component");
        Array* keys = map_get_keys(parent);
        unsigned int i = 0;
        while (i < array_size(keys)) {
            char* k = (char*)(array_get(keys, i));
            map_insert(object_list, k, map_get(parent, k));
            i++;
        }
    }
    return map_insert(acc_table, (char*)object, object_list);
}

int access_rm_access(void* object, void* subject)
{
    HashMap* object_list;
    if (object == NULL) { // default (component level) acl
        object_list = (HashMap*)map_get(acc_table, "component");
    } else {
        object_list = (HashMap*)map_get(acc_table, object);
    }

    if (object_list == NULL)
        return 1;
    int rtn = map_remove(object_list, subject);
    if (map_size(object_list) == 0) {
        acl_rm_object(object);
    }
    return rtn;
}

int acl_rm_object(void* object)
{
    if (object != NULL) { // never delete the default (component) acl
        return map_remove(acc_table, object);
    } else {
        return 0;
    }
}

char* access_get_credential()
{
    return credential;
}

int access_set_credential(char* cred)
{
    credential = cred;
    return 1;
}

int acl_set_on_access_granted(void (*handler)(void*, const char*))
{
    on_access_granted = handler;
    return 0;
}
int acl_set_on_access_denied(void (*handler)(void*, const char*))
{
    on_access_denied = handler;
    return 0;
}
int acl_set_on_change(void (*handler)(void*, const char*))
{
    on_change = handler;
    return 0;
}
