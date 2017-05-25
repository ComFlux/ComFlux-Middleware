/*
 * array.h
 *
 *  Created on: 4 Apr 2016
 *      Author: rad
 */

#ifndef ARRAY_H_
#define ARRAY_H_
/*
 * A wrapper for an array implementation
 */

#include "utarray.h"


#define ELEM_TYPE_PTR 0
#define ELEM_TYPE_INT 1
#define ELEM_TYPE_STR 2


typedef struct Array_{
	int elem_type;
	UT_array *ua;
} Array;



Array* array_new(int elem_type);

void array_free(Array *array);

int array_add(Array *array, void *elem);

int array_remove(Array *array, void *elem);

int array_remove_index(Array *array, int index);

void* array_get(Array *array, int index);

unsigned int array_size(Array *array);

void array_foreach(Array *const array, void (*f)(void *));

#endif /* COMMON_ARRAY_H_ */
