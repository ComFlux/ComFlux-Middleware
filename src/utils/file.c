//
// Created by Sam Hubbard on 19/07/2016.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"


#ifdef __ANDROID__

AAssetManager* asset_mgr;

void file_init_asset_mgr(JNIEnv* env, jobject asset_mgr_obj)
{
    asset_mgr = AAssetManager_fromJava(env, asset_mgr_obj);
}

char* text_load_from_file(const char* filename)
{
    AAsset* asset = AAssetManager_open(asset_mgr, filename, AASSET_MODE_STREAMING);

    if (asset == NULL)
        return NULL;

    off_t length = AAsset_getLength(asset);
    char* var = (char*) malloc(((size_t) length + 1) * sizeof (char));
    var[length] = '\0';

    AAsset_read(asset, var, (size_t) length);

    AAsset_close(asset);

    return var;
}

#else

char* text_load_from_file(const char* filename)
{
    FILE* _file = fopen(filename, "r");

    if (_file == NULL)
        return NULL;

    /* get size of file */
    fseek(_file, 0L, SEEK_END);
    int size = (int) ftell(_file);
    rewind(_file);

    /* read file */
    char* var;
    var = (char*) malloc((size_t) size + 1);
    memset(var, 0, (size_t) size + 1);

    fread (var, 1, (size_t) size, _file);
    var[size] = '\0';
    fclose(_file);

    /* return text */
    return var;
}

#endif
