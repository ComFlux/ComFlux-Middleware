/*
 * krb5_acc.c

 *
 *  Created on: Mar 7, 2017
 *      Author: jie
 *
 *  Kerberos needs to be installed and relevant uses need to be set.
 *  Refer to https://web.mit.edu/kerberos/krb5-1.3/krb5-1.3.4/doc/krb5-admin.html for more details.
 */

//#include "krb5.h"
#include "com_err.h"
#include "k5-int.h"

#include <json.h>
#include <hashmap.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

krb5_context context;
krb5_error_code ret;
krb5_creds mycreds;

int initiated;
void* auth_module;
HashMap* acc_table; /* hashmap<"name",hashmap<"peer_credential1":"" ; "peer_credential1":"", > > */
char* credential;
HashMap* cred_table;//

int acl_add_object(void* object);
int acl_rm_object(void* object);

void (*on_access_granted)(void*, const char*);
void (*on_access_denied)(void*, const char*);
void (*on_change)(void*, const char*);

int access_init(void* module, const char* config_json) {
	auth_module = module;
	acc_table = map_new(KEY_TYPE_STR);
    cred_table = map_new(KEY_TYPE_PTR);//

	if (acc_table)
		initiated = 1; // TRUE;
	char* hostname;
	char* password;
	if (config_json != NULL) {
		JSON* file = json_new(config_json);
		hostname = json_get_str(file, "hostname");
		password = json_get_str(file, "password");
		char* str = json_get_str(file, "access_granted");
		char* rest = str;
		char* token;
		while ((token = strtok_r(rest, ",", &rest))) {
			access_add_access(NULL, token);
		}
	}

	ret = krb5_init_context(&context);
	if (ret)
		com_err("", ret, "while initializing krb5");
	krb5_principal client_princ = NULL;
	memset(&mycreds, 0, sizeof(mycreds));
	ret = krb5_parse_name(context, hostname, &client_princ);
	if (ret)
		com_err("", ret, "krb5_parse_name");
	ret = krb5_get_init_creds_password(context, &mycreds, client_princ, password,
	NULL, NULL, 0, NULL, NULL);
	if (ret)
		com_err("", ret, "krb5_get_init_creds_password");

	return initiated;
}

int access_authenticate(void* cpt_cred, void* connection) {
	krb5_creds *creds=(krb5_creds*)cpt_cred;
	ret = krb5_verify_init_creds(context, creds, NULL, NULL, NULL, NULL);
		if (ret){
			com_err("", ret, "krb5_verify_init_creds");
		}else{//pass
			map_update(cred_table, connection, cpt_cred);//
			return 1;
		}
}

void access_disconnect(void* connection) //
{
	map_remove(cred_table, connection);
}

/*
 * check if the client in the subject (krb5 credential) in the list
 * subject is the krb5 credential object
 * object is the component or specific endpoint
 */
int access_has_access_credential(void* object, void* subject) {

	 HashMap* object_list;
	    if (object == NULL) {
	        object_list = (HashMap*)map_get(acc_table, "component");
	        // printf("hha %d\n", map_size(object_list));
	    } else {
	        object_list = (HashMap*)map_get(acc_table, object);
	    }

	    if (object_list == NULL)
	        return -1;

	    krb5_creds *creds=(krb5_creds*)subject; //krb5 cred passed in
	    char* name=(mycreds.client)->data->data;//get cred client name
	    return map_contains(object_list, name); //if the client is in the list
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

int access_add_access(void* object, void* subject) {
	//subject as a string
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

	    void* operation_value = map_insert(object_list, subject, "");
	    if (operation_value == NULL)
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

int access_rm_access(void* object, void* subject) {
	//subject as a string
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

char* access_get_credential() {
	return (void*) &mycreds;
}

int access_set_credential(char* cred) {
	mycreds=*((krb5_creds*)cred);
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

/*void main() {

	krb5_creds creds, *out_creds;
	krb5_principal client_princ = NULL;

	krb5_ccache id;

	memset(&creds, 0, sizeof(creds));
	ret = krb5_parse_name(context, "frank", &client_princ);
	if (ret)
		com_err("", ret, "krb5_parse_name");

	ret = krb5_cc_default(context, &id);
	if (ret)
		com_err("", ret, "krb5_cc_default");
	ret = krb5_cc_initialize(context, id, client_princ);
	if (ret)
		com_err("", ret, "krb5_cc_initialize");

	ret = krb5_get_init_creds_password(context, &creds, client_princ, "frank",
	NULL, NULL, 0, NULL, NULL);
	if (ret)
		com_err("", ret, "krb5_get_init_creds_password");

	ret = krb5_verify_init_creds(context, &creds, NULL, NULL, NULL, NULL);
	if (ret)
		com_err("", ret, "krb5_verify_init_creds");

	//get cred
//	 ret = krb5_parse_name(context, "frank@test.com",
//	                          &creds.client);
//	    if (ret)
//	    	com_err(context, 1, ret, "krb5_parse_name");
//
//
	printf("1\n");
	ret = krb5_cc_store_cred(context, id, &creds);
	if (ret)
		com_err("", ret, "krb5_cc_store_cred");

	printf("123\n");
	ret = krb5_parse_name(context, "host/test1.test.com", &creds.server);
	if (ret)
		com_err("", ret, "krb5_parse_name");

	ret = krb5_get_credentials(context, 0, id, &creds, &out_creds);
	if (ret) {
		com_err("", ret, "krb5_get_credentials");
	}

	printf("123suf3\n");

}*/
