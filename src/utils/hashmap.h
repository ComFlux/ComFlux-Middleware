/*
 * hashmap.h
 *
 *  Created on: 3 Apr 2016
 *      Author: rad
 */

#ifndef HASHMAP_H_
#define HASHMAP_H_
/*
 * A wrapper for hash map implementation
 */

#include "uthash.h"
#include "array.h"

#define KEY_TYPE_PTR 0
#define KEY_TYPE_INT 1
#define KEY_TYPE_STR 2

typedef struct Value_{
	void * _key_;
	void * _value_;
	UT_hash_handle hh; /* makes this structure hashable */
} Value;

typedef struct HashMap_{
	int key_type;
	Value *values;
} HashMap;

/*
 * allocates memory and returns a hashmap object
 */
HashMap* map_new(int key_type);

/*
 * frees the memory of a hashmap
 */
void map_free(HashMap *map);

int map_insert(HashMap *map, void *key, void *value);

int map_update(HashMap *map, void *key, void *value);

int map_remove(HashMap *map, void *key);

void* map_get(HashMap *map, void *key);

int map_contains(HashMap *map, void *key);

unsigned int map_size(HashMap *map);

Array* map_get_keys(HashMap *map);

Array* map_get_values(HashMap *map);

void map_foreach(HashMap *map, void (*f)(void *key, void *val));

/* functions for key equality *
int key_equal_str(char *str1, char *str2);
int key_equal_int(int i1, int i2);
int key_equal_int(int i1, int i2);
*/

#endif /* HASHMAP_H_ */
