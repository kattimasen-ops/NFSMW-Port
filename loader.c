/* ==========================================================================
 * NFS Most Wanted – ARMv7 (RK3326/Cortex-A35) Port Loader
 * Version mit GL-Stack-Erkennung + Multistufen-Fallback
 * ========================================================================== */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <execinfo.h>
#include <ucontext.h>
#include <SDL2/SDL.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

/* ==========================================================================
 * 0. FRÜH-DIAGNOSE
 * ========================================================================== */

static void rawmsg(const char *s) {
    if (!s) return;
    size_t n = strlen(s);
    ssize_t r1 = write(2, s, n); (void)r1;
    ssize_t r2 = write(1, s, n); (void)r2;
}

static void rawf(const char *fmt, ...) {
    char buf[512];
    va_list a;
    va_start(a, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    if (n > 0) {
        ssize_t r1 = write(2, buf, (size_t)n); (void)r1;
        ssize_t r2 = write(1, buf, (size_t)n); (void)r2;
    }
}

__attribute__((constructor))
static void __early_ctor(void) {
    rawmsg("[NFS] >>> .init_array constructor reached (pre-main OK)\n");
}

static const char *signame(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV";
        case SIGBUS:  return "SIGBUS";
        case SIGILL:  return "SIGILL";
        case SIGABRT: return "SIGABRT";
        case SIGFPE:  return "SIGFPE";
        default:      return "UNKNOWN";
    }
}

static void crash_handler(int sig, siginfo_t *si, void *ctx) {
    (void)ctx;
    rawf("\n[NFS] *** SIGNAL %d (%s) ***\n", sig, signame(sig));
    rawf("  si_addr = %p\n", si ? si->si_addr : NULL);
    rawf("  si_code = %d\n", si ? si->si_code : -1);
    rawf("  pid     = %d\n", (int)getpid());

    void *bt[32];
    int frames = backtrace(bt, 32);
    rawmsg("[NFS] --- backtrace ---\n");
    backtrace_symbols_fd(bt, frames, 2);
    rawmsg("[NFS] --- end backtrace ---\n");

    _exit(128 + sig);
}

static void install_crash_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
}

/* ==========================================================================
 * 1. BIONIC-SHIMS
 * ========================================================================== */
uintptr_t __stack_chk_guard = 0xd00a5300;
void __stack_chk_fail(void){ rawmsg("[NFS] STACK CORRUPTION\n"); _exit(1); }
int *__errno(void){ return &errno; }
long long strtoll_shim(const char *n, char **e, int b){ return strtoll(n, e, b); }
unsigned long long strtoull_shim(const char *n, char **e, int b){ return strtoull(n, e, b); }

/* ==========================================================================
 * 2. JNI-TYPEN
 * ========================================================================== */
typedef int32_t  jint;
typedef int64_t  jlong;
typedef int8_t   jbyte;
typedef uint16_t jchar;
typedef int16_t  jshort;
typedef float    jfloat;
typedef double   jdouble;
typedef uint8_t  jboolean;
typedef jint     jsize;

typedef void*    jobject;
typedef void*    jclass;
typedef void*    jstring;
typedef void*    jarray;
typedef void*    jobjectArray;
typedef void*    jbooleanArray;
typedef void*    jbyteArray;
typedef void*    jcharArray;
typedef void*    jshortArray;
typedef void*    jintArray;
typedef void*    jlongArray;
typedef void*    jfloatArray;
typedef void*    jdoubleArray;
typedef void*    jthrowable;
typedef void*    jweak;
typedef void*    jmethodID;
typedef void*    jfieldID;

typedef union {
    jboolean z; jbyte b; jchar c; jshort s; jint i; jlong j;
    jfloat f; jdouble d; jobject l;
} jvalue;

struct JNINativeInterface_struct;
struct JNIInvokeInterface_struct;
typedef const struct JNINativeInterface_struct *JNIEnv;
typedef const struct JNIInvokeInterface_struct *JavaVM;

#define JNI_OK 0
#define JNI_ERR (-1)
#define JNI_FALSE 0
#define JNI_TRUE 1
#define JNI_VERSION_1_6 0x00010006

/* ==========================================================================
 * 3. JNI-STUBS (JNIEnv)
 * ========================================================================== */

static jint   E_GetVersion(JNIEnv *e){ (void)e; return JNI_VERSION_1_6; }
static jclass E_DefineClass(JNIEnv *e, const char *n, jobject l, const jbyte *b, jsize s){ (void)e;(void)n;(void)l;(void)b;(void)s; return (jclass)1; }
static jclass E_FindClass(JNIEnv *e, const char *n){ (void)e;(void)n; return (jclass)1; }
static jmethodID E_FromReflectedMethod(JNIEnv *e, jobject m){ (void)e;(void)m; return (jmethodID)1; }
static jfieldID  E_FromReflectedField(JNIEnv *e, jobject f){ (void)e;(void)f; return (jfieldID)1; }
static jobject   E_ToReflectedMethod(JNIEnv *e, jclass c, jmethodID m, jboolean s){ (void)e;(void)c;(void)m;(void)s; return (jobject)1; }
static jclass    E_GetSuperclass(JNIEnv *e, jclass c){ (void)e;(void)c; return (jclass)1; }
static jboolean  E_IsAssignableFrom(JNIEnv *e, jclass a, jclass b){ (void)e;(void)a;(void)b; return JNI_TRUE; }
static jobject   E_ToReflectedField(JNIEnv *e, jclass c, jfieldID f, jboolean s){ (void)e;(void)c;(void)f;(void)s; return (jobject)1; }
static jint      E_Throw(JNIEnv *e, jthrowable t){ (void)e;(void)t; return 0; }
static jint      E_ThrowNew(JNIEnv *e, jclass c, const char *m){ (void)e;(void)c;(void)m; return 0; }
static jthrowable E_ExceptionOccurred(JNIEnv *e){ (void)e; return NULL; }
static void      E_ExceptionDescribe(JNIEnv *e){ (void)e; }
static void      E_ExceptionClear(JNIEnv *e){ (void)e; }
static void      E_FatalError(JNIEnv *e, const char *m){ (void)e; rawf("[JNI] %s\n", m?m:""); _exit(1); }
static jint      E_PushLocalFrame(JNIEnv *e, jint c){ (void)e;(void)c; return 0; }
static jobject   E_PopLocalFrame(JNIEnv *e, jobject r){ (void)e; return r; }
static jobject   E_NewGlobalRef(JNIEnv *e, jobject o){ (void)e; return o; }
static void      E_DeleteGlobalRef(JNIEnv *e, jobject o){ (void)e;(void)o; }
static void      E_DeleteLocalRef(JNIEnv *e, jobject o){ (void)e;(void)o; }
static jboolean  E_IsSameObject(JNIEnv *e, jobject a, jobject b){ (void)e; return a==b?JNI_TRUE:JNI_FALSE; }
static jobject   E_NewLocalRef(JNIEnv *e, jobject o){ (void)e; return o; }
static jint      E_EnsureLocalCapacity(JNIEnv *e, jint c){ (void)e;(void)c; return 0; }
static jobject   E_AllocObject(JNIEnv *e, jclass c){ (void)e;(void)c; return (jobject)1; }
static jobject   E_NewObject(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return (jobject)1; }
static jobject   E_NewObjectV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return (jobject)1; }
static jobject   E_NewObjectA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return (jobject)1; }
static jclass    E_GetObjectClass(JNIEnv *e, jobject o){ (void)e;(void)o; return (jclass)1; }
static jboolean  E_IsInstanceOf(JNIEnv *e, jobject o, jclass c){ (void)e;(void)o;(void)c; return JNI_TRUE; }
static jmethodID E_GetMethodID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jmethodID)1; }

