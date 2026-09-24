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
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

/* ==========================================================================
 * 1. BIONIC / ANDROID SYSTEM SHIMS
 * ========================================================================== */

/* Stack canary – von Android-NDK-Binaries erwartet */
uintptr_t __stack_chk_guard = 0xd00a5300;

void __stack_chk_fail(void) {
    fprintf(stderr, "[NFS Loader] KRITISCH: Stack Corruption Detected!\n");
    exit(1);
}

/* __errno als Funktion exportieren (Bionic-Konvention) */
int *__errno(void) {
    return &errno;
}

/* POSIX-C-Shims, deren Symbole in Android-NDK-Bibliotheken anders heißen */
long long strtoll_shim(const char *nptr, char **endptr, int base) {
    return strtoll(nptr, endptr, base);
}
unsigned long long strtoull_shim(const char *nptr, char **endptr, int base) {
    return strtoull(nptr, endptr, base);
}

/* ==========================================================================
 * 2. JNI TYPEN (AOSP jni.h kompatibel)
 * ========================================================================== */

typedef int32_t  jint;
typedef int64_t  jlong;
typedef int8_t   jbyte;
typedef uint16_t jchar;
typedef int16_t  jshort;
typedef float    jfloat;
typedef double   jdouble;
typedef uint8_t  jboolean;

typedef void* jobject;
typedef void* jclass;
typedef void* jstring;
typedef void* jarray;
typedef void* jobjectArray;
typedef void* jbooleanArray;
typedef void* jbyteArray;
typedef void* jcharArray;
typedef void* jshortArray;
typedef void* jintArray;
typedef void* jlongArray;
typedef void* jfloatArray;
typedef void* jdoubleArray;
typedef void* jthrowable;
typedef void* jweak;
typedef void* jmethodID;
typedef void* jfieldID;

typedef union {
    jboolean z; jbyte b; jchar c; jshort s; jint i; jlong j;
    jfloat f; jdouble d; jobject l;
} jvalue;

typedef enum {
    JNIInvalidRefType = 0,
    JNILocalRefType   = 1,
    JNIGlobalRefType  = 2,
    JNIWeakGlobalRefType = 3
} jobjectRefType;

#define JNI_OK           0
#define JNI_ERR         (-1)
#define JNI_EDETACHED   (-2)
#define JNI_EVERSION    (-3)
#define JNI_COMMIT       1
#define JNI_ABORT        2
#define JNI_FALSE        0
#define JNI_TRUE         1
#define JNI_VERSION_1_6  0x00010006

struct JNINativeInterface_struct;
struct JNIInvokeInterface_struct;

typedef const struct JNINativeInterface_struct *JNIEnv;
typedef const struct JNIInvokeInterface_struct *JavaVM;

/* ==========================================================================
 * 3. JNI NATIVE INTERFACE – STUBS
 * ========================================================================== */

#define A(...) __VA_ARGS__

static jint       NJI_GetVersion(JNIEnv *e){ (void)e; return JNI_VERSION_1_6; }
static jclass     NJI_DefineClass(JNIEnv *e, const char *n, jobject l, const jbyte *b, jsize_t d){ (void)e;(void)n;(void)l;(void)b;(void)d; return (jclass)1; }
static jclass     NJI_FindClass(JNIEnv *e, const char *n){ (void)e;(void)n; return (jclass)1; }
static jmethodID  NJI_FromReflectedMethod(JNIEnv *e, jobject m){ (void)e;(void)m; return (jmethodID)1; }
static jfieldID   NJI_FromReflectedField(JNIEnv *e, jobject f){ (void)e;(void)f; return (jfieldID)1; }
static jobject    NJI_ToReflectedMethod(JNIEnv *e, jclass c, jmethodID m, jboolean s){ (void)e;(void)c;(void)m;(void)s; return (jobject)1; }
static jclass     NJI_GetSuperclass(JNIEnv *e, jclass c){ (void)e;(void)c; return (jclass)1; }
static jboolean   NJI_IsAssignableFrom(JNIEnv *e, jclass c1, jclass c2){ (void)e;(void)c1;(void)c2; return JNI_TRUE; }
static jobject    NJI_ToReflectedField(JNIEnv *e, jclass c, jfieldID f, jboolean s){ (void)e;(void)c;(void)f;(void)s; return (jobject)1; }
static jint       NJI_Throw(JNIEnv *e, jthrowable t){ (void)e;(void)t; return 0; }
static jint       NJI_ThrowNew(JNIEnv *e, jclass c, const char *m){ (void)e;(void)c;(void)m; return 0; }
static jthrowable NJI_ExceptionOccurred(JNIEnv *e){ (void)e; return NULL; }
static void       NJI_ExceptionDescribe(JNIEnv *e){ (void)e; }
static void       NJI_ExceptionClear(JNIEnv *e){ (void)e; }
static void       NJI_FatalError(JNIEnv *e, const char *m){ (void)e; fprintf(stderr,"[JNI Fatal] %s\n", m?m:""); exit(1); }
static jint       NJI_PushLocalFrame(JNIEnv *e, jint c){ (void)e;(void)c; return 0; }
static jobject    NJI_PopLocalFrame(JNIEnv *e, jobject r){ (void)e; return r; }
static jobject    NJI_NewGlobalRef(JNIEnv *e, jobject o){ (void)e; return o; }
static void       NJI_DeleteGlobalRef(JNIEnv *e, jobject o){ (void)e;(void)o; }
static void       NJI_DeleteLocalRef(JNIEnv *e, jobject o){ (void)e;(void)o; }
static jboolean   NJI_IsSameObject(JNIEnv *e, jobject a, jobject b){ (void)e; return (a==b)?JNI_TRUE:JNI_FALSE; }
static jobject    NJI_NewLocalRef(JNIEnv *e, jobject o){ (void)e; return o; }
static jint       NJI_EnsureLocalCapacity(JNIEnv *e, jint c){ (void)e;(void)c; return 0; }
static jobject    NJI_AllocObject(JNIEnv *e, jclass c){ (void)e;(void)c; return (jobject)1; }
static jobject    NJI_NewObject(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return (jobject)1; }
static jobject    NJI_NewObjectV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return (jobject)1; }
static jobject    NJI_NewObjectA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return (jobject)1; }
static jclass     NJI_GetObjectClass(JNIEnv *e, jobject o){ (void)e;(void)o; return (jclass)1; }
static jboolean   NJI_IsInstanceOf(JNIEnv *e, jobject o, jclass c){ (void)e;(void)o;(void)c; return JNI_TRUE; }
static jmethodID  NJI_GetMethodID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jmethodID)1; }

#define CALL_OBJ(name) static jobject name(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return NULL; }
#define CALL_OBJV(name) static jobject name##V(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return NULL; }
#define CALL_OBJA(name) static jobject name##A(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return NULL; }
CALL_OBJ(NJI_CallObjectMethod)
CALL_OBJV(NJI_CallObjectMethod)
CALL_OBJA(NJI_CallObjectMethod)

static jboolean NJI_CallBooleanMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return JNI_FALSE; }
static jboolean NJI_CallBooleanMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return JNI_FALSE; }
static jboolean NJI_CallBooleanMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return JNI_FALSE; }
static jbyte    NJI_CallByteMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jbyte    NJI_CallByteMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jbyte    NJI_CallByteMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jchar    NJI_CallCharMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jchar    NJI_CallCharMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jchar    NJI_CallCharMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jshort   NJI_CallShortMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jshort   NJI_CallShortMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jshort   NJI_CallShortMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jint     NJI_CallIntMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jint     NJI_CallIntMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jint     NJI_CallIntMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jlong    NJI_CallLongMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jlong    NJI_CallLongMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jlong    NJI_CallLongMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jfloat   NJI_CallFloatMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0.f; }
static jfloat   NJI_CallFloatMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0.f; }
static jfloat   NJI_CallFloatMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0.f; }
static jdouble  NJI_CallDoubleMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0.0; }
static jdouble  NJI_CallDoubleMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0.0; }
static jdouble  NJI_CallDoubleMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0.0; }
static void     NJI_CallVoidMethod(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; }
static void     NJI_CallVoidMethodV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; }
static void     NJI_CallVoidMethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; }

static jobject  NJI_CallNonvirtualObjectMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return NULL; }
static jobject  NJI_CallNonvirtualObjectMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return NULL; }
static jobject  NJI_CallNonvirtualObjectMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return NULL; }
static jboolean NJI_CallNonvirtualBooleanMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return JNI_FALSE; }
static jboolean NJI_CallNonvirtualBooleanMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return JNI_FALSE; }
static jboolean NJI_CallNonvirtualBooleanMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return JNI_FALSE; }
static jbyte    NJI_CallNonvirtualByteMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jbyte    NJI_CallNonvirtualByteMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jbyte    NJI_CallNonvirtualByteMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jchar    NJI_CallNonvirtualCharMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jchar    NJI_CallNonvirtualCharMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jchar    NJI_CallNonvirtualCharMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jshort   NJI_CallNonvirtualShortMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jshort   NJI_CallNonvirtualShortMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jshort   NJI_CallNonvirtualShortMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jint     NJI_CallNonvirtualIntMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jint     NJI_CallNonvirtualIntMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jint     NJI_CallNonvirtualIntMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jlong    NJI_CallNonvirtualLongMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jlong    NJI_CallNonvirtualLongMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jlong    NJI_CallNonvirtualLongMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jfloat   NJI_CallNonvirtualFloatMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0.f; }
static jfloat   NJI_CallNonvirtualFloatMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.f; }
static jfloat   NJI_CallNonvirtualFloatMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.f; }
static jdouble  NJI_CallNonvirtualDoubleMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0.0; }
static jdouble  NJI_CallNonvirtualDoubleMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.0; }
static jdouble  NJI_CallNonvirtualDoubleMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.0; }
static void     NJI_CallNonvirtualVoidMethod(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; }
static void     NJI_CallNonvirtualVoidMethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; }
static void     NJI_CallNonvirtualVoidMethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; }

