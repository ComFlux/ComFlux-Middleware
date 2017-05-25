//
// Created by Sam Hubbard on 05/09/2016.
//

#ifndef MIDDLEWARE_MEMORY_H
#define MIDDLEWARE_MEMORY_H

#include <stddef.h>

/* check null pointer before duplicating string */
char* strdup_null(const char* str);

/*generate a random ascii string with the given length */
char* randstring(size_t length);

/*get the hash of the character string */
char* hash(char* str);

#endif //MIDDLEWARE_MEMORY_H
