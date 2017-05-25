/*
 * krb5_acc.h
 *
 *  Created on: Mar 7, 2017
 *      Author: jie
 */

#ifndef MODULES_ACC_MODULES_KRB5_ACC_H_
#define MODULES_ACC_MODULES_KRB5_ACC_H_


int acl_set_on_access_granted(void (*handler)(void*, const char*));
int acl_set_on_access_denied(void (*handler)(void*, const char*));
int acl_set_on_change(void (*handler)(void*, const char*));



#endif /* MODULES_ACC_MODULES_KRB5_ACC_H_ */