static jobject   E_CallObj(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return NULL; }
static jobject   E_CallObjV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return NULL; }
static jobject   E_CallObjA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return NULL; }
static jboolean  E_CallBool(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jboolean  E_CallBoolV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jboolean  E_CallBoolA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jbyte     E_CallByte(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jbyte     E_CallByteV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jbyte     E_CallByteA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jchar     E_CallChar(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jchar     E_CallCharV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jchar     E_CallCharA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jshort    E_CallShort(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jshort    E_CallShortV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jshort    E_CallShortA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jint      E_CallInt(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jint      E_CallIntV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jint      E_CallIntA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jlong     E_CallLong(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0; }
static jlong     E_CallLongV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jlong     E_CallLongA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0; }
static jfloat    E_CallFloat(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0.f; }
static jfloat    E_CallFloatV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0.f; }
static jfloat    E_CallFloatA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0.f; }
static jdouble   E_CallDouble(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; return 0.0; }
static jdouble   E_CallDoubleV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; return 0.0; }
static jdouble   E_CallDoubleA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; return 0.0; }
static void      E_CallVoid(JNIEnv *e, jobject o, jmethodID m, ...){ (void)e;(void)o;(void)m; }
static void      E_CallVoidV(JNIEnv *e, jobject o, jmethodID m, va_list a){ (void)e;(void)o;(void)m;(void)a; }
static void      E_CallVoidA(JNIEnv *e, jobject o, jmethodID m, jvalue *a){ (void)e;(void)o;(void)m;(void)a; }

static jobject   E_CNObj(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return NULL; }
static jobject   E_CNObjV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return NULL; }
static jobject   E_CNObjA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return NULL; }
static jboolean  E_CNBool(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jboolean  E_CNBoolV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jboolean  E_CNBoolA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jbyte     E_CNByte(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jbyte     E_CNByteV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jbyte     E_CNByteA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jchar     E_CNChar(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jchar     E_CNCharV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jchar     E_CNCharA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jshort    E_CNShort(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jshort    E_CNShortV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jshort    E_CNShortA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jint      E_CNInt(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jint      E_CNIntV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jint      E_CNIntA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jlong     E_CNLong(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0; }
static jlong     E_CNLongV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jlong     E_CNLongA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0; }
static jfloat    E_CNFloat(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0.f; }
static jfloat    E_CNFloatV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.f; }
static jfloat    E_CNFloatA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.f; }
static jdouble   E_CNDouble(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; return 0.0; }
static jdouble   E_CNDoubleV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.0; }
static jdouble   E_CNDoubleA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; return 0.0; }
static void      E_CNVoid(JNIEnv *e, jobject o, jclass c, jmethodID m, ...){ (void)e;(void)o;(void)c;(void)m; }
static void      E_CNVoidV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a){ (void)e;(void)o;(void)c;(void)m;(void)a; }
static void      E_CNVoidA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a){ (void)e;(void)o;(void)c;(void)m;(void)a; }

