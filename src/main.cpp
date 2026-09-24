#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

// JNI Datentypen & Konstanten
#define JNI_FALSE 0
#define JNI_TRUE  1
#define JNI_OK    0

typedef uint8_t jboolean;
typedef int32_t jint;
typedef void*   jobject;

// ---------------------------------------------------------------------------
// DUMMY JAVAVM / JNIENV
// Verhindert Segmentation Faults, falls JNI_OnLoad vm->GetEnv() aufruft
// ---------------------------------------------------------------------------
struct JNINativeInterface_ {
    void* reserved0;
    void* reserved1;
    void* reserved2;
    void* reserved3;
    jint (*GetVersion)(void* env);
};

struct JNIInvokeInterface_ {
    void* reserved0;
    void* reserved1;
    void* reserved2;
    jint (*DestroyJavaVM)(void* vm);
    jint (*AttachCurrentThread)(void* vm, void** p_env, void* thr_args);
    jint (*DetachCurrentThread)(void* vm);
    jint (*GetEnv)(void* vm, void** p_env, jint version);
};

static jint Dummy_GetEnv(void* vm, void** p_env, jint version) {
    static struct JNINativeInterface_ dummy_env_vtbl = { NULL, NULL, NULL, NULL, NULL };
    static struct JNINativeInterface_* dummy_env = &dummy_env_vtbl;
    if (p_env) *p_env = &dummy_env;
    return JNI_OK;
}

static struct JNIInvokeInterface_ dummy_vm_vtbl = {
    NULL, NULL, NULL, NULL, NULL, NULL, Dummy_GetEnv
};

// Funktionszeiger-Typen für libNFSMW.so
typedef jint (*JNI_OnLoad_t)(void* vm, void* reserved);
typedef void (*NativeInit_t)(int width, int height);
typedef void (*NativeRender_t)();
typedef void (*NativeTouchEvent_t)(int action, float x, float y, int pointer_id);

// Globale Variablen
SDL_Window* g_window = NULL;
SDL_GLContext g_gl_context = NULL;
void* g_game_handle = NULL;

NativeInit_t g_NativeInit = NULL;
NativeRender_t g_NativeRender = NULL;
NativeTouchEvent_t g_NativeTouchEvent = NULL;

// ---------------------------------------------------------------------------
// EXPORTIERTE ANDROID BIONIC STUBS & JNI SPOOFING
// Werden dank CMAKE_ENABLE_EXPORTS direkt von libNFSMW.so auflösbar
// ---------------------------------------------------------------------------
extern "C" {

    void __android_log_print(int prio, const char* tag, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        printf("[%s] ", tag ? tag : "NFS");
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
    }

    void __android_log_write(int prio, const char* tag, const char* msg) {
        printf("[%s] %s\n", tag ? tag : "NFS", msg ? msg : "");
    }

    void __android_log_assert(const char* cond, const char* tag, const char* fmt, ...) {
        printf("[%s] ASSERT: %s\n", tag ? tag : "NFS", cond ? cond : "");
    }

    void __android_log_buf_print() {}
    void __android_log_vprint() {}

    // Android Bitmap Stubs
    void AndroidBitmap_getInfo() {}
    void AndroidBitmap_lockPixels() {}
    void AndroidBitmap_unlockPixels() {}

    // Android Asset Stubs
    void AAssetManager_open() {}
    void AAsset_read() {}
    void AAsset_close() {}

    // EA Gamepad Detection JNI Spoofing (MOGA Pro Mode)
    jboolean Java_com_ea_games_nfs13_1row_NFS_isGamepadConnected(void* env, jobject thiz) {
        return JNI_TRUE;
    }

    jint Java_com_ea_games_nfs13_1row_NFS_getGamepadType(void* env, jobject thiz) {
        return 1;
    }

} // extern "C"

int main(int argc, char* argv[]) {
    printf("[NFS Loader] Starte RK3326 Native Port...\n");

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("[NFS Loader] Aktuelles Arbeitsverzeichnis: %s\n", cwd);
    }

    // 1. SDL2 Video & Controller initialisieren
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[NFS Loader] FEHLER: SDL_Init fehlgeschlagen: %s\n", SDL_GetError());
        return 1;
    }

    // 2. OpenGL ES 2.0 Context aufbauen
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    g_window = SDL_CreateWindow("Need for Speed: Most Wanted",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                640, 480,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if (!g_window) {
        printf("[NFS Loader] FEHLER: Fenster konnte nicht erstellt werden: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    g_gl_context = SDL_GL_CreateContext(g_window);
    SDL_GL_SetSwapInterval(1); // 60 FPS V-Sync

    // 3. Android Spielbibliothek mit RTLD_LAZY laden
    printf("[NFS Loader] Lade libNFSMW.so...\n");
    dlerror(); // Vorherigen Fehler-Buffer leeren

    // Versuche verschiedene Pfadvarianten
    g_game_handle = dlopen("libNFSMW.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!g_game_handle) {
        g_game_handle = dlopen("./libNFSMW.so", RTLD_LAZY | RTLD_GLOBAL);
    }
    if (!g_game_handle) {
        g_game_handle = dlopen("/roms/ports/nfs_mw/libNFSMW.so", RTLD_LAZY | RTLD_GLOBAL);
    }

    if (!g_game_handle) {
        const char* err = dlerror();
        printf("[NFS Loader] FEHLER beim Laden von libNFSMW.so: %s\n", err ? err : "Unbekannter Fehler");
        SDL_GL_DeleteContext(g_gl_context);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    printf("[NFS Loader] libNFSMW.so erfolgreich geladen!\n");

    // 4. JNI_OnLoad mit Fake-JavaVM ausführen
    JNI_OnLoad_t JNI_OnLoad_fn = (JNI_OnLoad_t)dlsym(g_game_handle, "JNI_OnLoad");
    if (JNI_OnLoad_fn) {
        printf("[NFS Loader] Führe JNI_OnLoad aus...\n");
        struct JNIInvokeInterface_* fake_vm_ptr = &dummy_vm_vtbl;
        JNI_OnLoad_fn(&fake_vm_ptr, NULL);
    }

    // Symbol-Adressen auflösen
    g_NativeInit = (NativeInit_t)dlsym(g_game_handle, "nativeInit");
    g_NativeRender = (NativeRender_t)dlsym(g_game_handle, "nativeRender");
    g_NativeTouchEvent = (NativeTouchEvent_t)dlsym(g_game_handle, "nativeTouchEvent");

    if (g_NativeInit) {
        printf("[NFS Loader] Initialisiere Engine (640x480)...\n");
        g_NativeInit(640, 480);
    } else {
        printf("[NFS Loader] WARNUNG: nativeInit Symbol nicht gefunden.\n");
    }

    // 5. Hauptschleife
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        if (g_NativeRender) {
            g_NativeRender();
        }

        SDL_GL_SwapWindow(g_window);
    }

    // Shutdown
    dlclose(g_game_handle);
    SDL_GL_DeleteContext(g_gl_context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
