/*
 * acc_ssl.h
 *
 *  Created on: 22 Nov 2016
 *      Author: jd
 * Authenticating using ssl, check public key against ca file
 * Authorization is using a list of strings similar to the simple_acc
 */

#ifndef MODULES_ACC_MODULES_SSL_ACC_H_
#define MODULES_ACC_MODULES_SSL_ACC_H_

char* acl_get_token(void* object);

int acl_set_on_access_granted(void (*handler)(void*, const char*));
int acl_set_on_access_denied(void (*handler)(void*, const char*));
int acl_set_on_change(void (*handler)(void*, const char*));

#endif /* MODULES_ACC_MODULES_SIMPLE_ACC_H_ */
