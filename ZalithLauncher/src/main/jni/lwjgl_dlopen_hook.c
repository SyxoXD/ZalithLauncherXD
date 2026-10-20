//
// Created by maks on 06.01.2025.
//

#include <android/log.h>
#include <jni.h>

#include <environ/environ.h>

#include <dlfcn.h>
#include <string.h>

extern void* maybe_load_vulkan();

#define LWJGL_HOOK_TAG "LwjglLinkerHook"

/*
 * Android Vulkan loaders are normally exposed as libvulkan.so. Some LWJGL
 * integrations may pass a full path or the versioned SONAME instead, so match
 * the basename rather than assuming the name starts at byte zero.
 */
static const char* library_basename(const char* filename) {
    if (filename == NULL) return "";
    const char* slash = strrchr(filename, '/');
    return slash != NULL ? slash + 1 : filename;
}

static int is_vulkan_loader(const char* filename) {
    const char* base = library_basename(filename);
    return strcmp(base, "libvulkan.so") == 0 ||
           strcmp(base, "libvulkan.so.1") == 0;
}

/*
 * LWJGL's ndlopen() hook lets the launcher provide its selected Vulkan loader
 * and perform dlopen() from the launcher process namespace. Keep the original
 * dlopen path for all non-Vulkan libraries (including mod-provided libraries).
 */
static jlong ndlopen_bugfix(__attribute__((unused)) JNIEnv *env,
                            __attribute__((unused)) jclass clazz,
                            jlong filename_ptr,
                            jint jmode) {
    const char* filename = (const char*) filename_ptr;
    if (filename == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LWJGL_HOOK_TAG,
                            "LWJGL ndlopen received a null library name");
        return (jlong) 0;
    }

    if (is_vulkan_loader(filename)) {
        __android_log_print(ANDROID_LOG_INFO, LWJGL_HOOK_TAG,
                            "Redirecting Vulkan load request: %s", filename);
        void* handle = maybe_load_vulkan();
        if (handle == NULL) {
            __android_log_print(ANDROID_LOG_ERROR, LWJGL_HOOK_TAG,
                                "Custom Vulkan loader failed for request: %s", filename);
        }
        return (jlong) handle;
    }

    int mode = (int) jmode;
    dlerror();
    void* handle = dlopen(filename, mode);
    if (handle == NULL) {
        const char* error = dlerror();
        __android_log_print(ANDROID_LOG_WARN, LWJGL_HOOK_TAG,
                            "dlopen failed for %s: %s", filename,
                            error != NULL ? error : "unknown error");
    }
    return (jlong) handle;
}

/*
 * Install the LWJGL dlopen hook. The Java signature must remain (JI)J.
 */
void installLwjglDlopenHook() {
    __android_log_print(ANDROID_LOG_INFO, LWJGL_HOOK_TAG,
                        "Installing LWJGL dlopen() hook");

    if (pojav_environ == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LWJGL_HOOK_TAG,
                            "Pojav runtime environment is unavailable");
        return;
    }

    JNIEnv* env = pojav_environ->runtimeJNIEnvPtr_JRE;
    if (env == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LWJGL_HOOK_TAG,
                            "JRE JNI environment is unavailable");
        return;
    }

    jclass dynamicLinkLoader = (*env)->FindClass(
            env, "org/lwjgl/system/linux/DynamicLinkLoader");
    if (dynamicLinkLoader == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LWJGL_HOOK_TAG,
                            "Failed to find DynamicLinkLoader");
        (*env)->ExceptionClear(env);
        return;
    }

    JNINativeMethod ndlopenMethod[] = {
            {"ndlopen", "(JI)J", (void*) &ndlopen_bugfix}
    };
    if ((*env)->RegisterNatives(env, dynamicLinkLoader, ndlopenMethod, 1) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, LWJGL_HOOK_TAG,
                            "Failed to register the ndlopen hook");
        (*env)->ExceptionClear(env);
    }
    (*env)->DeleteLocalRef(env, dynamicLinkLoader);
}
