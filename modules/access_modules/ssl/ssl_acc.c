/*
 * ssl_acc.c
 *
 *  Created on: 22 Nov 2016
 *      Author: jd
 */

#include "../../access_modules/ssl/ssl_acc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>
#include <hashmap.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

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

X509_STORE* x509store;

int access_init(void* module, const char* config_json)
{
    auth_module = module;
    acc_table = map_new(KEY_TYPE_STR);
    cred_table = map_new(KEY_TYPE_PTR);//

    if (acc_table)
        initiated = 1; // TRUE;

    if (config_json != NULL) {
        JSON* jfile = json_new(config_json);
        // set public key as credential
        char* buffer = 0;
        long length;
        FILE* pkfile = fopen(json_get_str(jfile, "public_key"), "rb");
        fseek(pkfile, 0, SEEK_END);
        length = ftell(pkfile);
        fseek(pkfile, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer) {
            fread(buffer, 1, length, pkfile);
        }
        fclose(pkfile);
        access_set_credential(buffer);
        // set ca file
        x509store = X509_STORE_new();
        if (x509store == NULL) {
            fprintf(stderr, "unable to create new X509 store.\n");
            return NULL;
        }
        int rc = X509_STORE_load_locations(x509store, json_get_str(jfile, "ca_file"), NULL);
        if (rc != 1) {
            fprintf(stderr, "unable to load certificates at %s to store\n", json_get_str(jfile, "ca_file"));
            X509_STORE_free(x509store);
            return NULL;
        }
        // set default access lists
        char* str = json_get_str(jfile, "access_granted");
        char* rest = str;
        char* token;
        while ((token = strtok_r(rest, ",", &rest))) {
            access_add_access(NULL, token);
        }
    }
    return initiated;
}
int access_authenticate(void* cpt_cred, void* connection) // cpt_cred is the public key in char*
{
    //creat store ctx
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
    if (!ctx) {
        fprintf(stderr, "unable to create STORE CTX\n");
        return -1;
    }
    //read public key from cpt_cred
    BIO* certbio = BIO_new(BIO_s_mem());
    BIO_puts(certbio, cpt_cred);
    X509* pkey = PEM_read_bio_X509(certbio, NULL, NULL, NULL);
    //init and check
    if (X509_STORE_CTX_init(ctx, x509store, pkey, NULL) != 1) {
        fprintf(stderr, "unable to initialize STORE CTX.\n");
        X509_STORE_CTX_free(ctx);
        return -1;
    }
    map_update(cred_table, connection, cpt_cred);//
    return 1;
    //return X509_verify_cert(ctx);
}

void access_disconnect(void* connection) //
{
	map_remove(cred_table, connection);
}

/*object is the endpoint
 * subject is the public key, thus process is required to get the real "Subject: CN=clent2", 
 * which is the info stored in the list
 * */

int access_has_access_credential(void* object, void* subject)
{
   HashMap* object_list;
    if (object == NULL) {
        object_list = (HashMap*)map_get(acc_table, "component");
        // printf("hha %d\n", map_size(object_list));
    } else {
        object_list = (HashMap*)map_get(acc_table, object);
    }

    if (object_list == NULL)
        return -1;

    return map_contains(object_list, subject);
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

int access_add_access(void* object, void* subject) //object is the public key
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

    void* operation_value = map_insert(object_list, subject, ""); //fix this 
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

int access_rm_access(void* object, void* subject) //object is the public key
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

/*Just put here at the moment, may not be useful as the ep pass the string over as crentidial as ready, no need to process the public key
 */
char* acl_get_token(void* certstring)
{
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_puts(bio,(char*)certstring);
    X509* pkey = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    if(pkey==NULL){
        return NULL;
    }
    char* fulsub = X509_NAME_oneline(X509_get_subject_name(pkey), NULL, 0);
    char* secsub = strchr(fulsub+3,'/');
    if(!secsub)
    {
        secsub=(char*)malloc(0);
    }
    char* sub =(char*)malloc(strlen(fulsub)-strlen(secsub)-3);
    memcpy(sub,fulsub+4,strlen(fulsub)-strlen(secsub)-4);
    return sub;
}
