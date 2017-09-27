//
// Created by Sam Hubbard on 05/09/2016.
//

#include "utils.h"

#include "slog.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

char* strdup_null(const char* str)
{
    if (str == NULL)
        return NULL;

    errno = 0;
    char* result = strdup(str);
    if (errno != 0) {
        //slog(SLOG_ERROR, "Malloc error: %s", strerror(errno));
        //slog(SLOG_ERROR, "while duplicating: *%s*, strlen: %d \n", str, strlen(str));
        errno = 0;
    }
    return result;
}

char *randstring(size_t length)
{
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *randomString = NULL;

    if (length) {
    	//srand (time(NULL));
        randomString = malloc(sizeof(char) * (length +1));

        if (randomString) {
            int n;
            for ( n = 0;n < length;n++) {
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }

            randomString[length] = '\0';
        }
    }

    return randomString;
}

uint64_t hash_long(char *str)
{
	//printf("-----------hash of %s\n", str);
    uint64_t hash = 5381;
    int32_t c, i;

    for(i = 0; i<strlen(str); i++)
    {
    	c = (int32_t)(str[i]);
        hash = ((hash << 5) + hash) + c; ///* hash * 33 + c
    }
    return hash;
}

char* hash( char *str)
{
	if (str == NULL)
		return NULL;

    uint64_t hash_ = hash_long(str);
    const int n = snprintf(NULL, 0, "%" PRIu64, hash_);
    char* hash = (char*)malloc((n+1)*sizeof(char));
    snprintf(hash, n+1, "%" PRIu64, hash_);

    //slog(SLOG_DEBUG, "-----------hash is %s", hash);
    return hash;
}
