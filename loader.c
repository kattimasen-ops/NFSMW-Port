#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

// ============================================================================
// JNI MOCK UMGEBUNG (VERHINDERT CRASH BEI JNI_OnLoad)
// ============================================================================

typedef int32_t jint;
typedef int64_t jlong;
typedef uint8_t jboolean;
typedef void* jobject;
typedef void* jclass;
typedef void* jstring;
typedef void* jarray;
typedef void* jmethodID;
typedef void* jfieldID;

#define JNI_OK 0
#define JNI_ERR (-1)
#define JNI_VERSION_1_6 0x00010006

struct JNINativeInterface_struct;
struct JNIInvokeInterface_struct;

typedef const struct JNINativeInterface_struct* JNIEnv;
typedef const struct JNIInvokeInterface_struct* JavaVM;

static jint JNI_GetVersion(JNIEnv *env) { (void)env; return JNI_VERSION_1_6; }
static jclass JNI_FindClass(JNIEnv *env, const char *name) { (void)env; (void)name; return (jclass)1; }
static jmethodID JNI_GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig) { (void)env; (void)clazz; (void)name; (void)sig; return (jmethodID)1; }
static jmethodID JNI_GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig) { (void)env; (void)clazz; (void)name; (void)sig; return (jmethodID)1; }
static jobject JNI_CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) { (void)env; (void)obj; (void)methodID; return NULL; }
static jint JNI_CallIntMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) { (void)env; (void)obj; (void)methodID; return 0; }
static jboolean JNI_CallBooleanMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) { (void)env; (void)obj; (void)methodID; return 0; }
static jstring JNI_NewStringUTF(JNIEnv *env, const char *bytes) { (void)env; (void)bytes; return (jstring)1; }
static const char* JNI_GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy) { (void)env; (void)string; if (isCopy) *isCopy = 0; return ""; }
static void JNI_ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *utf) { (void)env; (void)string; (void)utf; }

struct JNINativeInterface_struct {
    void *reserved0; void *reserved1; void *reserved2; void *reserved3;
    jint (*GetVersion)(JNIEnv *);
    void *DefineClass;
    jclass (*FindClass)(JNIEnv *, const char *);
    void *FromReflectedMethod; void *FromReflectedField; void *ToReflectedMethod;
    void *GetSuperclass; void *IsAssignableFrom; void *ToReflectedField;
    void *Throw; void *ThrowNew; void *ExceptionOccurred; void *ExceptionDescribe;
    void *ExceptionClear; void *FatalError; void *PushLocalFrame; void *PopLocalFrame;
    void *NewGlobalRef; void *DeleteGlobalRef; void *DeleteLocalRef; void *IsSameObject;
    void *NewLocalRef; void *EnsureLocalCapacity; void *AllocObject; void *NewObject;
    void *NewObjectV; void *NewObjectA; void *GetObjectClass; void *IsInstanceOf;
    jmethodID (*GetMethodID)(JNIEnv *, jclass, const char *, const char *);
    jobject (*CallObjectMethod)(JNIEnv *, jobject, jmethodID, ...);
    void *CallObjectMethodV; void *CallObjectMethodA;
    jboolean (*CallBooleanMethod)(JNIEnv *, jobject, jmethodID, ...);
    void *CallBooleanMethodV; void *CallBooleanMethodA;
    void *CallByteMethod; void *CallByteMethodV; void *CallByteMethodA;
    void *CallCharMethod; void *CallCharMethodV; void *CallCharMethodA;
    void *CallShortMethod; void *CallShortMethodV; void *CallShortMethodA;
    jint (*CallIntMethod)(JNIEnv *, jobject, jmethodID, ...);
    void *CallIntMethodV; void *CallIntMethodA;
    void *reserved_rest[200];
};

static const struct JNINativeInterface_struct g_jni_env_vtbl = {
    .GetVersion = JNI_GetVersion,
    .FindClass = JNI_FindClass,
    .GetMethodID = JNI_GetMethodID,
    .CallObjectMethod = JNI_CallObjectMethod,
    .CallBooleanMethod = JNI_CallBooleanMethod,
    .CallIntMethod = JNI_CallIntMethod
};

static const struct JNINativeInterface_struct *g_jni_env_ptr = &g_jni_env_vtbl;
static JNIEnv g_jni_env = &g_jni_env_ptr;

static jint JVM_GetEnv(JavaVM *vm, void **env, jint version) {
    (void)vm; (void)version;
    if (env) *env = (void*)g_jni_env;
    return JNI_OK;
}

static jint JVM_AttachCurrentThread(JavaVM *vm, void **p_env, void *thr_args) {
    (void)vm; (void)thr_args;
    if (p_env) *p_env = (void*)g_jni_env;
    return JNI_OK;
}

static jint JVM_DetachCurrentThread(JavaVM *vm) {
    (void)vm;
    return JNI_OK;
}

struct JNIInvokeInterface_struct {
    void *reserved0; void *reserved1; void *reserved2;
    void *DestroyJavaVM;
    jint (*AttachCurrentThread)(JavaVM *, void **, void *);
    jint (*DetachCurrentThread)(JavaVM *);
    jint (*GetEnv)(JavaVM *, void **, jint);
    jint (*AttachCurrentThreadAsDaemon)(JavaVM *, void **, void *);
};