static jfieldID NJI_GetFieldID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jfieldID)1; }
static jobject  NJI_GetObjectField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return NULL; }
static jboolean NJI_GetBooleanField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return JNI_FALSE; }
static jbyte    NJI_GetByteField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jchar    NJI_GetCharField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jshort   NJI_GetShortField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jint     NJI_GetIntField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jlong    NJI_GetLongField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jfloat   NJI_GetFloatField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0.f; }
static jdouble  NJI_GetDoubleField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0.0; }
static void     NJI_SetObjectField(JNIEnv *e, jobject o, jfieldID f, jobject v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetBooleanField(JNIEnv *e, jobject o, jfieldID f, jboolean v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetByteField(JNIEnv *e, jobject o, jfieldID f, jbyte v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetCharField(JNIEnv *e, jobject o, jfieldID f, jchar v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetShortField(JNIEnv *e, jobject o, jfieldID f, jshort v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetIntField(JNIEnv *e, jobject o, jfieldID f, jint v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetLongField(JNIEnv *e, jobject o, jfieldID f, jlong v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetFloatField(JNIEnv *e, jobject o, jfieldID f, jfloat v){ (void)e;(void)o;(void)f;(void)v; }
static void     NJI_SetDoubleField(JNIEnv *e, jobject o, jfieldID f, jdouble v){ (void)e;(void)o;(void)f;(void)v; }

static jmethodID NJI_GetStaticMethodID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jmethodID)1; }
static jobject   NJI_CallStaticObjectMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return NULL; }
static jobject   NJI_CallStaticObjectMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return NULL; }
static jobject   NJI_CallStaticObjectMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return NULL; }
static jboolean  NJI_CallStaticBooleanMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return JNI_FALSE; }
static jboolean  NJI_CallStaticBooleanMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return JNI_FALSE; }
static jboolean  NJI_CallStaticBooleanMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return JNI_FALSE; }
static jbyte     NJI_CallStaticByteMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jbyte     NJI_CallStaticByteMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jbyte     NJI_CallStaticByteMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jchar     NJI_CallStaticCharMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jchar     NJI_CallStaticCharMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jchar     NJI_CallStaticCharMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jshort    NJI_CallStaticShortMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jshort    NJI_CallStaticShortMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jshort    NJI_CallStaticShortMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jint      NJI_CallStaticIntMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jint      NJI_CallStaticIntMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jint      NJI_CallStaticIntMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jlong     NJI_CallStaticLongMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jlong     NJI_CallStaticLongMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jlong     NJI_CallStaticLongMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jfloat    NJI_CallStaticFloatMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0.f; }
static jfloat    NJI_CallStaticFloatMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0.f; }
static jfloat    NJI_CallStaticFloatMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0.f; }
static jdouble   NJI_CallStaticDoubleMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0.0; }
static jdouble   NJI_CallStaticDoubleMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0.0; }
static jdouble   NJI_CallStaticDoubleMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0.0; }
static void      NJI_CallStaticVoidMethod(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; }
static void      NJI_CallStaticVoidMethodV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; }
static void      NJI_CallStaticVoidMethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; }

