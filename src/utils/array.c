/*
 * array.c
 *
 *  Created on: 4 Apr 2016
 *      Author: rad
 */


#include "array.h"


UT_icd ptr_icd = {sizeof(void*), NULL, NULL, NULL };

Array* array_new(int elem_type)
{
	Array *array = (Array*)malloc(sizeof(Array));

	array->elem_type = elem_type;

	if(elem_type == ELEM_TYPE_INT)
		utarray_new(array->ua, &ut_int_icd);
	else if(elem_type == ELEM_TYPE_STR)
		utarray_new(array->ua, &ut_str_icd);
	else if(elem_type == ELEM_TYPE_PTR)
		utarray_new(array->ua, &ut_ptr_icd);

	else
	{
		free(array);
		return NULL;
	}

	return array;
}

void array_free(Array *array)
{
	if(array == NULL)
		return;
	int i;
	char* key;
	for(i=0;i<array_size(array); i++)
	{
		//key = array_get(array, i);
		array_remove_index(array, i);
		//free(key);
	}
	utarray_free(array->ua);
	free(array);
}

int array_add(Array *array, void *elem)
{
	if(array->elem_type == ELEM_TYPE_INT)
	{
		utarray_push_back(array->ua, elem);
	}
	else if(array->elem_type == ELEM_TYPE_STR)
	{
		utarray_push_back(array->ua, &elem);
	}
	else if(array->elem_type == ELEM_TYPE_PTR)
	{
		utarray_push_back(array->ua, &elem);
	}

	else
	{
		return -1;
	}

	return 0;
}

static int strsort(const void *_a, const void *_b)
{
    char *a = *(char**)_a;
    char *b = *(char**)_b;
    return strcmp(a,b);
}

int array_remove(Array *array, void *elem)
{
	if (array == NULL)
		return -1;
	char **elem_ptr = utarray_find(array->ua, &elem, strsort);
	if(elem_ptr == NULL)
		return -1;
	/*else
		printf ("1 %s\n", *elem_ptr);

	char *ep=*elem_ptr;
	int elem_idx = utarray_eltidx(array->ua, elem_ptr);
	printf ("2 %d \n", elem_idx);*/
	utarray_erase(array->ua, utarray_eltidx(array->ua, elem_ptr), 1);
	//printf ("3\n");

	return 0;//g_ptr_array_remove(array, elem);
}

int array_remove_index(Array *array, int index)
{
	if(index >= array_size(array))
		return 1;
	if(index < 0)
		return -1;
	utarray_erase(array->ua, index, 1);
	return 0;//g_ptr_array_remove(array, elem);
}

void* array_get(Array *array, int index)
{
	if(index >= array_size(array) || index < 0 || array->ua == NULL)
		return NULL;

	int i;
	void *p=NULL;
	for (i=0; i<=index; i= i+1)
	{
		p=utarray_next(array->ua,p);
	}

	if(array->elem_type == ELEM_TYPE_INT)
		return (void*)p;
	else if(array->elem_type == ELEM_TYPE_STR)
		return (void*)*(char**)p;
	else if(array->elem_type == ELEM_TYPE_PTR)
		return (void*)*(void**)p;
	else
	{
		return NULL;
	}
}

unsigned int array_size(Array *array)
{
	return utarray_len(array->ua);
}

void array_foreach(Array *const array, void (*f)(void *))
{
	unsigned int i;
	for (i = 0; i < array_size(array); i++)
	{
		f(array_get(array, i));
	}
}