static jfieldID  E_GetFieldID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jfieldID)1; }
static jobject   E_GetObjField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return NULL; }
static jboolean  E_GetBoolField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jbyte     E_GetByteField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jchar     E_GetCharField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jshort    E_GetShortField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jint      E_GetIntField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jlong     E_GetLongField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0; }
static jfloat    E_GetFloatField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0.f; }
static jdouble   E_GetDoubleField(JNIEnv *e, jobject o, jfieldID f){ (void)e;(void)o;(void)f; return 0.0; }
static void      E_SetObjField(JNIEnv *e, jobject o, jfieldID f, jobject v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetBoolField(JNIEnv *e, jobject o, jfieldID f, jboolean v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetByteField(JNIEnv *e, jobject o, jfieldID f, jbyte v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetCharField(JNIEnv *e, jobject o, jfieldID f, jchar v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetShortField(JNIEnv *e, jobject o, jfieldID f, jshort v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetIntField(JNIEnv *e, jobject o, jfieldID f, jint v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetLongField(JNIEnv *e, jobject o, jfieldID f, jlong v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetFloatField(JNIEnv *e, jobject o, jfieldID f, jfloat v){ (void)e;(void)o;(void)f;(void)v; }
static void      E_SetDoubleField(JNIEnv *e, jobject o, jfieldID f, jdouble v){ (void)e;(void)o;(void)f;(void)v; }

static jmethodID E_GetStaticMID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jmethodID)1; }
static jobject   E_CallStatObj(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return NULL; }
static jobject   E_CallStatObjV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return NULL; }
static jobject   E_CallStatObjA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return NULL; }
static jboolean  E_CallStatBool(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jboolean  E_CallStatBoolV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jboolean  E_CallStatBoolA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jbyte     E_CallStatByte(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jbyte     E_CallStatByteV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jbyte     E_CallStatByteA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jchar     E_CallStatChar(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jchar     E_CallStatCharV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jchar     E_CallStatCharA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jshort    E_CallStatShort(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jshort    E_CallStatShortV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jshort    E_CallStatShortA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jint      E_CallStatInt(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jint      E_CallStatIntV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jint      E_CallStatIntA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jlong     E_CallStatLong(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0; }
static jlong     E_CallStatLongV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jlong     E_CallStatLongA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0; }
static jfloat    E_CallStatFloat(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0.f; }
static jfloat    E_CallStatFloatV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0.f; }
static jfloat    E_CallStatFloatA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0.f; }
static jdouble   E_CallStatDouble(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; return 0.0; }
static jdouble   E_CallStatDoubleV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; return 0.0; }
static jdouble   E_CallStatDoubleA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; return 0.0; }
static void      E_CallStatVoid(JNIEnv *e, jclass c, jmethodID m, ...){ (void)e;(void)c;(void)m; }
static void      E_CallStatVoidV(JNIEnv *e, jclass c, jmethodID m, va_list a){ (void)e;(void)c;(void)m;(void)a; }
static void      E_CallStatVoidA(JNIEnv *e, jclass c, jmethodID m, jvalue *a){ (void)e;(void)c;(void)m;(void)a; }

static jfieldID  E_GetStatFID(JNIEnv *e, jclass c, const char *n, const char *s){ (void)e;(void)c;(void)n;(void)s; return (jfieldID)1; }
static jobject   E_GetStatObj(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return NULL; }
static jboolean  E_GetStatBool(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jbyte     E_GetStatByte(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jchar     E_GetStatChar(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jshort    E_GetStatShort(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jint      E_GetStatInt(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jlong     E_GetStatLong(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0; }
static jfloat    E_GetStatFloat(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0.f; }
static jdouble   E_GetStatDouble(JNIEnv *e, jclass c, jfieldID f){ (void)e;(void)c;(void)f; return 0.0; }
static void      E_SetStatObj(JNIEnv *e, jclass c, jfieldID f, jobject v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatBool(JNIEnv *e, jclass c, jfieldID f, jboolean v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatByte(JNIEnv *e, jclass c, jfieldID f, jbyte v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatChar(JNIEnv *e, jclass c, jfieldID f, jchar v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatShort(JNIEnv *e, jclass c, jfieldID f, jshort v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatInt(JNIEnv *e, jclass c, jfieldID f, jint v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatLong(JNIEnv *e, jclass c, jfieldID f, jlong v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatFloat(JNIEnv *e, jclass c, jfieldID f, jfloat v){ (void)e;(void)c;(void)f;(void)v; }
static void      E_SetStatDouble(JNIEnv *e, jclass c, jfieldID f, jdouble v){ (void)e;(void)c;(void)f;(void)v; }

static jstring  E_NewString(JNIEnv *e, const jchar *u, jsize l){ (void)e;(void)u;(void)l; return (jstring)1; }
static jsize    E_GetStrLen(JNIEnv *e, jstring s){ (void)e;(void)s; return 0; }
static const jchar *E_GetStrChars(JNIEnv *e, jstring s, jboolean *c){ (void)e;(void)s; if(c)*c=0; return (const jchar*)L""; }
static void     E_RelStrChars(JNIEnv *e, jstring s, const jchar *c){ (void)e;(void)s;(void)c; }
static jstring  E_NewStrUTF(JNIEnv *e, const char *b){ (void)e;(void)b; return (jstring)1; }
static jsize    E_GetStrUTFLen(JNIEnv *e, jstring s){ (void)e;(void)s; return 0; }
static const char *E_GetStrUTF(JNIEnv *e, jstring s, jboolean *c){ (void)e;(void)s; if(c)*c=0; return ""; }
static void     E_RelStrUTF(JNIEnv *e, jstring s, const char *u){ (void)e;(void)s;(void)u; }

static jsize    E_GetArrLen(JNIEnv *e, jarray a){ (void)e;(void)a; return 0; }
static jobjectArray E_NewObjArr(JNIEnv *e, jsize l, jclass c, jobject i){ (void)e;(void)l;(void)c;(void)i; return (jobjectArray)1; }
static jobject  E_GetObjArrElem(JNIEnv *e, jobjectArray a, jsize i){ (void)e;(void)a;(void)i; return NULL; }
static void     E_SetObjArrElem(JNIEnv *e, jobjectArray a, jsize i, jobject v){ (void)e;(void)a;(void)i;(void)v; }

#define MOCK_NEWARR(ret, name) static ret name(JNIEnv *e, jsize l){ (void)e;(void)l; return (ret)1; }
MOCK_NEWARR(jbooleanArray, E_NewBoolArr)
MOCK_NEWARR(jbyteArray,    E_NewByteArr)
MOCK_NEWARR(jcharArray,    E_NewCharArr)
MOCK_NEWARR(jshortArray,   E_NewShortArr)
MOCK_NEWARR(jintArray,     E_NewIntArr)
MOCK_NEWARR(jlongArray,    E_NewLongArr)
MOCK_NEWARR(jfloatArray,   E_NewFloatArr)
MOCK_NEWARR(jdoubleArray,  E_NewDoubleArr)

#define MOCK_GETARR(ret, name) static ret *name(JNIEnv *e, ret a, jboolean *c){ (void)e;(void)a; if(c)*c=0; return NULL; }
MOCK_GETARR(jboolean, E_GetBoolArr)
MOCK_GETARR(jbyte,    E_GetByteArr)
MOCK_GETARR(jchar,    E_GetCharArr)
MOCK_GETARR(jshort,   E_GetShortArr)
MOCK_GETARR(jint,     E_GetIntArr)
MOCK_GETARR(jlong,    E_GetLongArr)
MOCK_GETARR(jfloat,   E_GetFloatArr)
MOCK_GETARR(jdouble,  E_GetDoubleArr)

static void E_RelBoolArr(JNIEnv *e, jbooleanArray a, jboolean *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelByteArr(JNIEnv *e, jbyteArray a, jbyte *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelCharArr(JNIEnv *e, jcharArray a, jchar *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelShortArr(JNIEnv *e, jshortArray a, jshort *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelIntArr(JNIEnv *e, jintArray a, jint *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelLongArr(JNIEnv *e, jlongArray a, jlong *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelFloatArr(JNIEnv *e, jfloatArray a, jfloat *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static void E_RelDoubleArr(JNIEnv *e, jdoubleArray a, jdouble *p, jint m){ (void)e;(void)a;(void)p;(void)m; }

static void E_GetBoolReg(JNIEnv *e, jbooleanArray a, jsize s, jsize l, jboolean *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetByteReg(JNIEnv *e, jbyteArray a, jsize s, jsize l, jbyte *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetCharReg(JNIEnv *e, jcharArray a, jsize s, jsize l, jchar *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetShortReg(JNIEnv *e, jshortArray a, jsize s, jsize l, jshort *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetIntReg(JNIEnv *e, jintArray a, jsize s, jsize l, jint *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetLongReg(JNIEnv *e, jlongArray a, jsize s, jsize l, jlong *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetFloatReg(JNIEnv *e, jfloatArray a, jsize s, jsize l, jfloat *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_GetDoubleReg(JNIEnv *e, jdoubleArray a, jsize s, jsize l, jdouble *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetBoolReg(JNIEnv *e, jbooleanArray a, jsize s, jsize l, const jboolean *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetByteReg(JNIEnv *e, jbyteArray a, jsize s, jsize l, const jbyte *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetCharReg(JNIEnv *e, jcharArray a, jsize s, jsize l, const jchar *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetShortReg(JNIEnv *e, jshortArray a, jsize s, jsize l, const jshort *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetIntReg(JNIEnv *e, jintArray a, jsize s, jsize l, const jint *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetLongReg(JNIEnv *e, jlongArray a, jsize s, jsize l, const jlong *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetFloatReg(JNIEnv *e, jfloatArray a, jsize s, jsize l, const jfloat *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }
static void E_SetDoubleReg(JNIEnv *e, jdoubleArray a, jsize s, jsize l, const jdouble *b){ (void)e;(void)a;(void)s;(void)l;(void)b; }

static jint     E_RegisterNatives(JNIEnv *e, jclass c, const void *m, jint n){ (void)e;(void)c;(void)m;(void)n; return 0; }
static jint     E_UnregisterNatives(JNIEnv *e, jclass c){ (void)e;(void)c; return 0; }
static jint     E_MonitorEnter(JNIEnv *e, jobject o){ (void)e;(void)o; return 0; }
static jint     E_MonitorExit(JNIEnv *e, jobject o){ (void)e;(void)o; return 0; }
static jint     E_GetJavaVM(JNIEnv *e, void **vm){ (void)e; if(vm)*vm=NULL; return 0; }
static void     E_GetStrReg(JNIEnv *e, jstring s, jsize st, jsize l, jchar *b){ (void)e;(void)s;(void)st;(void)l;(void)b; }
static void     E_GetStrUTFReg(JNIEnv *e, jstring s, jsize st, jsize l, char *b){ (void)e;(void)s;(void)st;(void)l;(void)b; }
static void    *E_GetPrimArrCrit(JNIEnv *e, jarray a, jboolean *c){ (void)e;(void)a; if(c)*c=0; return NULL; }
static void     E_RelPrimArrCrit(JNIEnv *e, jarray a, void *p, jint m){ (void)e;(void)a;(void)p;(void)m; }
static const jchar *E_GetStrCrit(JNIEnv *e, jstring s, jboolean *c){ (void)e;(void)s; if(c)*c=0; return (const jchar*)L""; }
static void     E_RelStrCrit(JNIEnv *e, jstring s, const jchar *c){ (void)e;(void)s;(void)c; }
static jweak    E_NewWeakGRef(JNIEnv *e, jobject o){ (void)e; return (jweak)o; }
static void     E_DelWeakGRef(JNIEnv *e, jweak w){ (void)e;(void)w; }
static jboolean E_ExceptionCheck(JNIEnv *e){ (void)e; return 0; }
static jobject  E_NewDirectBB(JNIEnv *e, void *a, jlong c){ (void)e;(void)a;(void)c; return (jobject)1; }
static void    *E_GetDirectBAddr(JNIEnv *e, jobject b){ (void)e;(void)b; return NULL; }
static jlong    E_GetDirectBCap(JNIEnv *e, jobject b){ (void)e;(void)b; return 0; }
static jint     E_GetObjRefType(JNIEnv *e, jobject o){ (void)e;(void)o; return 0; }

/* ==========================================================================
 * 4. JavaVM-STUBS
 * ========================================================================== */
static jint VM_Destroy(JavaVM *vm){ (void)vm; return 0; }
static jint VM_Attach(JavaVM *vm, void **env, void *thr){ (void)vm;(void)thr; if(env)*env=NULL; return 0; }
static jint VM_Detach(JavaVM *vm){ (void)vm; return 0; }
static jint VM_GetEnv(JavaVM *vm, void **env, jint v){ (void)vm;(void)v; if(env)*env=NULL; return 0; }
static jint VM_AttachDaemon(JavaVM *vm, void **env, void *thr){ return VM_Attach(vm, env, thr); }

/* ==========================================================================
 * 5. VTABLE-ARRAYS
 * ========================================================================== */
static void *g_jni_vtbl[256] = {
    [4]   = (void*)E_GetVersion,
    [5]   = (void*)E_DefineClass,
    [6]   = (void*)E_FindClass,
    [7]   = (void*)E_FromReflectedMethod,
    [8]   = (void*)E_FromReflectedField,
    [9]   = (void*)E_ToReflectedMethod,
    [10]  = (void*)E_GetSuperclass,
    [11]  = (void*)E_IsAssignableFrom,
    [12]  = (void*)E_ToReflectedField,
    [13]  = (void*)E_Throw,
    [14]  = (void*)E_ThrowNew,
    [15]  = (void*)E_ExceptionOccurred,
    [16]  = (void*)E_ExceptionDescribe,
    [17]  = (void*)E_ExceptionClear,
    [18]  = (void*)E_FatalError,
    [19]  = (void*)E_PushLocalFrame,
    [20]  = (void*)E_PopLocalFrame,
    [21]  = (void*)E_NewGlobalRef,
    [22]  = (void*)E_DeleteGlobalRef,
    [23]  = (void*)E_DeleteLocalRef,
    [24]  = (void*)E_IsSameObject,
    [25]  = (void*)E_NewLocalRef,
    [26]  = (void*)E_EnsureLocalCapacity,
    [27]  = (void*)E_AllocObject,
    [28]  = (void*)E_NewObject,
    [29]  = (void*)E_NewObjectV,
    [30]  = (void*)E_NewObjectA,
    [31]  = (void*)E_GetObjectClass,
    [32]  = (void*)E_IsInstanceOf,
    [33]  = (void*)E_GetMethodID,
    [34]  = (void*)E_CallObj,
    [35]  = (void*)E_CallObjV,
    [36]  = (void*)E_CallObjA,
    [37]  = (void*)E_CallBool,
    [38]  = (void*)E_CallBoolV,
    [39]  = (void*)E_CallBoolA,
    [40]  = (void*)E_CallByte,
    [41]  = (void*)E_CallByteV,
    [42]  = (void*)E_CallByteA,
    [43]  = (void*)E_CallChar,
    [44]  = (void*)E_CallCharV,
    [45]  = (void*)E_CallCharA,
    [46]  = (void*)E_CallShort,
    [47]  = (void*)E_CallShortV,
    [48]  = (void*)E_CallShortA,
    [49]  = (void*)E_CallInt,
    [50]  = (void*)E_CallIntV,
    [51]  = (void*)E_CallIntA,
    [52]  = (void*)E_CallLong,
    [53]  = (void*)E_CallLongV,
    [54]  = (void*)E_CallLongA,
    [55]  = (void*)E_CallFloat,
    [56]  = (void*)E_CallFloatV,
    [57]  = (void*)E_CallFloatA,
    [58]  = (void*)E_CallDouble,
    [59]  = (void*)E_CallDoubleV,
    [60]  = (void*)E_CallDoubleA,
    [61]  = (void*)E_CallVoid,
    [62]  = (void*)E_CallVoidV,
    [63]  = (void*)E_CallVoidA,
    [64]  = (void*)E_CNObj,
    [65]  = (void*)E_CNObjV,
    [66]  = (void*)E_CNObjA,
    [67]  = (void*)E_CNBool,
    [68]  = (void*)E_CNBoolV,
    [69]  = (void*)E_CNBoolA,
    [70]  = (void*)E_CNByte,
    [71]  = (void*)E_CNByteV,
    [72]  = (void*)E_CNByteA,
    [73]  = (void*)E_CNChar,
    [74]  = (void*)E_CNCharV,
    [75]  = (void*)E_CNCharA,
    [76]  = (void*)E_CNShort,
    [77]  = (void*)E_CNShortV,
    [78]  = (void*)E_CNShortA,
    [79]  = (void*)E_CNInt,
    [80]  = (void*)E_CNIntV,
    [81]  = (void*)E_CNIntA,
    [82]  = (void*)E_CNLong,
    [83]  = (void*)E_CNLongV,
    [84]  = (void*)E_CNLongA,
    [85]  = (void*)E_CNFloat,
    [86]  = (void*)E_CNFloatV,
    [87]  = (void*)E_CNFloatA,
    [88]  = (void*)E_CNDouble,
    [89]  = (void*)E_CNDoubleV,
    [90]  = (void*)E_CNDoubleA,
    [91]  = (void*)E_CNVoid,
    [92]  = (void*)E_CNVoidV,
    [93]  = (void*)E_CNVoidA,
    [94]  = (void*)E_GetFieldID,
    [95]  = (void*)E_GetObjField,
    [96]  = (void*)E_GetBoolField,
    [97]  = (void*)E_GetByteField,
    [98]  = (void*)E_GetCharField,
    [99]  = (void*)E_GetShortField,
    [100] = (void*)E_GetIntField,
    [101] = (void*)E_GetLongField,
    [102] = (void*)E_GetFloatField,
    [103] = (void*)E_GetDoubleField,
    [104] = (void*)E_SetObjField,
    [105] = (void*)E_SetBoolField,
    [106] = (void*)E_SetByteField,
    [107] = (void*)E_SetCharField,
    [108] = (void*)E_SetShortField,
    [109] = (void*)E_SetIntField,
    [110] = (void*)E_SetLongField,
    [111] = (void*)E_SetFloatField,
    [112] = (void*)E_SetDoubleField,
    [113] = (void*)E_GetStaticMID,
    [114] = (void*)E_CallStatObj,
    [115] = (void*)E_CallStatObjV,
    [116] = (void*)E_CallStatObjA,
    [117] = (void*)E_CallStatBool,
    [118] = (void*)E_CallStatBoolV,
    [119] = (void*)E_CallStatBoolA,
    [120] = (void*)E_CallStatByte,
    [121] = (void*)E_CallStatByteV,
    [122] = (void*)E_CallStatByteA,
    [123] = (void*)E_CallStatChar,
    [124] = (void*)E_CallStatCharV,
    [125] = (void*)E_CallStatCharA,
    [126] = (void*)E_CallStatShort,
    [127] = (void*)E_CallStatShortV,
    [128] = (void*)E_CallStatShortA,
    [129] = (void*)E_CallStatInt,
    [130] = (void*)E_CallStatIntV,
    [131] = (void*)E_CallStatIntA,
    [132] = (void*)E_CallStatLong,
    [133] = (void*)E_CallStatLongV,
    [134] = (void*)E_CallStatLongA,
    [135] = (void*)E_CallStatFloat,
    [136] = (void*)E_CallStatFloatV,
    [137] = (void*)E_CallStatFloatA,
    [138] = (void*)E_CallStatDouble,
    [139] = (void*)E_CallStatDoubleV,
    [140] = (void*)E_CallStatDoubleA,
    [141] = (void*)E_CallStatVoid,
    [142] = (void*)E_CallStatVoidV,
    [143] = (void*)E_CallStatVoidA,
    [144] = (void*)E_GetStatFID,
    [145] = (void*)E_GetStatObj,
    [146] = (void*)E_GetStatBool,
    [147] = (void*)E_GetStatByte,
    [148] = (void*)E_GetStatChar,
    [149] = (void*)E_GetStatShort,
    [150] = (void*)E_GetStatInt,
    [151] = (void*)E_GetStatLong,
    [152] = (void*)E_GetStatFloat,
    [153] = (void*)E_GetStatDouble,
    [154] = (void*)E_SetStatObj,
    [155] = (void*)E_SetStatBool,
    [156] = (void*)E_SetStatByte,
    [157] = (void*)E_SetStatChar,
    [158] = (void*)E_SetStatShort,
    [159] = (void*)E_SetStatInt,
    [160] = (void*)E_SetStatLong,
    [161] = (void*)E_SetStatFloat,
    [162] = (void*)E_SetStatDouble,
    [163] = (void*)E_NewString,
    [164] = (void*)E_GetStrLen,
    [165] = (void*)E_GetStrChars,
    [166] = (void*)E_RelStrChars,
    [167] = (void*)E_NewStrUTF,
    [168] = (void*)E_GetStrUTFLen,
    [169] = (void*)E_GetStrUTF,
    [170] = (void*)E_RelStrUTF,
    [171] = (void*)E_GetArrLen,
    [172] = (void*)E_NewObjArr,
    [173] = (void*)E_GetObjArrElem,
    [174] = (void*)E_SetObjArrElem,
    [175] = (void*)E_NewBoolArr,
    [176] = (void*)E_NewByteArr,
    [177] = (void*)E_NewCharArr,
    [178] = (void*)E_NewShortArr,
    [179] = (void*)E_NewIntArr,
    [180] = (void*)E_NewLongArr,
    [181] = (void*)E_NewFloatArr,
    [182] = (void*)E_NewDoubleArr,
    [183] = (void*)E_GetBoolArr,
    [184] = (void*)E_GetByteArr,
    [185] = (void*)E_GetCharArr,
    [186] = (void*)E_GetShortArr,
    [187] = (void*)E_GetIntArr,
    [188] = (void*)E_GetLongArr,
    [189] = (void*)E_GetFloatArr,
    [190] = (void*)E_GetDoubleArr,
    [191] = (void*)E_RelBoolArr,
    [192] = (void*)E_RelByteArr,
    [193] = (void*)E_RelCharArr,
    [194] = (void*)E_RelShortArr,
    [195] = (void*)E_RelIntArr,
    [196] = (void*)E_RelLongArr,
    [197] = (void*)E_RelFloatArr,
    [198] = (void*)E_RelDoubleArr,
    [199] = (void*)E_GetBoolReg,
    [200] = (void*)E_GetByteReg,
    [201] = (void*)E_GetCharReg,
    [202] = (void*)E_GetShortReg,
    [203] = (void*)E_GetIntReg,
    [204] = (void*)E_GetLongReg,
    [205] = (void*)E_GetFloatReg,
    [206] = (void*)E_GetDoubleReg,
    [207] = (void*)E_SetBoolReg,
    [208] = (void*)E_SetByteReg,
    [209] = (void*)E_SetCharReg,
    [210] = (void*)E_SetShortReg,
    [211] = (void*)E_SetIntReg,
    [212] = (void*)E_SetLongReg,
    [213] = (void*)E_SetFloatReg,
    [214] = (void*)E_SetDoubleReg,
    [215] = (void*)E_RegisterNatives,
    [216] = (void*)E_UnregisterNatives,
    [217] = (void*)E_MonitorEnter,
    [218] = (void*)E_MonitorExit,
    [219] = (void*)E_GetJavaVM,
    [220] = (void*)E_GetStrReg,
    [221] = (void*)E_GetStrUTFReg,
    [222] = (void*)E_GetPrimArrCrit,
    [223] = (void*)E_RelPrimArrCrit,
    [224] = (void*)E_GetStrCrit,
    [225] = (void*)E_RelStrCrit,
    [226] = (void*)E_NewWeakGRef,
    [227] = (void*)E_DelWeakGRef,
    [228] = (void*)E_ExceptionCheck,
    [229] = (void*)E_NewDirectBB,
    [230] = (void*)E_GetDirectBAddr,
    [231] = (void*)E_GetDirectBCap,
    [232] = (void*)E_GetObjRefType,
};

static void *g_vm_vtbl[8] = {
    [0] = NULL,
    [1] = NULL,
    [2] = NULL,
    [3] = (void*)VM_Destroy,
    [4] = (void*)VM_Attach,
    [5] = (void*)VM_Detach,
    [6] = (void*)VM_GetEnv,
    [7] = (void*)VM_AttachDaemon,
};

static void *g_jni_env_handle __attribute__((unused)) = (void*)g_jni_vtbl;
static void *g_java_vm_handle = (void*)g_vm_vtbl;

/* ==========================================================================
 * 6. LOGGING-SHIMS
 * ========================================================================== */
int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    (void)prio;
    char buf[1024];
    int n = snprintf(buf, sizeof(buf), "[%s] ", tag?tag:"NFS");
    va_list a; va_start(a, fmt);
    int m = vsnprintf(buf + n, sizeof(buf) - (size_t)n, fmt, a);
    va_end(a);
    int total = n + (m > 0 ? m : 0);
    if (total < (int)sizeof(buf) - 1) { buf[total++] = '\n'; buf[total] = 0; }
    ssize_t r1 = write(1, buf, (size_t)total); (void)r1;
    ssize_t r2 = write(2, buf, (size_t)total); (void)r2;
    return 0;
}
int __android_log_write(int prio, const char *tag, const char *text) {
    (void)prio; rawf("[%s] %s\n", tag?tag:"NFS", text?text:""); return 0;
}
int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap) {
    (void)prio;
    char buf[1024];
    int n = snprintf(buf, sizeof(buf), "[%s] ", tag?tag:"NFS");
    int m = vsnprintf(buf + n, sizeof(buf) - (size_t)n, fmt, ap);
    int total = n + (m > 0 ? m : 0);
    if (total < (int)sizeof(buf) - 1) { buf[total++] = '\n'; buf[total] = 0; }
    ssize_t r1 = write(1, buf, (size_t)total); (void)r1;
    ssize_t r2 = write(2, buf, (size_t)total); (void)r2;
    return 0;
}

/* ==========================================================================
 * 7. C++ ABI / RTTI / STL PUFFER
 * ========================================================================== */
#define P(name) uintptr_t name[256] = {0}
P(_ZTVN10__cxxabiv117__class_type_infoE);
P(_ZTVN10__cxxabiv119__pointer_type_infoE);
P(_ZTVN10__cxxabiv120__function_type_infoE);
P(_ZTVN10__cxxabiv120__si_class_type_infoE);
P(_ZTVN10__cxxabiv121__vmi_class_type_infoE);
P(_ZTVN10__cxxabiv123__fundamental_type_infoE);
P(_ZTVN10__cxxabiv129__pointer_to_member_type_infoE);
P(_ZTIN10__cxxabiv117__class_type_infoE);
P(_ZTIN10__cxxabiv119__pointer_type_infoE);
P(_ZTIN10__cxxabiv120__function_type_infoE);
P(_ZTIN10__cxxabiv120__si_class_type_infoE);
P(_ZTIN10__cxxabiv121__vmi_class_type_infoE);
P(_ZTIN10__cxxabiv123__fundamental_type_infoE);
P(_ZTIN10__cxxabiv129__pointer_to_member_type_infoE);
P(_ZTISt9exception);
P(_ZTISt9bad_alloc);
P(_ZTISt9type_info);
P(_ZTISt13runtime_error);
P(_ZTISt12out_of_range);
P(_ZTISt11logic_error);
P(_ZTISt16invalid_argument);
P(_ZTISt12length_error);
P(_ZTISt9bad_cast);
P(_ZTISt10bad_typeid);
P(_ZTVSt9exception);
P(_ZTVSt9bad_alloc);
P(_ZTVSt9type_info);
P(_ZTVSt13runtime_error);
P(_ZTVSt12out_of_range);
P(_ZTVSt11logic_error);
P(_ZTVSt16invalid_argument);
P(_ZTVSt12length_error);
P(_ZTVSt9bad_cast);
P(_ZTVSt10bad_typeid);
P(_ZNSt6__ndk14cerrE);
P(_ZNSt6__ndk14coutE);
P(_ZNSt6__ndk15clogE);
P(_ZNSt6__ndk14wcerrE);
P(_ZNSt6__ndk14wcoutE);
P(_ZNSt6__ndk15ctypeIcE2idE);
P(_ZNSt6__ndk17num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
P(_ZNSt6__ndk17num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
P(_ZNSt6__ndk19money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
P(_ZNSt6__ndk19money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
P(_ZNSt6__ndk18time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
P(_ZNSt6__ndk18time_putIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
P(_ZNSt6__ndk18ios_base7failureD1Ev);
P(_ZNSt6__ndk18ios_base7failureD2Ev);
P(_ZNSt6__ndk18ios_baseD2Ev);
P(_ZNSt6__ndk19basic_iosIcSt11char_traitsIcEED1Ev);
P(_ZNSt6__ndk19basic_iosIcSt11char_traitsIcEED2Ev);
P(_ZNSt6__ndk119__shared_weak_countD2Ev);
P(_ZNSt6__ndk114__shared_countD2Ev);
P(_ZNSt6__ndk112bad_weak_ptrD1Ev);
P(_ZNSt6__ndk112bad_weak_ptrD2Ev);
P(_ZNSt6__ndk114__shared_count12__add_sharedEv);
P(_ZNSt6__ndk114__shared_count16__release_sharedEv);
P(_ZTVNSt6__ndk118ios_base7failureE);
P(_ZTVNSt6__ndk112bad_weak_ptrE);
P(_ZTINSt6__ndk118ios_base7failureE);
P(_ZTINSt6__ndk112bad_weak_ptrE);
P(_ZTVNSt6__ndk19basic_iosIcSt11char_traitsIcEEE);
P(_ZTVNSt6__ndk18ios_baseE);

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
void _ZSt9terminatev(void){ rawmsg("[NFS] std::terminate\n"); _exit(1); }
void _ZSt10unexpectedv(void){ rawmsg("[NFS] std::unexpected\n"); _exit(1); }
void __cxa_pure_virtual(void){ rawmsg("[NFS] pure virtual call\n"); _exit(1); }
void __cxa_deleted_virtual(void){ rawmsg("[NFS] deleted virtual call\n"); _exit(1); }
void *__cxa_allocate_exception(size_t n){ return calloc(1, n?n:1); }
void __cxa_free_exception(void *p){ free(p); }
void __cxa_throw(void *e, void *t, void *d){ (void)e;(void)t;(void)d; rawmsg("[NFS] C++ Exception (Mock)\n"); abort(); }
void *__cxa_begin_catch(void *e){ return e; }
void __cxa_end_catch(void){ }
void *__cxa_get_exception_ptr(void *e){ return e; }
void __cxa_rethrow(void){ abort(); }
void *__gxx_personality_v0(void){ return NULL; }

/* ==========================================================================
 * 9. GAME-SPEZIFISCHE JNI-NATIVES
 * ========================================================================== */
void Java_com_ea_ironmonkey_MogaController_nativeOnKeyEvent(JNIEnv *e, jobject o, jint k, jint a){ (void)e;(void)o;(void)k;(void)a; }
void Java_com_ea_ironmonkey_MogaController_nativeOnMotionEvent(JNIEnv *e, jobject o, jint x, jfloat v){ (void)e;(void)o;(void)x;(void)v; }
void Java_com_ea_ironmonkey_MogaController_nativeOnStateEvent(JNIEnv *e, jobject o, jint s, jint v){ (void)e;(void)o;(void)s;(void)v; }
void Java_com_ea_ironmonkey_MogaController_nativeOnGenericMotionEvent(JNIEnv *e, jobject o, jint s, jfloat v){ (void)e;(void)o;(void)s;(void)v; }
void Java_com_ea_ironmonkey_MogaController_nativeOnJoystickEvent(JNIEnv *e, jobject o, jint x, jfloat v){ (void)e;(void)o;(void)x;(void)v; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnCreate(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnResume(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnPause(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeActivity_nativeOnDestroy(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeRenderer_nativeOnSurfaceCreated(JNIEnv *e, jobject o){ (void)e;(void)o; }
void Java_com_ea_ironmonkey_NativeRenderer_nativeOnSurfaceChanged(JNIEnv *e, jobject o, jint w, jint h){ (void)e;(void)o;(void)w;(void)h; }
void Java_com_ea_ironmonkey_NativeRenderer_nativeOnDrawFrame(JNIEnv *e, jobject o){ (void)e;(void)o; }

/* ==========================================================================
 * 10. GL-STACK-CHECK
 * ========================================================================== */
static void check_gl_symbols(void) {
    rawmsg("[NFS] --- GL-Stack-Check ---\n");
    const char *symbols[] = {
        "eglGetDisplay", "eglInitialize", "eglGetPlatformDisplay",
        "glGetString", "glCreateShader", "gbm_create_device",
        NULL
    };
    for (int i = 0; symbols[i]; i++) {
        void *p = dlsym(RTLD_DEFAULT, symbols[i]);
        rawf("[NFS]   dlsym(RTLD_DEFAULT, \"%s\") = %p\n", symbols[i], p);
    }
    /* Versuche auch, libEGL und libGLESv2 direkt zu dlopen */
    const char *libs[] = {
        "libEGL.so.1", "libEGL.so",
        "libGLESv2.so.2", "libGLESv2.so",
        "libgbm.so.1", "libgbm.so",
        "libmali.so.1", "libmali.so",
        NULL
    };
    for (int i = 0; libs[i]; i++) {
        void *h = dlopen(libs[i], RTLD_NOW | RTLD_GLOBAL);
        rawf("[NFS]   dlopen(\"%s\") = %p  (%s)\n", libs[i], h, h ? "OK" : dlerror());
    }
}

/* ==========================================================================
 * 11. DRM-VORPRÜFUNG
 * ========================================================================== */
static void probe_drm(void) {
    rawmsg("[NFS] --- DRM-Vorprüfung ---\n");
    rawf("[NFS]   uid=%d gid=%d euid=%d egid=%d\n",
         (int)getuid(), (int)getgid(), (int)geteuid(), (int)getegid());

    struct stat st;
    if (stat("/dev/dri", &st) != 0) rawf("[NFS]   /dev/dri: %s\n", strerror(errno));
    else rawf("[NFS]   /dev/dri: mode=%o uid=%d gid=%d\n", st.st_mode & 07777, (int)st.st_uid, (int)st.st_gid);

    if (stat("/dev/dri/card0", &st) != 0) rawf("[NFS]   /dev/dri/card0: %s\n", strerror(errno));
    else rawf("[NFS]   /dev/dri/card0: mode=%o uid=%d gid=%d\n", st.st_mode & 07777, (int)st.st_uid, (int)st.st_gid);

    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) rawf("[NFS]   open(card0, RDWR): %s\n", strerror(errno));
    else { rawf("[NFS]   open(card0, RDWR): OK fd=%d\n", fd); close(fd); }
}

/* ==========================================================================
 * 12. DLOPEN-HELFER
 * ========================================================================== */
static void *try_load_so(const char *name){
    char p[512];
    void *h;
    const char *dirs[] = {"libs/", "./", ""};
    for (size_t i = 0; i < sizeof(dirs)/sizeof(dirs[0]); i++){
        snprintf(p, sizeof p, "%s%s", dirs[i], name);
        rawf("[NFS]   dlopen(\"%s\", RTLD_NOW|RTLD_GLOBAL)...\n", p);
        h = dlopen(p, RTLD_NOW | RTLD_GLOBAL);
        if (h){ rawf("[NFS]   -> OK @ %p\n", h); return h; }
        rawf("[NFS]   -> FAIL: %s\n", dlerror());
    }
    rawf("[NFS] Hinweis: '%s' nicht geladen\n", name);
    return NULL;
}

/* ==========================================================================
 * 13. MAIN
 * ========================================================================== */
int main(int argc, char **argv){
    (void)argc; (void)argv;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    rawmsg("[NFS] >>> main() reached\n");
    install_crash_handlers();

    rawf("[NFS Loader] NFS Most Wanted – ARMv7 RK3326/A35 Port\n");
    rawf("[NFS Loader] PID: %d\n", (int)getpid());

    char cwd[512];
    if (getcwd(cwd, sizeof cwd)) rawf("[NFS Loader] CWD: %s\n", cwd);

    /* SDL-Version + Treiber */
    {
        SDL_version ver;
        SDL_GetVersion(&ver);
        rawf("[NFS] SDL Version: %d.%d.%d\n", ver.major, ver.minor, ver.patch);
        const char *vd = SDL_getenv("SDL_VIDEODRIVER");
        rawf("[NFS] SDL_VIDEODRIVER = %s\n", vd ? vd : "(nicht gesetzt)");
        int nv = SDL_GetNumVideoDrivers();
        rawf("[NFS] Video-Treiber: %d verfügbar\n", nv);
        for (int i = 0; i < nv; i++) rawf("[NFS]   video[%d] = %s\n", i, SDL_GetVideoDriver(i));
    }

    /* DRM + GL-Check */
    probe_drm();
    check_gl_symbols();

    /* SDL-Subsysteme einzeln hochfahren */
    rawmsg("[NFS] >>> SDL_InitSubSystem(VIDEO)...\n");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) rawf("[NFS] VIDEO FAIL: %s\n", SDL_GetError());
    else rawmsg("[NFS] VIDEO OK\n");

    rawmsg("[NFS] >>> SDL_InitSubSystem(AUDIO)...\n");
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) rawf("[NFS] AUDIO FAIL: %s\n", SDL_GetError());
    else rawmsg("[NFS] AUDIO OK\n");

    rawmsg("[NFS] >>> SDL_InitSubSystem(GAMECONTROLLER)...\n");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) rawf("[NFS] GAMECONTROLLER FAIL: %s\n", SDL_GetError());
    else rawmsg("[NFS] GAMECONTROLLER OK\n");

    rawmsg("[NFS] >>> SDL_InitSubSystem(EVENTS)...\n");
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0) rawf("[NFS] EVENTS FAIL: %s\n", SDL_GetError());
    else rawmsg("[NFS] EVENTS OK\n");

    /* ---- WICHTIG: Wir versuchen zuerst OHNE SDL_WINDOW_OPENGL ---- */
    rawmsg("[NFS] >>> SDL_CreateWindow (OHNE OpenGL-Flag)...\n");
    SDL_Window *w = SDL_CreateWindow("NFS MW",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
        SDL_WINDOW_SHOWN);
    if (!w) {
        rawf("[NFS] Fenster (ohne GL): %s\n", SDL_GetError());
    } else {
        rawmsg("[NFS] Fenster (ohne GL) OK – teste jetzt GL-Kontext\n");
        SDL_GLContext gl = SDL_GL_CreateContext(w);
        if (!gl) rawf("[NFS] GL-Kontext (ohne Flag): %s\n", SDL_GetError());
        else rawmsg("[NFS] GL-Kontext (ohne Flag) OK\n");
        if (gl) SDL_GL_DeleteContext(gl);
        SDL_DestroyWindow(w);
    }

    /* ---- Zweiter Versuch MIT SDL_WINDOW_OPENGL ---- */
    rawmsg("[NFS] >>> SDL_CreateWindow (MIT OpenGL-Flag)...\n");
    w = SDL_CreateWindow("NFS MW",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!w) {
        rawf("[NFS] Fenster (mit GL): %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }
    rawmsg("[NFS] Fenster (mit GL) OK\n");

    SDL_GLContext gl = SDL_GL_CreateContext(w);
    if (!gl) {
        rawf("[NFS] GL-Kontext: %s\n", SDL_GetError());
        SDL_DestroyWindow(w); SDL_Quit();
        return 3;
    }
    SDL_GL_SetSwapInterval(1);
    rawmsg("[NFS] >>> GL-Kontext OK\n");

    /* Preload */
    const char *pre[] = {
        "libc++_shared.so","libgnustl_shared.so","liblog.so","libandroid.so",
        "libjnigraphics.so","libfmodex.so","libfmodevent.so","libNimble.so"
    };
    rawmsg("[NFS] >>> Preload-Bibliotheken...\n");
    for (size_t i = 0; i < sizeof(pre)/sizeof(pre[0]); i++) try_load_so(pre[i]);

    rawmsg("[NFS] >>> libNFSMW.so laden...\n");
    void *so = try_load_so("libNFSMW.so");
    if (!so){
        rawf("[NFS] FATAL: %s\n", dlerror());
        SDL_GL_DeleteContext(gl); SDL_DestroyWindow(w); SDL_Quit(); return 1;
    }
    rawf("[NFS Loader] libNFSMW.so @ %p\n", so);

    typedef jint (*OnLoad_t)(void*, void*);
    OnLoad_t onload = (OnLoad_t)dlsym(so, "JNI_OnLoad");
    if (onload){
        rawmsg("[NFS] >>> rufe JNI_OnLoad...\n");
        jint r = onload(&g_java_vm_handle, NULL);
        rawf("[NFS Loader] JNI_OnLoad -> %d\n", r);
    } else rawmsg("[NFS] Kein JNI_OnLoad\n");

    const char *cands[] = {"ANativeActivity_onCreate","SDL_main","main","native_main",NULL};
    for (int i = 0; cands[i]; i++){
        void *s = dlsym(so, cands[i]);
        if (s){ rawf("[NFS] >>> starte '%s' @ %p\n", cands[i], s); ((void(*)(void))s)(); break; }
    }

    rawmsg("[NFS] >>> Eventloop\n");
    int running = 1; SDL_Event ev;
    while (running){
        while (SDL_PollEvent(&ev)){
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }
        SDL_GL_SwapWindow(w);
        SDL_Delay(16);
    }

    rawmsg("[NFS] >>> beende\n");
    dlclose(so); SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(w); SDL_Quit();
    return 0;
}