/*
 * simple_acc.h
 *
 *  Created on: 10 Nov 2016
 *      Author: rad
 */

#ifndef MODULES_ACC_MODULES_SIMPLE_ACC_H_
#define MODULES_ACC_MODULES_SIMPLE_ACC_H_

#include "../../access_modules/auth.h"

int acl_set_on_access_granted(void (*handler)(void*, const char*));
int acl_set_on_access_denied(void (*handler)(void*, const char*));
int acl_set_on_change(void (*handler)(void*, const char*));

#endif /* MODULES_ACC_MODULES_SIMPLE_ACC_H_ */