static const struct JNIInvokeInterface_struct g_java_vm_vtbl = {
    .DestroyJavaVM = NULL,
    .AttachCurrentThread = JVM_AttachCurrentThread,
    .DetachCurrentThread = JVM_DetachCurrentThread,
    .GetEnv = JVM_GetEnv,
    .AttachCurrentThreadAsDaemon = JVM_AttachCurrentThread
};

static const struct JNIInvokeInterface_struct *g_java_vm_ptr = &g_java_vm_vtbl;
static JavaVM g_java_vm = &g_java_vm_ptr;

// ============================================================================
// EXPORTIERTE STUBS FÜR C++, LOGGING UND JNI CONTROLLER
// ============================================================================

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    (void)prio;
    va_list args;
    printf("[%s] ", tag ? tag : "NFS");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    return 0;
}

int __android_log_write(int prio, const char *tag, const char *text) {
    (void)prio;
    printf("[%s] %s\n", tag ? tag : "NFS", text ? text : "");
    return 0;
}

void Java_com_ea_ironmonkey_MogaController_nativeOnKeyEvent(void *env, void *obj, int keycode, int action) {
    (void)env; (void)obj; (void)keycode; (void)action;
}

void Java_com_ea_ironmonkey_MogaController_nativeOnMotionEvent(void *env, void *obj, int axis, float val) {
    (void)env; (void)obj; (void)axis; (void)val;
}

void Java_com_ea_ironmonkey_MogaController_nativeOnStateEvent(void *env, void *obj, int state, int val) {
    (void)env; (void)obj; (void)state; (void)val;
}

int custom_cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    (void)func; (void)arg; (void)dso_handle;
    return 0;
}

uintptr_t _ZTVN10__cxxabiv117__class_type_infoE[16] = {0};
uintptr_t _ZTVN10__cxxabiv119__pointer_type_infoE[16] = {0};
uintptr_t _ZTVN10__cxxabiv120__function_type_infoE[16] = {0};
uintptr_t _ZTVN10__cxxabiv120__si_class_type_infoE[16] = {0};

uintptr_t _ZNSt6__ndk14cerrE[16] = {0};
uintptr_t _ZNSt6__ndk15ctypeIcE2idE[16] = {0};
uintptr_t _ZTISt9exception[16] = {0};

void _ZNSt9exceptionD2Ev(void *this_ptr) {
    (void)this_ptr;
}

// ============================================================================
// HELPER ZUM GELATENEN VON BIBLIOTHEKEN AUS DEM LIBS-ORDNER
// ============================================================================

void *try_load_so(const char *libname) {
    char path[512];
    void *handle = NULL;

    // 1. Suche in libs/
    snprintf(path, sizeof(path), "libs/%s", libname);
    handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
        printf("[NFS Loader] Erfolgreich geladen: %s\n", path);
        return handle;
    }

    // 2. Suche im Hauptverzeichnis
    snprintf(path, sizeof(path), "./%s", libname);
    handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
        printf("[NFS Loader] Erfolgreich geladen: %s\n", path);
        return handle;
    }

    // 3. Suche in System-Pfaden
    handle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
        printf("[NFS Loader] Erfolgreich geladen: %s\n", libname);
        return handle;
    }

    printf("[NFS Loader] Hinweis: '%s' nicht gefunden/geladen (%s)\n", libname, dlerror());
    return NULL;
}

// ============================================================================
// MAIN LOADER INITIALISIERUNG
// ============================================================================

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("[NFS Loader] Starte NFS Most Wanted Port Loader...\n");

    char cwd[256];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("[NFS Loader] Arbeitsverzeichnis: %s\n", cwd);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) < 0) {
        printf("[NFS Loader] SDL_Init Fehler: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Need for Speed Most Wanted",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
    );

    if (!window) {
        printf("[NFS Loader] Fenster konnte nicht erstellt werden: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("[NFS Loader] SDL_GL Context Hinweis: %s\n", SDL_GetError());
    }

    // 1. Pre-loading aller C++ und Support-Bibliotheken (RTLD_GLOBAL stellt Symbole bereit)
    try_load_so("libc++_shared.so");
    try_load_so("libgnustl_shared.so");
    try_load_so("liblog.so");
    try_load_so("libandroid.so");
    try_load_so("libjnigraphics.so");
    try_load_so("libfmodex.so");
    try_load_so("libfmodevent.so");
    try_load_so("libNimble.so");

    // 2. Laden der Hauptbibliothek
    void *so_handle = try_load_so("libNFSMW.so");
    if (!so_handle) {
        printf("[NFS Loader] KRITISCHER FEHLER: libNFSMW.so konnte nicht geladen werden!\n");
        printf("[NFS Loader] Stelle sicher, dass libNFSMW.so im Ordner 'libs/' liegt.\n");
        if (gl_context) SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("[NFS Loader] libNFSMW.so erfolgreich geladen.\n");

    // 3. Ausführung von JNI_OnLoad mit g_java_vm (stürzt nicht mehr ab)
    jint (*JNI_OnLoad)(void *vm, void *reserved) = (jint (*)(void *, void *))dlsym(so_handle, "JNI_OnLoad");
    if (JNI_OnLoad) {
        printf("[NFS Loader] Führe JNI_OnLoad aus...\n");
        jint res = JNI_OnLoad(g_java_vm, NULL);
        printf("[NFS Loader] JNI_OnLoad Ergebnis: %d\n", res);
    }

    // Haupt-Eventloop
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        SDL_GL_SwapWindow(window);
        usleep(16000); // ~60 FPS
    }

    dlclose(so_handle);
    if (gl_context) SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
