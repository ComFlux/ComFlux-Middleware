//
// Created by Sam Hubbard on 19/07/2016.
//

#ifndef TEST_FILE_H
#define TEST_FILE_H

#ifdef __ANDROID__
    #include "android/asset_manager_jni.h"
    void file_init_asset_mgr(JNIEnv* env, jobject asset_mgr_obj);
#endif // __ANDROID__

/*
 * Copy from a file into a variable.
 * Allocates on heap and returns the pointer.
 */
char* text_load_from_file(const char *filename);

#endif //TEST_FILE_H
