#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

// JNI Datentypen & Konstanten definieren
#define JNI_FALSE 0
#define JNI_TRUE  1
typedef uint8_t jboolean;
typedef int32_t jint;
typedef void*   jobject;
typedef void*   JNIEnv;

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
// JNI SPOOFING: Täuscht der EA-Engine vor, dass ein MOGA-Controller aktiv ist.
// Dadurch schaltet das Spiel alle Touchscreen-Buttons und HUD-Icons stumm.
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

    jboolean Java_com_ea_games_nfs13_1row_NFS_isGamepadConnected(JNIEnv* env, jobject thiz) {
        return JNI_TRUE; // Signalisiert permanent verbundenen Controller
    }

    jint Java_com_ea_games_nfs13_1row_NFS_getGamepadType(JNIEnv* env, jobject thiz) {
        return 1; // 1 = Physical Gamepad (MOGA Pro Mode -> blendet Touch-HUD aus)
    }

} // extern "C"

int main(int argc, char* argv[]) {
    printf("[NFS Loader] Starte RK3326 Native Port...\n");

    // 1. SDL2 Video & Controller initialisieren
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[NFS Loader] FEHLER: SDL_Init fehlgeschlagen: %s\n", SDL_GetError());
        return 1;
    }

    // 2. Direct OpenGL ES 2.0 Context aufbauen
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
        return 1;
    }

    g_gl_context = SDL_GL_CreateContext(g_window);
    SDL_GL_SetSwapInterval(1); // Lock auf 60 FPS V-Sync

    // 3. Android Spielbibliothek laden
    printf("[NFS Loader] Lade libNFSMW.so...\n");
    g_game_handle = dlopen("./libNFSMW.so", RTLD_NOW | RTLD_GLOBAL);

    if (!g_game_handle) {
        printf("[NFS Loader] FEHLER: libNFSMW.so nicht gefunden! (%s)\n", dlerror());
        SDL_Quit();
        return 1;
    }

    // 4. JNI_OnLoad ausführen
    JNI_OnLoad_t JNI_OnLoad_fn = (JNI_OnLoad_t)dlsym(g_game_handle, "JNI_OnLoad");
    if (JNI_OnLoad_fn) {
        JNI_OnLoad_fn(NULL, NULL);
    }

    // Symbol-Adressen auflösen
    g_NativeInit = (NativeInit_t)dlsym(g_game_handle, "nativeInit");
    g_NativeRender = (NativeRender_t)dlsym(g_game_handle, "nativeRender");
    g_NativeTouchEvent = (NativeTouchEvent_t)dlsym(g_game_handle, "nativeTouchEvent");

    if (g_NativeInit) {
        printf("[NFS Loader] Initialisiere Engine mit 640x480...\n");
        g_NativeInit(640, 480);
    }

    // 5. Game Loop
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

        // Render-Schritt der Engine ausführen
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
