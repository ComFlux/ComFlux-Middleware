/*
 * hashmap.c
 *
 *  Created on: 3 Apr 2016
 *      Author: Raluca Diaconu
 */

#include "hashmap.h"

//HashMap *map = map_new(KEY_TYPE_PTR)
HashMap* map_new(int key_type)
{
	HashMap *map = (HashMap*)malloc(sizeof(HashMap));
	map->values = NULL;
	map->key_type = key_type;

	return map;
}

void map_free(HashMap *map)
{
	Value *s, *tmp;
	/* free the hash table contents */
	HASH_ITER(hh, map->values, s, tmp)
	{
	  HASH_DEL(map->values, s);
	  free(s);
	}
	free(map);

	//HASH_CLEAR(hh,users);
}

int map_insert(HashMap *map, void *key_, void *value)
{
	Value *elem = map_get(map, key_);
	if (elem != NULL)
	{
		map_update(map, key_, value);
		return 0;
	}

	elem = (Value*)malloc(sizeof(Value));
	elem->_value_ = value;
	if (map->key_type == KEY_TYPE_INT)
	{
		//("__ins key %d", *((int*)key_));
		elem->_key_ = (int*) malloc(sizeof(int));
		*((int*)(elem->_key_)) = *((int*)key_);
		//HASH_ADD_INT(map->values, _key_, elem);
		/* doesn't work because they are ptrs */
		HASH_ADD_KEYPTR( hh, map->values, key_, sizeof(int), elem );
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		elem->_key_ = key_;
		HASH_ADD_STR(map->values, _key_, elem);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		elem->_key_ = key_;
		HASH_ADD_PTR(map->values, _key_, elem);
	}
	else
	{
		return -1;
	}

	return 0;
}

int map_update(HashMap *map, void *key_, void *value)
{
	map_remove(map, key_);

	map_insert(map, key_, value);

	return 0;

	Value *elem, *tmp;
	if(!map_contains(map, key_))
		return -1;

	elem = (Value*) malloc(sizeof(Value));
	if (map->key_type == KEY_TYPE_INT)
	{
		elem->_key_ = (int*) malloc(sizeof(int));
		*((int*)(elem->_key_)) = *((int*)key_);
		HASH_REPLACE_INT(map->values, _key_, elem, tmp);
		/* id: name of key field */
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		elem->_key_ = (char*) malloc((strlen(key_)+1)*sizeof(char));
		strcpy(elem->_key_, key_);
		HASH_REPLACE_STR(map->values, _key_, elem, tmp);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		elem->_key_ = key_;
		HASH_REPLACE_PTR(map->values, _key_, elem, tmp);
	}
	else
	{
		return -1;
	}

	if(tmp)
		HASH_DEL(map->values, tmp);
		free(tmp);

	return 0;
}

int map_remove(HashMap *map, void *key_)
{
	Value *elem = NULL;
	if (map->key_type == KEY_TYPE_INT)
	{
		HASH_FIND_INT(map->values, key_, elem);
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		HASH_FIND_STR(map->values, key_, elem);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		HASH_FIND_PTR(map->values, &key_, elem);
	}

	if (elem != NULL)
	{
		HASH_DEL(map->values, elem);
		return 0;
	}
	else
		return 1;
}

void* map_get(HashMap *map, void *key_)
{
	if(map == NULL || key_ == NULL)
		return NULL;

	Value *elem = NULL;
	if (map->key_type == KEY_TYPE_INT)
	{
		HASH_FIND_INT(map->values, key_, elem);
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		HASH_FIND_STR(map->values, key_, elem);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		HASH_FIND_PTR(map->values, &key_, elem);
	}

	if (elem == NULL)
	{
		return NULL;
	}

	return elem->_value_;
}

int map_contains(HashMap *map, void *key_)
{
	Value *elem = NULL;
	if (map->key_type == KEY_TYPE_INT)
	{
		HASH_FIND_INT(map->values, key_, elem);
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		HASH_FIND_STR(map->values, key_, elem);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		HASH_FIND_PTR(map->values, &key_, elem);
	}

	if(elem != NULL)
		return 1;
	else
		return 0;
}

unsigned int map_size(HashMap *map)
{
	return HASH_COUNT(map->values);
}

void map_foreach(HashMap *map, void (*f)(void* key, void *value) )
{
	Value *s;
	for (s = map->values; s != NULL; s=s->hh.next)
	{
		f(s->_key_, s->_value_);
	}
}

Array* map_get_keys(HashMap *map)
{
	Array* keys;
	if (map->key_type == KEY_TYPE_INT)
	{
		keys = array_new(ELEM_TYPE_INT);
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		keys = array_new(ELEM_TYPE_STR);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		keys = array_new(ELEM_TYPE_PTR);
	}
	else
		return NULL;

	Value *s;
	for(s=map->values; s != NULL; s=s->hh.next)
	{
		if (map->key_type == KEY_TYPE_STR)
			array_add(keys, strdup(s->_key_));
		else
			array_add(keys, s->_key_);
	}

	return keys;
}

Array* map_get_values(HashMap *map)
{
	Array* keys = map_get_keys(map);
	Array* values = array_new(ELEM_TYPE_PTR);
	if (map->key_type == KEY_TYPE_INT)
	{
		int i;
		int key;
		for(i= 0; i<array_size(keys); i++)
		{
			key = (int)array_get(keys, i);
			array_add(values, map_get(map, &key));
		}
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		int i;
		char* key;
		for(i= 0; i<array_size(keys); i++)
		{
			key = (char*)array_get(keys, i);
			array_add(values, map_get(map, key));
		}
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		int i;
		void* key;
		for(i= 0; i<array_size(keys); i++)
		{
			key = array_get(keys, i);
			array_add(values, map_get(map, key));
		}
	}
	else
		return NULL;

	return values;
}
