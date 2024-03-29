#include <jni.h>
#include <string>

#include <jni.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <android/log.h>
#include "MMKV.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_blend_ndkadvanced_mmkv_MMKV_jniInitialize(JNIEnv *env, jclass clazz, jstring rootDir_) {
    const char *rootDir = env->GetStringUTFChars(rootDir_, 0);

    MMKV::initializeMMKV(rootDir);

    env->ReleaseStringUTFChars(rootDir_, rootDir);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_blend_ndkadvanced_mmkv_MMKV_getDefaultMMKV(JNIEnv *env, jclass clazz) {
    MMKV *kv = MMKV::defaultMMKV();
    return reinterpret_cast<jlong>(kv);
}

/**
 * 写入数据
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_blend_ndkadvanced_mmkv_MMKV_putInt(JNIEnv *env, jobject thiz, jlong handle, jstring key_,
                                            jint value) {
    const char *key = env->GetStringUTFChars(key_, 0);
    MMKV *kv = reinterpret_cast<MMKV *>(handle);
    kv->putInt(key, value);
    env->ReleaseStringUTFChars(key_, key);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_blend_ndkadvanced_mmkv_MMKV_getInt(JNIEnv *env, jobject thiz, jlong handle, jstring key_,
                                            jint defaultValue) {
    const char *key = env->GetStringUTFChars(key_, 0);
    MMKV *kv = reinterpret_cast<MMKV *>(handle);

    int returnValue = kv->getInt(key, defaultValue);

    env->ReleaseStringUTFChars(key_, key);
    return returnValue;
}