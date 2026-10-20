//
// Created by maks on 06.01.2025.
//
// LWJGL loader hook with DroidBridge-compatible RenderSpec provider support.
// The DBL OpenGL proxy path is opt-in: existing renderer configurations retain
// their current behavior unless they request a libGLDroidBridge*.so sentinel.
//

#include <android/log.h>
#include <jni.h>
#include <environ/environ.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void* maybe_load_vulkan(void);

#define LWJGL_HOOK_TAG "LwjglLinkerHook"
#define DB_OPENGL_PROXY_SONAME "libGLDroidBridge.so"
#define DB_OPENGL_PROXY_ALT_SONAME "libGLDroidBridgeMesa.so"

static const char* library_basename(const char* filename) {
    if (filename == NULL) return "";
    const char* slash = strrchr(filename, '/');
    return slash != NULL ? slash + 1 : filename;
}

static bool is_vulkan_loader(const char* filename) {
    const char* base = library_basename(filename);
    return strcmp(base, "libvulkan.so") == 0 ||
           strcmp(base, "libvulkan.so.1") == 0;
}

static bool is_droidbridge_opengl_proxy(const char* filename) {
    const char* base = library_basename(filename);
    return strcmp(base, DB_OPENGL_PROXY_SONAME) == 0 ||
           strcmp(base, DB_OPENGL_PROXY_ALT_SONAME) == 0;
}

static const char* first_non_empty(const char* a, const char* b,
                                   const char* c, const char* d,
                                   const char* e) {
    if (a != NULL && a[0] != '\0') return a;
    if (b != NULL && b[0] != '\0') return b;
    if (c != NULL && c[0] != '\0') return c;
    if (d != NULL && d[0] != '\0') return d;
    if (e != NULL && e[0] != '\0') return e;
    return NULL;
}

static void* try_dlopen_logged(const char* library, int mode) {
    if (library == NULL || library[0] == '\0') return NULL;
    dlerror();
    void* handle = dlopen(library, mode);
    if (handle == NULL) {
        const char* error = dlerror();
        __android_log_print(ANDROID_LOG_WARN, LWJGL_HOOK_TAG,
                            "RenderSpec could not open %s: %s", library,
                            error != NULL ? error : "unknown error");
    } else {
        __android_log_print(ANDROID_LOG_INFO, LWJGL_HOOK_TAG,
                            "RenderSpec opened %s", library);
    }
    return handle;
}

/*
 * Resolve the same EGL provider selected by DroidBridge's RenderSpec setup.
 * This branch only activates for the explicit DBL proxy SONAMEs.
 */
static void* acquire_droidbridge_egl_provider(int mode) {
    if ((mode & (RTLD_NOW | RTLD_LAZY)) == 0) mode |= RTLD_NOW;
    mode |= RTLD_GLOBAL;

    const char* preferred = first_non_empty(
            getenv("DROIDBRIDGE_RENDERSPEC_EGL"),
            getenv("DROIDBRIDGE_MESA_EGL"),
            getenv("POJAVEXEC_EGL"),
            getenv("POJAV_EGL_LIBRARY"),
            getenv("POJAVEXEC_EGL_LIBRARY"));

    void* handle = try_dlopen_logged(preferred, mode);
    if (handle != NULL) return handle;
    handle = try_dlopen_logged("libEGL_mesa.so", mode);
    if (handle != NULL) return handle;
    return try_dlopen_logged("libEGL.so", mode);
}

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

    int mode = (int) jmode;
    if (is_vulkan_loader(filename)) {
        __android_log_print(ANDROID_LOG_INFO, LWJGL_HOOK_TAG,
                            "Redirecting Vulkan load request: %s", filename);
        void* handle = maybe_load_vulkan();
        if (handle != NULL) return (jlong) handle;

        __android_log_print(ANDROID_LOG_WARN, LWJGL_HOOK_TAG,
                            "Custom Vulkan loader failed for %s; falling back to dlopen",
                            filename);
    }

    if (is_droidbridge_opengl_proxy(filename)) {
        __android_log_print(ANDROID_LOG_INFO, LWJGL_HOOK_TAG,
                            "Matched DroidBridge OpenGL proxy: %s", filename);
        void* handle = acquire_droidbridge_egl_provider(mode);
        if (handle != NULL) return (jlong) handle;
        __android_log_print(ANDROID_LOG_WARN, LWJGL_HOOK_TAG,
                            "DroidBridge EGL provider unavailable; falling back to normal dlopen");
    }

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

void installLwjglDlopenHook(void) {
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
