/*
 * hashmap.c
 *
 *  Created on: 3 Apr 2016
 *      Author: Raluca Diaconu
 */

#include "hashmap.h"



HashMap* map_new(int key_type)
{
	HashMap *map = (HashMap*)malloc(sizeof(HashMap));
	map->values = (Values*)malloc(sizeof(Values));

	map->key_type = key_type;

	if(key_type == KEY_TYPE_INT)
	{
		map->values->values_int = NULL;
	}
	else if(map->key_type == KEY_TYPE_STR)
	{
		map->values->values_str = NULL;
	}
	else if(map->key_type == KEY_TYPE_PTR)
	{
		map->values->values_ptr = NULL;
	}
	else
	{
		map_free(map);
		return NULL;
	}

	return map;
}

void map_free(HashMap *map)
{
	if(map == NULL)
		return;

	if(map->key_type == KEY_TYPE_INT)
	{
		map_int *s, *tmp, *values;
		/* free the hash table contents */
		values = map->values->values_int;
		HASH_ITER(hh, values, s, tmp)
		{
			HASH_DEL(values, s);
			free(s);
		}
		free(values);
	}
	else if(map->key_type == KEY_TYPE_STR)
	{
		map_str *s, *tmp, *values;
		/* free the hash table contents */
		values = map->values->values_str;
		HASH_ITER(hh, values, s, tmp)
		{
			HASH_DEL(values, s);
			free(s);
		}
		free(values);
	}
	else if(map->key_type == KEY_TYPE_PTR)
	{
		map_ptr *s, *tmp, *values;
		/* free the hash table contents */
		values = map->values->values_ptr;
		HASH_ITER(hh, values, s, tmp)
		{
			HASH_DEL(values, s);
			free(s);
		}
		free(values);
	}
	free(map);
}

int map_insert(HashMap *map, void *key_, void *value)
{
	if(map == NULL || key_ == NULL || value == NULL)
	{
		return -1;
	}

	if (map->key_type == KEY_TYPE_INT)
	{
		map_int* elem = NULL;

		HASH_FIND_INT(map->values->values_int, key_, elem);

		if (elem==NULL)
		{
			elem = (map_int*)malloc(sizeof(map_int));
			elem->_key_ = *((int*)key_);
			elem->_value_ = value;
			HASH_ADD_INT(map->values->values_int, _key_, elem);
		}
		else/* value was found */
		{
			elem->_value_ = value;
		}
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		map_str* elem = NULL;

		HASH_FIND_STR(map->values->values_str, key_, elem);

		if (elem == NULL)
		{
			elem = (map_str*)malloc(sizeof(map_str));
			elem->_key_ = strdup((char*)key_);
			elem->_value_ = value;
			HASH_ADD_STR(map->values->values_str, _key_, elem);
		}
		else/* value was found */
		{
			elem->_value_ = value;
		}
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		map_ptr* elem = NULL;

		HASH_FIND_PTR(map->values->values_ptr, &key_, elem);

		if (elem == NULL)
		{
			elem = (map_ptr*)malloc(sizeof(map_ptr));
			elem->_key_ = (char*)key_;
			elem->_value_ = value;
			HASH_ADD_PTR(map->values->values_ptr, _key_, elem);
		}
		else /* value was found */
		{
			elem->_value_ = value;
		}
	}
	else /* bad key */
	{
		return -1;
	}

	return 0;
}

/* insert acts as update */
int map_update(HashMap *map, void *key_, void *value)
{
	//map_remove(map, key_);
	return map_insert(map, key_, value);
}

int map_remove(HashMap *map, void *key_)
{
	if(map == NULL || key_ == NULL)
		return -1;

	if (map->key_type == KEY_TYPE_INT)
	{
		map_int* elem = NULL;
		HASH_FIND_INT(map->values->values_int, key_, elem);
		if (elem != NULL)
		{
			HASH_DEL(map->values->values_int, elem);
			return 0;
		}
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		map_str* elem = NULL;
		HASH_FIND_STR(map->values->values_str, key_, elem);
		if (elem != NULL)
		{
			HASH_DEL(map->values->values_str, elem);
			return 0;
		}
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		map_ptr* elem = NULL;
		HASH_FIND_PTR(map->values->values_ptr, &key_, elem);
		if (elem != NULL)
		{
			HASH_DEL(map->values->values_ptr, elem);
			return 0;
		}
	}

	return 1;
}

