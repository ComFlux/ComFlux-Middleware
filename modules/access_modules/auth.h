/*
 * auth.h
 *
 *  Created on: 22 Mar 2017
 *      Author: Raluca Diaconu
 */

#ifndef ACCESS_H_
#define ACCESS_H_


int access_init(void* module, const char* config_json);

int access_authenticate(void* credential, void* connection);

void access_disconnect(void* connection);

int access_has_access(void* endpoint, void* connection);

int access_has_access_credential(void* endpoint, void* credential);


int access_add_access(void* endpoint, void* credential);

int access_rm_access(void* endpoint, void* credential);


/*
 * add credential for the component
 * not yet used
 */
int access_set_credential(char* credential);

/*
 * no yet used
 */
char* access_get_credential();

#endif