static jfieldID  NJI_GetStaticFieldID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jfieldID)1; }
static jobject   NJI_GetStaticObjectField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return NULL; }
static jboolean  NJI_GetStaticBooleanField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return JNI_FALSE; }
static jbyte     NJI_GetStaticByteField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jchar     NJI_GetStaticCharField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jshort    NJI_GetStaticShortField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jint      NJI_GetStaticIntField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jlong     NJI_GetStaticLongField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jfloat    NJI_GetStaticFloatField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0.f; }
static jdouble   NJI_GetStaticDoubleField(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0.0; }
static void      NJI_SetStaticObjectField(JNIEnv *e, jclass c, jfieldID f, jobject v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticBooleanField(JNIEnv *e, jclass c, jfieldID f, jboolean v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticByteField(JNIEnv *e, jclass c, jfieldID f, jbyte v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticCharField(JNIEnv *e, jclass c, jfieldID f, jchar v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticShortField(JNIEnv *e, jclass c, jfieldID f, jshort v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticIntField(JNIEnv *e, jclass c, jfieldID f, jint v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticLongField(JNIEnv *e, jclass c, jfieldID f, jlong v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticFloatField(JNIEnv *e, jclass c, jfieldID f, jfloat v){ (void)e;(void)c;(void)f;(void)v; }
static void      NJI_SetStaticDoubleField(JNIEnv *e, jclass c, jfieldID f, jdouble v){ (void)e;(void)c;(void)f;(void)v; }

static jstring NJI_NewString(JNIEnv *e, const jchar *u, jsize_t l){ (void)e;(void)u;(void)l; return (jstring)1; }
static jsize_t NJI_GetStringLength(JNIEnv *e, jstring s){ (void)e;(void)s; return 0; }
static const jchar *NJI_GetStringChars(JNIEnv *e, jstring s, jboolean *c){ (void)e;(void)s; if(c)*c=JNI_FALSE; return (const jchar*)L""; }
static void    NJI_ReleaseStringChars(JNIEnv *e, jstring s, const jchar *c){ (void)e;(void)s;(void)c; }
static jstring NJI_NewStringUTF(JNIEnv *e, const char *b){ (void)e;(void)b; return (jstring)1; }
static jsize_t NJI_GetStringUTFLength(JNIEnv *e, jstring s){ (void)e;(void)s; return 0; }
static const char *NJI_GetStringUTFChars(JNIEnv *e, jstring s, jboolean *c){ (void)e;(void)s; if(c)*c=JNI_FALSE; return ""; }
static void    NJI_ReleaseStringUTFChars(JNIEnv *e, jstring s, const char *u){ (void)e;(void)s;(void)u; }

static jsize_t NJI_GetArrayLength(JNIEnv *e, jarray a){ (void)e;(void)a; return 0; }
static jobjectArray NJI_NewObjectArray(JNIEnv *e, jsize_t l, jclass c, jobject i){ (void)e;(void)l;(void)c;(void)i; return (jobjectArray)1; }
static jobject NJI_GetObjectArrayElement(JNIEnv *e, jobjectArray a, jsize_t i){ (void)e;(void)a;(void)i; return NULL; }
static void    NJI_SetObjectArrayElement(JNIEnv *e, jobjectArray a, jsize_t i, jobject v){ (void)e;(void)a;(void)i;(void)v; }

#define NEW_ARRAY(ret, name, elem) static ret name(JNIEnv *e, jsize_t l){ (void)e;(void)l; return (ret)1; }
NEW_ARRAY(jbooleanArray, NJI_NewBooleanArray, jboolean)
NEW_ARRAY(jbyteArray,    NJI_NewByteArray,    jbyte)
NEW_ARRAY(jcharArray,    NJI_NewCharArray,    jchar)
NEW_ARRAY(jshortArray,   NJI_NewShortArray,   jshort)
NEW_ARRAY(jintArray,     NJI_NewIntArray,     jint)
NEW_ARRAY(jlongArray,    NJI_NewLongArray,    jlong)
NEW_ARRAY(jfloatArray,   NJI_NewFloatArray,   jfloat)
NEW_ARRAY(jdoubleArray,  NJI_NewDoubleArray,  jdouble)

#define GET_ARRAY(ret, name, elem) static ret *name(JNIEnv *e, ret a, jboolean *c){ (void)e;(void)a; if(c)*c=JNI_FALSE; return NULL; }
GET_ARRAY(jboolean, NJI_GetBooleanArrayElements, jboolean)
GET_ARRAY(jbyte,    NJI_GetByteArrayElements,    jbyte)
GET_ARRAY(jchar,    NJI_GetCharArrayElements,    jchar)
GET_ARRAY(jshort,   NJI_GetShortArrayElements,   jshort)
GET_ARRAY(jint,     NJI_GetIntArrayElements,     jint)
GET_ARRAY(jlong,    NJI_GetLongArrayElements,    jlong)
GET_ARRAY(jfloat,   NJI_GetFloatArrayElements,   jfloat)
GET_ARRAY(jdouble,  NJI_GetDoubleArrayElements,  jdouble)

#define REL_ARRAY(ret, name, elem) static void name(JNIEnv *e, ret a, elem *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
REL_ARRAY(jbooleanArray, NJI_ReleaseBooleanArrayElements, jboolean)
REL_ARRAY(jbyteArray,    NJI_ReleaseByteArrayElements,    jbyte)
REL_ARRAY(jcharArray,    NJI_ReleaseCharArrayElements,    jchar)
REL_ARRAY(jshortArray,   NJI_ReleaseShortArrayElements,   jshort)
REL_ARRAY(jintArray,     NJI_ReleaseIntArrayElements,     jint)
REL_ARRAY(jlongArray,    NJI_ReleaseLongArrayElements,    jlong)
REL_ARRAY(jfloatArray,   NJI_ReleaseFloatArrayElements,   jfloat)
REL_ARRAY(jdoubleArray,  NJI_ReleaseDoubleArrayElements,  jdouble)

#define GETREG(name, type) static void name(JNIEnv *e, type a, jsize_t st, jsize_t l, type##_t *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
/* Vereinfachte Regions (mit jboolean_t etc. entfällt – wir nutzen die Standardtypen) */
static void NJI_GetBooleanArrayRegion(JNIEnv *e, jbooleanArray a, jsize_t st, jsize_t l, jboolean *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetByteArrayRegion   (JNIEnv *e, jbyteArray    a, jsize_t st, jsize_t l, jbyte    *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetCharArrayRegion   (JNIEnv *e, jcharArray    a, jsize_t st, jsize_t l, jchar    *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetShortArrayRegion  (JNIEnv *e, jshortArray   a, jsize_t st, jsize_t l, jshort   *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetIntArrayRegion    (JNIEnv *e, jintArray     a, jsize_t st, jsize_t l, jint     *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetLongArrayRegion   (JNIEnv *e, jlongArray    a, jsize_t st, jsize_t l, jlong    *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetFloatArrayRegion  (JNIEnv *e, jfloatArray   a, jsize_t st, jsize_t l, jfloat   *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_GetDoubleArrayRegion (JNIEnv *e, jdoubleArray  a, jsize_t st, jsize_t l, jdouble  *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }

static void NJI_SetBooleanArrayRegion(JNIEnv *e, jbooleanArray a, jsize_t st, jsize_t l, const jboolean *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetByteArrayRegion   (JNIEnv *e, jbyteArray    a, jsize_t st, jsize_t l, const jbyte    *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetCharArrayRegion   (JNIEnv *e, jcharArray    a, jsize_t st, jsize_t l, const jchar    *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetShortArrayRegion  (JNIEnv *e, jshortArray   a, jsize_t st, jsize_t l, const jshort   *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetIntArrayRegion    (JNIEnv *e, jintArray     a, jsize_t st, jsize_t l, const jint     *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetLongArrayRegion   (JNIEnv *e, jlongArray    a, jsize_t st, jsize_t l, const jlong    *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetFloatArrayRegion  (JNIEnv *e, jfloatArray   a, jsize_t st, jsize_t l, const jfloat   *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }
static void NJI_SetDoubleArrayRegion (JNIEnv *e, jdoubleArray  a, jsize_t st, jsize_t l, const jdouble  *b){ (void)e;(void)a;(void)st;(void)l;(void)b; }

static jint NJI_RegisterNatives(JNIEnv *e, jclass c, const void *m, jint n){ (void)e;(void)c;(void)m;(void)n; return 0; }
static jint NJI_UnregisterNatives(JNIEnv *e, jclass c){ (void)e;(void)c; return 0; }
static jint NJI_MonitorEnter(JNIEnv *e, jobject o){ (void)e;(void)o; return 0; }
static jint NJI_MonitorExit(JNIEnv *e, jobject o){ (void)e;(void)o; return 0; }
static jint NJI_GetJavaVM(JNIEnv *e, JavaVM **vm){ (void)e; if(vm)*vm=NULL; return JNI_OK; }
static void NJI_GetStringRegion(JNIEnv *e, jstring s, jsize_t st, jsize_t l, jchar *b){ (void)e;(void)s;(void)st;(void)l;(void)b; }
static void NJI_GetStringUTFRegion(JNIEnv *e, jstring s, jsize_t st, jsize_t l, char *b){ (void)e;(void)s;(void)st;(void)l;(void)b; }
static void *NJI_GetPrimitiveArrayCritical(JNIEnv *e, jarray a, jboolean *c){ (void)e;(void)a; if(c)*c=JNI_FALSE; return NULL; }
static void  NJI_ReleasePrimitiveArrayCritical(JNIEnv *e, jarray a, void *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static const jchar *NJI_GetStringCritical(JNIEnv *e, jstring s, jboolean *c){ (void)e;(void)s; if(c)*c=JNI_FALSE; return (const jchar*)L""; }
static void  NJI_ReleaseStringCritical(JNIEnv *e, jstring s, const jchar *c){ (void)e;(void)s;(void)c; }
static jweak NJI_NewWeakGlobalRef(JNIEnv *e, jobject o){ (void)e; return (jweak)o; }
static void  NJI_DeleteWeakGlobalRef(JNIEnv *e, jweak w){ (void)e;(void)w; }
static jboolean NJI_ExceptionCheck(JNIEnv *e){ (void)e; return JNI_FALSE; }
static jobject NJI_NewDirectByteBuffer(JNIEnv *e, void *addr, jlong cap){ (void)e;(void)addr;(void)cap; return (jobject)1; }
static void   *NJI_GetDirectBufferAddress(JNIEnv *e, jobject b){ (void)e;(void)b; return NULL; }
static jlong   NJI_GetDirectBufferCapacity(JNIEnv *e, jobject b){ (void)e;(void)b; return 0; }
static jobjectRefType NJI_GetObjectRefType(JNIEnv *e, jobject o){ (void)e;(void)o; return JNIInvalidRefType; }

/* ==========================================================================
 * 4. JNI NATIVE INTERFACE – VTABLE
 * ========================================================================== */

struct JNINativeInterface_struct {
    void *reserved0, *reserved1, *reserved2, *reserved3;
    jint (*GetVersion)(JNIEnv *);
    jclass (*DefineClass)(JNIEnv *, const char *, jobject, const jbyte *, jsize_t);
    jclass (*FindClass)(JNIEnv *, const char *);
    jmethodID (*FromReflectedMethod)(JNIEnv *, jobject);
    jfieldID (*FromReflectedField)(JNIEnv *, jobject);
    jobject (*ToReflectedMethod)(JNIEnv *, jclass, jmethodID, jboolean);
    jclass (*GetSuperclass)(JNIEnv *, jclass);
    jboolean (*IsAssignableFrom)(JNIEnv *, jclass, jclass);
    jobject (*ToReflectedField)(JNIEnv *, jclass, jfieldID, jboolean);
    jint (*Throw)(JNIEnv *, jthrowable);
    jint (*ThrowNew)(JNIEnv *, jclass, const char *);
    jthrowable (*ExceptionOccurred)(JNIEnv *);
    void (*ExceptionDescribe)(JNIEnv *);
    void (*ExceptionClear)(JNIEnv *);
    void (*FatalError)(JNIEnv *, const char *);
    jint (*PushLocalFrame)(JNIEnv *, jint);
    jobject (*PopLocalFrame)(JNIEnv *, jobject);
    jobject (*NewGlobalRef)(JNIEnv *, jobject);
    void (*DeleteGlobalRef)(JNIEnv *, jobject);
    void (*DeleteLocalRef)(JNIEnv *, jobject);
    jboolean (*IsSameObject)(JNIEnv *, jobject, jobject);
    jobject (*NewLocalRef)(JNIEnv *, jobject);
    jint (*EnsureLocalCapacity)(JNIEnv *, jint);
    jobject (*AllocObject)(JNIEnv *, jclass);
    jobject (*NewObject)(JNIEnv *, jclass, jmethodID, ...);
    jobject (*NewObjectV)(JNIEnv *, jclass, jmethodID, va_list);
    jobject (*NewObjectA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jclass (*GetObjectClass)(JNIEnv *, jobject);
    jboolean (*IsInstanceOf)(JNIEnv *, jobject, jclass);
    jmethodID (*GetMethodID)(JNIEnv *, jclass, const char *, const char *);
    jobject (*CallObjectMethod)(JNIEnv *, jobject, jmethodID, ...);
    jobject (*CallObjectMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jobject (*CallObjectMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jboolean (*CallBooleanMethod)(JNIEnv *, jobject, jmethodID, ...);
    jboolean (*CallBooleanMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jboolean (*CallBooleanMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jbyte (*CallByteMethod)(JNIEnv *, jobject, jmethodID, ...);
    jbyte (*CallByteMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jbyte (*CallByteMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jchar (*CallCharMethod)(JNIEnv *, jobject, jmethodID, ...);
    jchar (*CallCharMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jchar (*CallCharMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jshort (*CallShortMethod)(JNIEnv *, jobject, jmethodID, ...);
    jshort (*CallShortMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jshort (*CallShortMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jint (*CallIntMethod)(JNIEnv *, jobject, jmethodID, ...);
    jint (*CallIntMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jint (*CallIntMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jlong (*CallLongMethod)(JNIEnv *, jobject, jmethodID, ...);
    jlong (*CallLongMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jlong (*CallLongMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jfloat (*CallFloatMethod)(JNIEnv *, jobject, jmethodID, ...);
    jfloat (*CallFloatMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jfloat (*CallFloatMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jdouble (*CallDoubleMethod)(JNIEnv *, jobject, jmethodID, ...);
    jdouble (*CallDoubleMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jdouble (*CallDoubleMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    void (*CallVoidMethod)(JNIEnv *, jobject, jmethodID, ...);
    void (*CallVoidMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    void (*CallVoidMethodA)(JNIEnv *, jobject, jmethodID, jvalue *);
    jobject (*CallNonvirtualObjectMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jobject (*CallNonvirtualObjectMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jobject (*CallNonvirtualObjectMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jboolean (*CallNonvirtualBooleanMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jboolean (*CallNonvirtualBooleanMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jboolean (*CallNonvirtualBooleanMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jbyte (*CallNonvirtualByteMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jbyte (*CallNonvirtualByteMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jbyte (*CallNonvirtualByteMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jchar (*CallNonvirtualCharMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jchar (*CallNonvirtualCharMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jchar (*CallNonvirtualCharMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jshort (*CallNonvirtualShortMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jshort (*CallNonvirtualShortMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jshort (*CallNonvirtualShortMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jint (*CallNonvirtualIntMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jint (*CallNonvirtualIntMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jint (*CallNonvirtualIntMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jlong (*CallNonvirtualLongMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jlong (*CallNonvirtualLongMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jlong (*CallNonvirtualLongMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jfloat (*CallNonvirtualFloatMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jfloat (*CallNonvirtualFloatMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jfloat (*CallNonvirtualFloatMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jdouble (*CallNonvirtualDoubleMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jdouble (*CallNonvirtualDoubleMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    jdouble (*CallNonvirtualDoubleMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    void (*CallNonvirtualVoidMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    void (*CallNonvirtualVoidMethodV)(JNIEnv *, jobject, jclass, jmethodID, va_list);
    void (*CallNonvirtualVoidMethodA)(JNIEnv *, jobject, jclass, jmethodID, jvalue *);
    jfieldID (*GetFieldID)(JNIEnv *, jclass, const char *, const char *);
    jobject (*GetObjectField)(JNIEnv *, jobject, jfieldID);
    jboolean (*GetBooleanField)(JNIEnv *, jobject, jfieldID);
    jbyte (*GetByteField)(JNIEnv *, jobject, jfieldID);
    jchar (*GetCharField)(JNIEnv *, jobject, jfieldID);
    jshort (*GetShortField)(JNIEnv *, jobject, jfieldID);
    jint (*GetIntField)(JNIEnv *, jobject, jfieldID);
    jlong (*GetLongField)(JNIEnv *, jobject, jfieldID);
    jfloat (*GetFloatField)(JNIEnv *, jobject, jfieldID);
    jdouble (*GetDoubleField)(JNIEnv *, jobject, jfieldID);
    void (*SetObjectField)(JNIEnv *, jobject, jfieldID, jobject);
    void (*SetBooleanField)(JNIEnv *, jobject, jfieldID, jboolean);
    void (*SetByteField)(JNIEnv *, jobject, jfieldID, jbyte);
    void (*SetCharField)(JNIEnv *, jobject, jfieldID, jchar);
    void (*SetShortField)(JNIEnv *, jobject, jfieldID, jshort);
    void (*SetIntField)(JNIEnv *, jobject, jfieldID, jint);
    void (*SetLongField)(JNIEnv *, jobject, jfieldID, jlong);
    void (*SetFloatField)(JNIEnv *, jobject, jfieldID, jfloat);
    void (*SetDoubleField)(JNIEnv *, jobject, jfieldID, jdouble);
    jmethodID (*GetStaticMethodID)(JNIEnv *, jclass, const char *, const char *);
    jobject (*CallStaticObjectMethod)(JNIEnv *, jclass, jmethodID, ...);
    jobject (*CallStaticObjectMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jobject (*CallStaticObjectMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jboolean (*CallStaticBooleanMethod)(JNIEnv *, jclass, jmethodID, ...);
    jboolean (*CallStaticBooleanMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jboolean (*CallStaticBooleanMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jbyte (*CallStaticByteMethod)(JNIEnv *, jclass, jmethodID, ...);
    jbyte (*CallStaticByteMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jbyte (*CallStaticByteMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jchar (*CallStaticCharMethod)(JNIEnv *, jclass, jmethodID, ...);
    jchar (*CallStaticCharMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jchar (*CallStaticCharMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jshort (*CallStaticShortMethod)(JNIEnv *, jclass, jmethodID, ...);
    jshort (*CallStaticShortMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jshort (*CallStaticShortMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jint (*CallStaticIntMethod)(JNIEnv *, jclass, jmethodID, ...);
    jint (*CallStaticIntMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jint (*CallStaticIntMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jlong (*CallStaticLongMethod)(JNIEnv *, jclass, jmethodID, ...);
    jlong (*CallStaticLongMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jlong (*CallStaticLongMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jfloat (*CallStaticFloatMethod)(JNIEnv *, jclass, jmethodID, ...);
    jfloat (*CallStaticFloatMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jfloat (*CallStaticFloatMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jdouble (*CallStaticDoubleMethod)(JNIEnv *, jclass, jmethodID, ...);
    jdouble (*CallStaticDoubleMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jdouble (*CallStaticDoubleMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    void (*CallStaticVoidMethod)(JNIEnv *, jclass, jmethodID, ...);
    void (*CallStaticVoidMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    void (*CallStaticVoidMethodA)(JNIEnv *, jclass, jmethodID, jvalue *);
    jfieldID (*GetStaticFieldID)(JNIEnv *, jclass, const char *, const char *);
    jobject (*GetStaticObjectField)(JNIEnv *, jclass, jfieldID);
    jboolean (*GetStaticBooleanField)(JNIEnv *, jclass, jfieldID);
    jbyte (*GetStaticByteField)(JNIEnv *, jclass, jfieldID);
    jchar (*GetStaticCharField)(JNIEnv *, jclass, jfieldID);
    jshort (*GetStaticShortField)(JNIEnv *, jclass, jfieldID);
    jint (*GetStaticIntField)(JNIEnv *, jclass, jfieldID);
    jlong (*GetStaticLongField)(JNIEnv *, jclass, jfieldID);
    jfloat (*GetStaticFloatField)(JNIEnv *, jclass, jfieldID);
    jdouble (*GetStaticDoubleField)(JNIEnv *, jclass, jfieldID);
    void (*SetStaticObjectField)(JNIEnv *, jclass, jfieldID, jobject);
    void (*SetStaticBooleanField)(JNIEnv *, jclass, jfieldID, jboolean);
    void (*SetStaticByteField)(JNIEnv *, jclass, jfieldID, jbyte);
    void (*SetStaticCharField)(JNIEnv *, jclass, jfieldID, jchar);
    void (*SetStaticShortField)(JNIEnv *, jclass, jfieldID, jshort);
    void (*SetStaticIntField)(JNIEnv *, jclass, jfieldID, jint);
    void (*SetStaticLongField)(JNIEnv *, jclass, jfieldID, jlong);
    void (*SetStaticFloatField)(JNIEnv *, jclass, jfieldID, jfloat);
    void (*SetStaticDoubleField)(JNIEnv *, jclass, jfieldID, jdouble);
    jstring (*NewString)(JNIEnv *, const jchar *, jsize_t);
    jsize_t (*GetStringLength)(JNIEnv *, jstring);
    const jchar *(*GetStringChars)(JNIEnv *, jstring, jboolean *);
    void (*ReleaseStringChars)(JNIEnv *, jstring, const jchar *);
    jstring (*NewStringUTF)(JNIEnv *, const char *);
    jsize_t (*GetStringUTFLength)(JNIEnv *, jstring);
    const char *(*GetStringUTFChars)(JNIEnv *, jstring, jboolean *);
    void (*ReleaseStringUTFChars)(JNIEnv *, jstring, const char *);
    jsize_t (*GetArrayLength)(JNIEnv *, jarray);
    jobjectArray (*NewObjectArray)(JNIEnv *, jsize_t, jclass, jobject);
    jobject (*GetObjectArrayElement)(JNIEnv *, jobjectArray, jsize_t);
    void (*SetObjectArrayElement)(JNIEnv *, jobjectArray, jsize_t, jobject);
    jbooleanArray (*NewBooleanArray)(JNIEnv *, jsize_t);
    jbyteArray (*NewByteArray)(JNIEnv *, jsize_t);
    jcharArray (*NewCharArray)(JNIEnv *, jsize_t);
    jshortArray (*NewShortArray)(JNIEnv *, jsize_t);
    jintArray (*NewIntArray)(JNIEnv *, jsize_t);
    jlongArray (*NewLongArray)(JNIEnv *, jsize_t);
    jfloatArray (*NewFloatArray)(JNIEnv *, jsize_t);
    jdoubleArray (*NewDoubleArray)(JNIEnv *, jsize_t);
    jboolean *(*GetBooleanArrayElements)(JNIEnv *, jbooleanArray, jboolean *);
    jbyte *(*GetByteArrayElements)(JNIEnv *, jbyteArray, jboolean *);
    jchar *(*GetCharArrayElements)(JNIEnv *, jcharArray, jboolean *);
    jshort *(*GetShortArrayElements)(JNIEnv *, jshortArray, jboolean *);
    jint *(*GetIntArrayElements)(JNIEnv *, jintArray, jboolean *);
    jlong *(*GetLongArrayElements)(JNIEnv *, jlongArray, jboolean *);
    jfloat *(*GetFloatArrayElements)(JNIEnv *, jfloatArray, jboolean *);
    jdouble *(*GetDoubleArrayElements)(JNIEnv *, jdoubleArray, jboolean *);
    void (*ReleaseBooleanArrayElements)(JNIEnv *, jbooleanArray, jboolean *, jint);
    void (*ReleaseByteArrayElements)(JNIEnv *, jbyteArray, jbyte *, jint);
    void (*ReleaseCharArrayElements)(JNIEnv *, jcharArray, jchar *, jint);
    void (*ReleaseShortArrayElements)(JNIEnv *, jshortArray, jshort *, jint);
    void (*ReleaseIntArrayElements)(JNIEnv *, jintArray, jint *, jint);
    void (*ReleaseLongArrayElements)(JNIEnv *, jlongArray, jlong *, jint);
    void (*ReleaseFloatArrayElements)(JNIEnv *, jfloatArray, jfloat *, jint);
    void (*ReleaseDoubleArrayElements)(JNIEnv *, jdoubleArray, jdouble *, jint);
    void (*GetBooleanArrayRegion)(JNIEnv *, jbooleanArray, jsize_t, jsize_t, jboolean *);
    void (*GetByteArrayRegion)(JNIEnv *, jbyteArray, jsize_t, jsize_t, jbyte *);
    void (*GetCharArrayRegion)(JNIEnv *, jcharArray, jsize_t, jsize_t, jchar *);
    void (*GetShortArrayRegion)(JNIEnv *, jshortArray, jsize_t, jsize_t, jshort *);
    void (*GetIntArrayRegion)(JNIEnv *, jintArray, jsize_t, jsize_t, jint *);
    void (*GetLongArrayRegion)(JNIEnv *, jlongArray, jsize_t, jsize_t, jlong *);
    void (*GetFloatArrayRegion)(JNIEnv *, jfloatArray, jsize_t, jsize_t, jfloat *);
    void (*GetDoubleArrayRegion)(JNIEnv *, jdoubleArray, jsize_t, jsize_t, jdouble *);
    void (*SetBooleanArrayRegion)(JNIEnv *, jbooleanArray, jsize_t, jsize_t, const jboolean *);
    void (*SetByteArrayRegion)(JNIEnv *, jbyteArray, jsize_t, jsize_t, const jbyte *);
    void (*SetCharArrayRegion)(JNIEnv *, jcharArray, jsize_t, jsize_t, const jchar *);
    void (*SetShortArrayRegion)(JNIEnv *, jshortArray, jsize_t, jsize_t, const jshort *);
    void (*SetIntArrayRegion)(JNIEnv *, jintArray, jsize_t, jsize_t, const jint *);
    void (*SetLongArrayRegion)(JNIEnv *, jlongArray, jsize_t, jsize_t, const jlong *);
    void (*SetFloatArrayRegion)(JNIEnv *, jfloatArray, jsize_t, jsize_t, const jfloat *);
    void (*SetDoubleArrayRegion)(JNIEnv *, jdoubleArray, jsize_t, jsize_t, const jdouble *);
    jint (*RegisterNatives)(JNIEnv *, jclass, const void *, jint);
    jint (*UnregisterNatives)(JNIEnv *, jclass);
    jint (*MonitorEnter)(JNIEnv *, jobject);
    jint (*MonitorExit)(JNIEnv *, jobject);
    jint (*GetJavaVM)(JNIEnv *, JavaVM **);
    void (*GetStringRegion)(JNIEnv *, jstring, jsize_t, jsize_t, jchar *);
    void (*GetStringUTFRegion)(JNIEnv *, jstring, jsize_t, jsize_t, char *);
    void *(*GetPrimitiveArrayCritical)(JNIEnv *, jarray, jboolean *);
    void (*ReleasePrimitiveArrayCritical)(JNIEnv *, jarray, void *, jint);
    const jchar *(*GetStringCritical)(JNIEnv *, jstring, jboolean *);
    void (*ReleaseStringCritical)(JNIEnv *, jstring, const jchar *);
    jweak (*NewWeakGlobalRef)(JNIEnv *, jobject);
    void (*DeleteWeakGlobalRef)(JNIEnv *, jweak);
    jboolean (*ExceptionCheck)(JNIEnv *);
    jobject (*NewDirectByteBuffer)(JNIEnv *, void *, jlong);
    void *(*GetDirectBufferAddress)(JNIEnv *, jobject);
    jlong (*GetDirectBufferCapacity)(JNIEnv *, jobject);
    jobjectRefType (*GetObjectRefType)(JNIEnv *, jobject);
};

static const struct JNINativeInterface_struct g_jni_env_vtbl = {
    .GetVersion = NJI_GetVersion,
    .DefineClass = NJI_DefineClass,
    .FindClass = NJI_FindClass,
    .FromReflectedMethod = NJI_FromReflectedMethod,
    .FromReflectedField = NJI_FromReflectedField,
    .ToReflectedMethod = NJI_ToReflectedMethod,
    .GetSuperclass = NJI_GetSuperclass,
    .IsAssignableFrom = NJI_IsAssignableFrom,
    .ToReflectedField = NJI_ToReflectedField,
    .Throw = NJI_Throw,
    .ThrowNew = NJI_ThrowNew,
    .ExceptionOccurred = NJI_ExceptionOccurred,
    .ExceptionDescribe = NJI_ExceptionDescribe,
    .ExceptionClear = NJI_ExceptionClear,
    .FatalError = NJI_FatalError,
    .PushLocalFrame = NJI_PushLocalFrame,
    .PopLocalFrame = NJI_PopLocalFrame,
    .NewGlobalRef = NJI_NewGlobalRef,
    .DeleteGlobalRef = NJI_DeleteGlobalRef,
    .DeleteLocalRef = NJI_DeleteLocalRef,
    .IsSameObject = NJI_IsSameObject,
    .NewLocalRef = NJI_NewLocalRef,
    .EnsureLocalCapacity = NJI_EnsureLocalCapacity,
    .AllocObject = NJI_AllocObject,
    .NewObject = NJI_NewObject,
    .NewObjectV = NJI_NewObjectV,
    .NewObjectA = NJI_NewObjectA,
    .GetObjectClass = NJI_GetObjectClass,
    .IsInstanceOf = NJI_IsInstanceOf,
    .GetMethodID = NJI_GetMethodID,
    .CallObjectMethod = NJI_CallObjectMethod,
    .CallObjectMethodV = NJI_CallObjectMethodV,
    .CallObjectMethodA = NJI_CallObjectMethodA,
    .CallBooleanMethod = NJI_CallBooleanMethod,
    .CallBooleanMethodV = NJI_CallBooleanMethodV,
    .CallBooleanMethodA = NJI_CallBooleanMethodA,
    .CallByteMethod = NJI_CallByteMethod,
    .CallByteMethodV = NJI_CallByteMethodV,
    .CallByteMethodA = NJI_CallByteMethodA,
    .CallCharMethod = NJI_CallCharMethod,
    .CallCharMethodV = NJI_CallCharMethodV,
    .CallCharMethodA = NJI_CallCharMethodA,
    .CallShortMethod = NJI_CallShortMethod,
    .CallShortMethodV = NJI_CallShortMethodV,
    .CallShortMethodA = NJI_CallShortMethodA,
    .CallIntMethod = NJI_CallIntMethod,
    .CallIntMethodV = NJI_CallIntMethodV,
    .CallIntMethodA = NJI_CallIntMethodA,
    .CallLongMethod = NJI_CallLongMethod,
    .CallLongMethodV = NJI_CallLongMethodV,
    .CallLongMethodA = NJI_CallLongMethodA,
    .CallFloatMethod = NJI_CallFloatMethod,
    .CallFloatMethodV = NJI_CallFloatMethodV,
    .CallFloatMethodA = NJI_CallFloatMethodA,
    .CallDoubleMethod = NJI_CallDoubleMethod,
    .CallDoubleMethodV = NJI_CallDoubleMethodV,
    .CallDoubleMethodA = NJI_CallDoubleMethodA,
    .CallVoidMethod = NJI_CallVoidMethod,
    .CallVoidMethodV = NJI_CallVoidMethodV,
    .CallVoidMethodA = NJI_CallVoidMethodA,
    .CallNonvirtualObjectMethod = NJI_CallNonvirtualObjectMethod,
    .CallNonvirtualObjectMethodV = NJI_CallNonvirtualObjectMethodV,
    .CallNonvirtualObjectMethodA = NJI_CallNonvirtualObjectMethodA,
    .CallNonvirtualBooleanMethod = NJI_CallNonvirtualBooleanMethod,
    .CallNonvirtualBooleanMethodV = NJI_CallNonvirtualBooleanMethodV,
    .CallNonvirtualBooleanMethodA = NJI_CallNonvirtualBooleanMethodA,
    .CallNonvirtualByteMethod = NJI_CallNonvirtualByteMethod,
    .CallNonvirtualByteMethodV = NJI_CallNonvirtualByteMethodV,
    .CallNonvirtualByteMethodA = NJI_CallNonvirtualByteMethodA,
    .CallNonvirtualCharMethod = NJI_CallNonvirtualCharMethod,
    .CallNonvirtualCharMethodV = NJI_CallNonvirtualCharMethodV,
    .CallNonvirtualCharMethodA = NJI_CallNonvirtualCharMethodA,
    .CallNonvirtualShortMethod = NJI_CallNonvirtualShortMethod,
    .CallNonvirtualShortMethodV = NJI_CallNonvirtualShortMethodV,
    .CallNonvirtualShortMethodA = NJI_CallNonvirtualShortMethodA,
    .CallNonvirtualIntMethod = NJI_CallNonvirtualIntMethod,
    .CallNonvirtualIntMethodV = NJI_CallNonvirtualIntMethodV,
    .CallNonvirtualIntMethodA = NJI_CallNonvirtualIntMethodA,
    .CallNonvirtualLongMethod = NJI_CallNonvirtualLongMethod,
    .CallNonvirtualLongMethodV = NJI_CallNonvirtualLongMethodV,
    .CallNonvirtualLongMethodA = NJI_CallNonvirtualLongMethodA,
    .CallNonvirtualFloatMethod = NJI_CallNonvirtualFloatMethod,
    .CallNonvirtualFloatMethodV = NJI_CallNonvirtualFloatMethodV,
    .CallNonvirtualFloatMethodA = NJI_CallNonvirtualFloatMethodA,
    .CallNonvirtualDoubleMethod = NJI_CallNonvirtualDoubleMethod,
    .CallNonvirtualDoubleMethodV = NJI_CallNonvirtualDoubleMethodV,
    .CallNonvirtualDoubleMethodA = NJI_CallNonvirtualDoubleMethodA,
    .CallNonvirtualVoidMethod = NJI_CallNonvirtualVoidMethod,
    .CallNonvirtualVoidMethodV = NJI_CallNonvirtualVoidMethodV,
    .CallNonvirtualVoidMethodA = NJI_CallNonvirtualVoidMethodA,
    .GetFieldID = NJI_GetFieldID,
    .GetObjectField = NJI_GetObjectField,
    .GetBooleanField = NJI_GetBooleanField,
    .GetByteField = NJI_GetByteField,
    .GetCharField = NJI_GetCharField,
    .GetShortField = NJI_GetShortField,
    .GetIntField = NJI_GetIntField,
    .GetLongField = NJI_GetLongField,
    .GetFloatField = NJI_GetFloatField,
    .GetDoubleField = NJI_GetDoubleField,
    .SetObjectField = NJI_SetObjectField,
    .SetBooleanField = NJI_SetBooleanField,
    .SetByteField = NJI_SetByteField,
    .SetCharField = NJI_SetCharField,
    .SetShortField = NJI_SetShortField,
    .SetIntField = NJI_SetIntField,
    .SetLongField = NJI_SetLongField,
    .SetFloatField = NJI_SetFloatField,
    .SetDoubleField = NJI_SetDoubleField,
    .GetStaticMethodID = NJI_GetStaticMethodID,
    .CallStaticObjectMethod = NJI_CallStaticObjectMethod,
    .CallStaticObjectMethodV = NJI_CallStaticObjectMethodV,
    .CallStaticObjectMethodA = NJI_CallStaticObjectMethodA,
    .CallStaticBooleanMethod = NJI_CallStaticBooleanMethod,
    .CallStaticBooleanMethodV = NJI_CallStaticBooleanMethodV,
    .CallStaticBooleanMethodA = NJI_CallStaticBooleanMethodA,
    .CallStaticByteMethod = NJI_CallStaticByteMethod,
    .CallStaticByteMethodV = NJI_CallStaticByteMethodV,
    .CallStaticByteMethodA = NJI_CallStaticByteMethodA,
    .CallStaticCharMethod = NJI_CallStaticCharMethod,
    .CallStaticCharMethodV = NJI_CallStaticCharMethodV,
    .CallStaticCharMethodA = NJI_CallStaticCharMethodA,
    .CallStaticShortMethod = NJI_CallStaticShortMethod,
    .CallStaticShortMethodV = NJI_CallStaticShortMethodV,
    .CallStaticShortMethodA = NJI_CallStaticShortMethodA,
    .CallStaticIntMethod = NJI_CallStaticIntMethod,
    .CallStaticIntMethodV = NJI_CallStaticIntMethodV,
    .CallStaticIntMethodA = NJI_CallStaticIntMethodA,
    .CallStaticLongMethod = NJI_CallStaticLongMethod,
    .CallStaticLongMethodV = NJI_CallStaticLongMethodV,
    .CallStaticLongMethodA = NJI_CallStaticLongMethodA,
    .CallStaticFloatMethod = NJI_CallStaticFloatMethod,
    .CallStaticFloatMethodV = NJI_CallStaticFloatMethodV,
    .CallStaticFloatMethodA = NJI_CallStaticFloatMethodA,
    .CallStaticDoubleMethod = NJI_CallStaticDoubleMethod,
    .CallStaticDoubleMethodV = NJI_CallStaticDoubleMethodV,
    .CallStaticDoubleMethodA = NJI_CallStaticDoubleMethodA,
    .CallStaticVoidMethod = NJI_CallStaticVoidMethod,
    .CallStaticVoidMethodV = NJI_CallStaticVoidMethodV,
    .CallStaticVoidMethodA = NJI_CallStaticVoidMethodA,
    .GetStaticFieldID = NJI_GetStaticFieldID,
    .GetStaticObjectField = NJI_GetStaticObjectField,
    .GetStaticBooleanField = NJI_GetStaticBooleanField,
    .GetStaticByteField = NJI_GetStaticByteField,
    .GetStaticCharField = NJI_GetStaticCharField,
    .GetStaticShortField = NJI_GetStaticShortField,
    .GetStaticIntField = NJI_GetStaticIntField,
    .GetStaticLongField = NJI_GetStaticLongField,
    .GetStaticFloatField = NJI_GetStaticFloatField,
    .GetStaticDoubleField = NJI_GetStaticDoubleField,
    .SetStaticObjectField = NJI_SetStaticObjectField,
    .SetStaticBooleanField = NJI_SetStaticBooleanField,
    .SetStaticByteField = NJI_SetStaticByteField,
    .SetStaticCharField = NJI_SetStaticCharField,
    .SetStaticShortField = NJI_SetStaticShortField,
    .SetStaticIntField = NJI_SetStaticIntField,
    .SetStaticLongField = NJI_SetStaticLongField,
    .SetStaticFloatField = NJI_SetStaticFloatField,
    .SetStaticDoubleField = NJI_SetStaticDoubleField,
    .NewString = NJI_NewString,
    .GetStringLength = NJI_GetStringLength,
    .GetStringChars = NJI_GetStringChars,
    .ReleaseStringChars = NJI_ReleaseStringChars,
    .NewStringUTF = NJI_NewStringUTF,
    .GetStringUTFLength = NJI_GetStringUTFLength,
    .GetStringUTFChars = NJI_GetStringUTFChars,
    .ReleaseStringUTFChars = NJI_ReleaseStringUTFChars,
    .GetArrayLength = NJI_GetArrayLength,
    .NewObjectArray = NJI_NewObjectArray,
    .GetObjectArrayElement = NJI_GetObjectArrayElement,
    .SetObjectArrayElement = NJI_SetObjectArrayElement,
    .NewBooleanArray = NJI_NewBooleanArray,
    .NewByteArray = NJI_NewByteArray,
    .NewCharArray = NJI_NewCharArray,
    .NewShortArray = NJI_NewShortArray,
    .NewIntArray = NJI_NewIntArray,
    .NewLongArray = NJI_NewLongArray,
    .NewFloatArray = NJI_NewFloatArray,
    .NewDoubleArray = NJI_NewDoubleArray,
    .GetBooleanArrayElements = NJI_GetBooleanArrayElements,
    .GetByteArrayElements = NJI_GetByteArrayElements,
    .GetCharArrayElements = NJI_GetCharArrayElements,
    .GetShortArrayElements = NJI_GetShortArrayElements,
    .GetIntArrayElements = NJI_GetIntArrayElements,
    .GetLongArrayElements = NJI_GetLongArrayElements,
    .GetFloatArrayElements = NJI_GetFloatArrayElements,
    .GetDoubleArrayElements = NJI_GetDoubleArrayElements,
    .ReleaseBooleanArrayElements = NJI_ReleaseBooleanArrayElements,
    .ReleaseByteArrayElements = NJI_ReleaseByteArrayElements,
    .ReleaseCharArrayElements = NJI_ReleaseCharArrayElements,
    .ReleaseShortArrayElements = NJI_ReleaseShortArrayElements,
    .ReleaseIntArrayElements = NJI_ReleaseIntArrayElements,
    .ReleaseLongArrayElements = NJI_ReleaseLongArrayElements,
    .ReleaseFloatArrayElements = NJI_ReleaseFloatArrayElements,
    .ReleaseDoubleArrayElements = NJI_ReleaseDoubleArrayElements,
    .GetBooleanArrayRegion = NJI_GetBooleanArrayRegion,
    .GetByteArrayRegion = NJI_GetByteArrayRegion,
    .GetCharArrayRegion = NJI_GetCharArrayRegion,
    .GetShortArrayRegion = NJI_GetShortArrayRegion,
    .GetIntArrayRegion = NJI_GetIntArrayRegion,
    .GetLongArrayRegion = NJI_GetLongArrayRegion,
    .GetFloatArrayRegion = NJI_GetFloatArrayRegion,
    .GetDoubleArrayRegion = NJI_GetDoubleArrayRegion,
    .SetBooleanArrayRegion = NJI_SetBooleanArrayRegion,
    .SetByteArrayRegion = NJI_SetByteArrayRegion,
    .SetCharArrayRegion = NJI_SetCharArrayRegion,
    .SetShortArrayRegion = NJI_SetShortArrayRegion,
    .SetIntArrayRegion = NJI_SetIntArrayRegion,
    .SetLongArrayRegion = NJI_SetLongArrayRegion,
    .SetFloatArrayRegion = NJI_SetFloatArrayRegion,
    .SetDoubleArrayRegion = NJI_SetDoubleArrayRegion,
    .RegisterNatives = NJI_RegisterNatives,
    .UnregisterNatives = NJI_UnregisterNatives,
    .MonitorEnter = NJI_MonitorEnter,
    .MonitorExit = NJI_MonitorExit,
    .GetJavaVM = NJI_GetJavaVM,
    .GetStringRegion = NJI_GetStringRegion,
    .GetStringUTFRegion = NJI_GetStringUTFRegion,
    .GetPrimitiveArrayCritical = NJI_GetPrimitiveArrayCritical,
    .ReleasePrimitiveArrayCritical = NJI_ReleasePrimitiveArrayCritical,
    .GetStringCritical = NJI_GetStringCritical,
    .ReleaseStringCritical = NJI_ReleaseStringCritical,
    .NewWeakGlobalRef = NJI_NewWeakGlobalRef,
    .DeleteWeakGlobalRef = NJI_DeleteWeakGlobalRef,
    .ExceptionCheck = NJI_ExceptionCheck,
    .NewDirectByteBuffer = NJI_NewDirectByteBuffer,
    .GetDirectBufferAddress = NJI_GetDirectBufferAddress,
    .GetDirectBufferCapacity = NJI_GetDirectBufferCapacity,
    .GetObjectRefType = NJI_GetObjectRefType,
};

/* ==========================================================================
 * 5. JNI INVOKE INTERFACE (JavaVM)
 * ========================================================================== */

struct JNIInvokeInterface_struct {
    void *reserved0;
    void *reserved1;
    void *reserved2;
    jint (*DestroyJavaVM)(JavaVM *);
    jint (*AttachCurrentThread)(JavaVM *, void **, void *);
    jint (*DetachCurrentThread)(JavaVM *);
    jint (*GetEnv)(JavaVM *, void **, jint);
    jint (*AttachCurrentThreadAsDaemon)(JavaVM *, void **, void *);
};

static const struct JNINativeInterface_struct *g_jni_env_vtbl_ptr = &g_jni_env_vtbl;
static const struct JNIInvokeInterface_struct *g_java_vm_vtbl_ptr = NULL;

static jint JVM_GetEnv(JavaVM *vm, void **env, jint version) {
    (void)vm; (void)version;
    if (env) *env = (void *)&g_jni_env_vtbl_ptr;
    return JNI_OK;
}
static jint JVM_AttachCurrentThread(JavaVM *vm, void **p_env, void *thr) {
    (void)vm; (void)thr;
    if (p_env) *p_env = (void *)&g_jni_env_vtbl_ptr;
    return JNI_OK;
}
static jint JVM_DetachCurrentThread(JavaVM *vm){ (void)vm; return JNI_OK; }
static jint JVM_DestroyJavaVM(JavaVM *vm){ (void)vm; return JNI_OK; }
static jint JVM_AttachCurrentThreadAsDaemon(JavaVM *vm, void **p_env, void *thr){
    return JVM_AttachCurrentThread(vm, p_env, thr);
}

static const struct JNIInvokeInterface_struct g_java_vm_vtbl = {
    .DestroyJavaVM = JVM_DestroyJavaVM,
    .AttachCurrentThread = JVM_AttachCurrentThread,
    .DetachCurrentThread = JVM_DetachCurrentThread,
    .GetEnv = JVM_GetEnv,
    .AttachCurrentThreadAsDaemon = JVM_AttachCurrentThreadAsDaemon,
};

/* Zeiger auf Zeiger – entspricht der C++ _JNIEnv/_JavaVM-Layout-Erwartung */
static const struct JNINativeInterface_struct **g_jni_env = &g_jni_env_vtbl_ptr;
static const struct JNIInvokeInterface_struct **g_java_vm = &g_java_vm_vtbl_ptr;

/* ==========================================================================
 * 6. LOGGING-SHIMS
 * ========================================================================== */

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    (void)prio;
    va_list args;
    fprintf(stdout, "[%s] ", tag ? tag : "NFS");
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fputc('\n', stdout);
    fflush(stdout);
    return 0;
}

int __android_log_write(int prio, const char *tag, const char *text) {
    (void)prio;
    fprintf(stdout, "[%s] %s\n", tag ? tag : "NFS", text ? text : "");
    fflush(stdout);
    return 0;
}

int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap) {
    (void)prio;
    fprintf(stdout, "[%s] ", tag ? tag : "NFS");
    vfprintf(stdout, fmt, ap);
    fputc('\n', stdout);
    fflush(stdout);
    return 0;
}

/* ==========================================================================
 * 7. C++ ABI / RTTI / STL SPEICHER-PUFFER
 *
 * Wichtig: Diese müssen ECHTER, beschreibbarer Speicher sein, damit
 * Vtable-Pointer-Aufrufe aus der Hauptbibliothek nicht auf NULL treffen.
 * Größe 256 * 4 = 1 KiB pro Symbol reicht für jede Vtable / jedes RTTI-Objekt.
 * ========================================================================== */

#define ABI_PUFFER(name) uintptr_t name[256] = {0}

ABI_PUFFER(_ZTVN10__cxxabiv117__class_type_infoE);
ABI_PUFFER(_ZTVN10__cxxabiv119__pointer_type_infoE);
ABI_PUFFER(_ZTVN10__cxxabiv120__function_type_infoE);
ABI_PUFFER(_ZTVN10__cxxabiv120__si_class_type_infoE);
ABI_PUFFER(_ZTVN10__cxxabiv121__vmi_class_type_infoE);
ABI_PUFFER(_ZTVN10__cxxabiv123__fundamental_type_infoE);
ABI_PUFFER(_ZTVN10__cxxabiv129__pointer_to_member_type_infoE);

ABI_PUFFER(_ZTIN10__cxxabiv117__class_type_infoE);
ABI_PUFFER(_ZTIN10__cxxabiv119__pointer_type_infoE);
ABI_PUFFER(_ZTIN10__cxxabiv120__function_type_infoE);
ABI_PUFFER(_ZTIN10__cxxabiv120__si_class_type_infoE);
ABI_PUFFER(_ZTIN10__cxxabiv121__vmi_class_type_infoE);
ABI_PUFFER(_ZTIN10__cxxabiv123__fundamental_type_infoE);
ABI_PUFFER(_ZTIN10__cxxabiv129__pointer_to_member_type_infoE);

ABI_PUFFER(_ZTISt9exception);
ABI_PUFFER(_ZTISt9bad_alloc);
ABI_PUFFER(_ZTISt9type_info);
ABI_PUFFER(_ZTISt13runtime_error);
ABI_PUFFER(_ZTISt12out_of_range);
ABI_PUFFER(_ZTISt11logic_error);
ABI_PUFFER(_ZTISt16invalid_argument);
ABI_PUFFER(_ZTISt12length_error);
ABI_PUFFER(_ZTISt9bad_cast);
ABI_PUFFER(_ZTISt10bad_typeid);
ABI_PUFFER(_ZTVSt9exception);
ABI_PUFFER(_ZTVSt9bad_alloc);
ABI_PUFFER(_ZTVSt9type_info);
ABI_PUFFER(_ZTVSt13runtime_error);
ABI_PUFFER(_ZTVSt12out_of_range);
ABI_PUFFER(_ZTVSt11logic_error);
ABI_PUFFER(_ZTVSt16invalid_argument);
ABI_PUFFER(_ZTVSt12length_error);
ABI_PUFFER(_ZTVSt9bad_cast);
ABI_PUFFER(_ZTVSt10bad_typeid);

ABI_PUFFER(_ZNSt6__ndk14cerrE);
ABI_PUFFER(_ZNSt6__ndk14coutE);
ABI_PUFFER(_ZNSt6__ndk15clogE);
ABI_PUFFER(_ZNSt6__ndk14wcerrE);
ABI_PUFFER(_ZNSt6__ndk14wcoutE);
ABI_PUFFER(_ZNSt6__ndk15ctypeIcE2idE);
ABI_PUFFER(_ZNSt6__ndk17num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
ABI_PUFFER(_ZNSt6__ndk17num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
ABI_PUFFER(_ZNSt6__ndk19money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
ABI_PUFFER(_ZNSt6__ndk19money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
ABI_PUFFER(_ZNSt6__ndk18time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
ABI_PUFFER(_ZNSt6__ndk18time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
ABI_PUFFER(_ZNSt6__ndk18ios_base7failureD1Ev);
ABI_PUFFER(_ZNSt6__ndk18ios_base7failureD2Ev);
ABI_PUFFER(_ZNSt6__ndk18ios_baseD2Ev);
ABI_PUFFER(_ZNSt6__ndk19basic_iosIcSt11char_traitsIcEED1Ev);
ABI_PUFFER(_ZNSt6__ndk19basic_iosIcSt11char_traitsIcEED2Ev);
ABI_PUFFER(_ZNSt6__ndk119__shared_weak_countD2Ev);
ABI_PUFFER(_ZNSt6__ndk114__shared_countD2Ev);
ABI_PUFFER(_ZNSt6__ndk112bad_weak_ptrD1Ev);
ABI_PUFFER(_ZNSt6__ndk112bad_weak_ptrD2Ev);
ABI_PUFFER(_ZNSt6__ndk114__shared_count12__add_sharedEv);
ABI_PUFFER(_ZNSt6__ndk114__shared_count16__release_sharedEv);
ABI_PUFFER(_ZTVNSt6__ndk118ios_base7failureE);
ABI_PUFFER(_ZTVNSt6__ndk112bad_weak_ptrE);
ABI_PUFFER(_ZTINSt6__ndk118ios_base7failureE);
ABI_PUFFER(_ZTINSt6__ndk112bad_weak_ptrE);
ABI_PUFFER(_ZTVNSt6__ndk19basic_iosIcSt11char_traitsIcEEE);
ABI_PUFFER(_ZTVNSt6__ndk18ios_baseE);

/* ==========================================================================
 * 8. C++ EXCEPTION-HILFSFUNKTIONEN
 * ========================================================================== */

void _ZNSt9exceptionD2Ev(void *p){ (void)p; }
void _ZNSt9exceptionD1Ev(void *p){ (void)p; }
void _ZNSt9exceptionD0Ev(void *p){ (void)p; }
void _ZNSt13runtime_errorD2Ev(void *p){ (void)p; }
void _ZNSt13runtime_errorD1Ev(void *p){ (void)p; }
void _ZNSt9bad_allocD2Ev(void *p){ (void)p; }
void _ZNSt9bad_allocD1Ev(void *p){ (void)p; }
void _ZNSt9type_infoD2Ev(void *p){ (void)p; }
void _ZNSt9type_infoD1Ev(void *p){ (void)p; }
void _ZSt9terminatev(void){ fprintf(stderr,"[NFS Loader] std::terminate aufgerufen\n"); exit(1); }
void _ZSt10unexpectedv(void){ fprintf(stderr,"[NFS Loader] std::unexpected aufgerufen\n"); exit(1); }
void __cxa_pure_virtual(void){ fprintf(stderr,"[NFS Loader] pure virtual call!\n"); exit(1); }
void __cxa_deleted_virtual(void){ fprintf(stderr,"[NFS Loader] deleted virtual call!\n"); exit(1); }
void *__cxa_allocate_exception(size_t n){ return calloc(1, n ? n : 1); }
void __cxa_free_exception(void *p){ free(p); }
void __cxa_throw(void *exc, void *tinfo, void *dest){
    (void)exc; (void)tinfo; (void)dest;
    fprintf(stderr,"[NFS Loader] C++ Exception geworfen (Mock)\n");
    abort();
}
void *__cxa_begin_catch(void *exc){ return exc; }
void __cxa_end_catch(void){ }
void *__cxa_get_exception_ptr(void *exc){ return exc; }
void __cxa_rethrow(void){ abort(); }
void *__gxx_personality_v0(void){ return NULL; }

/* ==========================================================================
 * 9. GAME-SPEZIFISCHE JNI NATIVE-STUBS
 * ========================================================================== */

void Java_com_ea_ironmonkey_MogaController_nativeOnKeyEvent(JNIEnv *e, jobject o, jint k, jint a){ (void)e;(void)o;(void)k;(void)a; }
void Java_com_ea_ironmonkey_MogaController_nativeOnMotionEvent(JNIEnv *e, jobject o, jint ax, jfloat v){ (void)e;(void)o;(void)ax;(void)v; }
void Java_com_ea_ironmonkey_MogaController_nativeOnStateEvent(JNIEnv *e, jobject o, jint s, jint v){ (void)e;(void)o;(void)s;(void)v; }
void Java_com_ea_ironmonkey_MogaController_nativeOnGenericMotionEvent(JNIEnv *e, jobject o, jint s, jfloat v){ (void)e;(void)o;(void)s;(void)v; }
void Java_com_ea_ironmonkey_MogaController_nativeOnJoystickEvent(JNIEnv *e, jobject o, jint ax, jfloat v){ (void)e;(void)o;(void)ax;(void)v; }

void Java_com_ea_ironmonkey_NativeActivity_nativeOnCreate(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnResume(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnPause(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnDestroy(JNIEnv *e, jobject o){ (void)e;(void)o; }

void Java_com_ea_ironmonkey_NativeRenderer_nativeOnSurfaceCreated(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeRenderer_nativeOnSurfaceChanged(JNIEnv *e, jobject o, jint w, jint h){ (void)e;(void)o;(void)w;(void)h; }
void Java_com_ea_ironmonkey_NativeRenderer_nativeOnDrawFrame(JNIEnv *e, jobject o){ (void)e;(void)o; }

/* ==========================================================================
 * 10. DLOPEN-HELFER
 * ========================================================================== */

static void *try_load_so(const char *libname) {
    char path[512];
    void *handle = NULL;
    const char *dirs[] = { "libs/", "./", "" };
    for (size_t i = 0; i < sizeof(dirs)/sizeof(dirs[0]); i++) {
        snprintf(path, sizeof(path), "%s%s", dirs[i], libname);
        handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
        if (handle) {
            printf("[NFS Loader] OK: %s\n", path);
            return handle;
        }
    }
    printf("[NFS Loader] Hinweis: '%s' nicht geladen (%s)\n", libname, dlerror());
    return NULL;
}

/* ==========================================================================
 * 11. MAIN
 * ========================================================================== */

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("[NFS Loader] Need for Speed Most Wanted – ARMv7 (RK3326/A35) Port\n");
    printf("[NFS Loader] PID: %d\n", getpid());

    char cwd[512];
    if (getcwd(cwd, sizeof(cwd))) printf("[NFS Loader] CWD: %s\n", cwd);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "[NFS Loader] SDL_Init Fehler: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window *window = SDL_CreateWindow(
        "Need for Speed Most Wanted",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
    );
    if (!window) {
        fprintf(stderr, "[NFS Loader] Fenster-Fehler: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        fprintf(stderr, "[NFS Loader] GL-Kontext-Fehler: %s\n", SDL_GetError());
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }
    SDL_GL_SetSwapInterval(1);

    /* 1) Preload in korrekter Reihenfolge */
    const char *preload[] = {
        "libc++_shared.so",
        "libgnustl_shared.so",
        "liblog.so",
        "libandroid.so",
        "libjnigraphics.so",
        "libfmodex.so",
        "libfmodevent.so",
        "libNimble.so",
    };
    for (size_t i = 0; i < sizeof(preload)/sizeof(preload[0]); i++)
        try_load_so(preload[i]);

    /* 2) Hauptbibliothek laden */
    void *so = try_load_so("libNFSMW.so");
    if (!so) {
        fprintf(stderr, "[NFS Loader] FATAL: libNFSMW.so nicht ladbar: %s\n", dlerror());
        SDL_GL_DeleteContext(gl_ctx); SDL_DestroyWindow(window); SDL_Quit();
        return 1;
    }
    printf("[NFS Loader] libNFSMW.so geladen @ %p\n", so);

    /* 3) JNI_OnLoad aufrufen – mit korrektem JavaVM**-Layout */
    typedef jint (*JNI_OnLoad_t)(JavaVM *, void *);
    JNI_OnLoad_t onload = (JNI_OnLoad_t)dlsym(so, "JNI_OnLoad");
    if (onload) {
        printf("[NFS Loader] Rufe JNI_OnLoad auf...\n");
        jint r = onload((JavaVM *)g_java_vm, NULL);
        printf("[NFS Loader] JNI_OnLoad -> %d\n", r);
    } else {
        printf("[NFS Loader] Kein JNI_OnLoad vorhanden (ok).\n");
    }

    /* 4) Optionaler natives Einstiegspunkt */
    typedef void (*entry_t)(void);
    const char *candidates[] = {
        "ANativeActivity_onCreate",
        "SDL_main",
        "main",
        "native_main",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        void *sym = dlsym(so, candidates[i]);
        if (sym) {
            printf("[NFS Loader] Einstieg '%s' @ %p – rufe auf\n", candidates[i], sym);
            ((entry_t)sym)();
            break;
        }
    }

    /* 5) Event-Loop */
    int running = 1;
    SDL_Event ev;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }
        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    printf("[NFS Loader] Beende...\n");
    dlclose(so);
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}