void* map_get(HashMap *map, void *key_)
{
	if(map == NULL || key_ == NULL)
		return NULL;

	if (map->key_type == KEY_TYPE_INT)
	{
		map_int* elem = NULL;
		HASH_FIND_INT(map->values->values_int, key_, elem);
		if(elem != NULL)
			return elem->_value_;
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		map_str* elem = NULL;
		HASH_FIND_STR(map->values->values_str, key_, elem);
		if(elem != NULL)
		{
			return elem->_value_;
		}
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		map_ptr* elem = NULL;
		HASH_FIND_PTR(map->values->values_ptr, &key_, elem);
		if(elem != NULL)
			return elem->_value_;
	}

	return NULL;
}

int map_contains(HashMap *map, void *key_)
{
	if(map == NULL || key_ == NULL)
		return -1;

	if (map->key_type == KEY_TYPE_INT)
	{
		map_int* elem = NULL;
		HASH_FIND_INT(map->values->values_int, key_, elem);
		if(elem != NULL)
			return 1;
		else
			return 0;
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		map_str* elem = NULL;
		HASH_FIND_STR(map->values->values_str, key_, elem);
		if(elem != NULL)
			return 1;
		else
			return 0;
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		map_ptr* elem = NULL;
		HASH_FIND_PTR(map->values->values_ptr, &key_, elem);
		if(elem != NULL)
			return 1;
		else
			return 0;
	}
	return -1;
}

unsigned int map_size(HashMap *map)
{
	if(map == NULL)
		return -1;

	if (map->key_type == KEY_TYPE_INT)
	{
		return HASH_COUNT(map->values->values_int);
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		return HASH_COUNT(map->values->values_str);
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		return HASH_COUNT(map->values->values_ptr);
	}

	return -1;
}

void map_foreach(HashMap *map, void (*f)(void* key, void *value) )
{
	if(map == NULL || f == NULL)
		return;

	if (map->key_type == KEY_TYPE_INT)
	{
		map_int* s = NULL;
		for (s = map->values->values_int ; s != NULL; s=s->hh.next)
		{
			f(&(s->_key_), s->_value_);
		}
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		map_str* s = NULL;
		for (s = map->values->values_str ; s != NULL; s=s->hh.next)
		{
			f(s->_key_, s->_value_);
		}
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		map_ptr* s = NULL;
		for (s = map->values->values_ptr ; s != NULL; s=s->hh.next)
		{
			f(s->_key_, s->_value_);
		}
	}
}

Array* map_get_keys(HashMap *map)
{
	if(map == NULL)
		return NULL;

	Array* keys = NULL;

	if (map->key_type == KEY_TYPE_INT)
	{
		map_int* s = NULL;
		keys = array_new(ELEM_TYPE_INT);
		for (s = map->values->values_int ; s != NULL; s=s->hh.next)
		{
			array_add(keys, &(s->_key_));
		}
	}
	else if (map->key_type == KEY_TYPE_STR)
	{
		map_str* s = NULL;
		keys = array_new(ELEM_TYPE_STR);
		for (s = map->values->values_str ; s != NULL; s=s->hh.next)
		{
			/* duplicate a string key to avoid str corruption */
			array_add(keys, strdup(s->_key_));
		}
	}
	else if (map->key_type == KEY_TYPE_PTR)
	{
		map_ptr* s = NULL;
		keys = array_new(ELEM_TYPE_PTR);
		for (s = map->values->values_ptr ; s != NULL; s=s->hh.next)
		{
			array_add(keys, s->_key_);
		}
	}

	return keys;
}

Array* map_get_values(HashMap *map)
{
	if(map == NULL)
		return NULL;

	/* hashmap values are pointers */
	Array* values = array_new(ELEM_TYPE_PTR);
	/* get keys and the values */
	Array* keys = map_get_keys(map);

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
	{
		array_free(keys);
		array_free(values);
		return NULL;
	}

	array_free(keys);
	return values;
}
