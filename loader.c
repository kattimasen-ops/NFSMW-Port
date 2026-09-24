#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

// JNI Typ-Definition für Standalone-Builds ohne Android NDK
typedef int32_t jint;

// ============================================================================
// STUBS & REIMPLEMENTATIONEN FÜR FEHLENDE LOG-SYMBOLE
// ============================================================================

// 1. Moga Controller Native JNI Stubs
void Java_com_ea_ironmonkey_MogaController_nativeOnKeyEvent(void *env, void *obj, int keycode, int action) {
    (void)env; (void)obj; (void)keycode; (void)action;
}

void Java_com_ea_ironmonkey_MogaController_nativeOnMotionEvent(void *env, void *obj, int axis, float val) {
    (void)env; (void)obj; (void)axis; (void)val;
}

void Java_com_ea_ironmonkey_MogaController_nativeOnStateEvent(void *env, void *obj, int state, int val) {
    (void)env; (void)obj; (void)state; (void)val;
}

// 2. C Runtime / System Stubs
int custom_cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    (void)func; (void)arg; (void)dso_handle;
    return 0;
}

intmax_t custom_strtoimax(const char *nptr, char **endptr, int base) {
    return strtoimax(nptr, endptr, base);
}

long long custom_strtoll(const char *nptr, char **endptr, int base) {
    return strtoll(nptr, endptr, base);
}

unsigned long long custom_strtoull(const char *nptr, char **endptr, int base) {
    return strtoull(nptr, endptr, base);
}

uintmax_t custom_strtoumax(const char *nptr, char **endptr, int base) {
    return strtoumax(nptr, endptr, base);
}

// 3. C++ NDK & ABI Dummy Vtables / RTTI Objects
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
// SYMBOL LOOKUP MAPPER
// ============================================================================

typedef struct {
    const char *name;
    void *address;
} SymbolMap;

static SymbolMap g_symbol_map[] = {
    // Controller JNI
    {"Java_com_ea_ironmonkey_MogaController_nativeOnKeyEvent", (void*)&Java_com_ea_ironmonkey_MogaController_nativeOnKeyEvent},
    {"Java_com_ea_ironmonkey_MogaController_nativeOnMotionEvent", (void*)&Java_com_ea_ironmonkey_MogaController_nativeOnMotionEvent},
    {"Java_com_ea_ironmonkey_MogaController_nativeOnStateEvent", (void*)&Java_com_ea_ironmonkey_MogaController_nativeOnStateEvent},

    // C Standard / libc
    {"__cxa_atexit", (void*)&custom_cxa_atexit},
    {"strtoimax", (void*)&custom_strtoimax},
    {"strtoll", (void*)&custom_strtoll},
    {"strtoull", (void*)&custom_strtoull},
    {"strtoumax", (void*)&custom_strtoumax},

    // C++ ABI & NDK
    {"_ZNSt6__ndk14cerrE", (void*)&_ZNSt6__ndk14cerrE},
    {"_ZNSt6__ndk15ctypeIcE2idE", (void*)&_ZNSt6__ndk15ctypeIcE2idE},
    {"_ZNSt9exceptionD2Ev", (void*)&_ZNSt9exceptionD2Ev},
    {"_ZTISt9exception", (void*)&_ZTISt9exception},
    {"_ZTVN10__cxxabiv117__class_type_infoE", (void*)&_ZTVN10__cxxabiv117__class_type_infoE},
    {"_ZTVN10__cxxabiv119__pointer_type_infoE", (void*)&_ZTVN10__cxxabiv119__pointer_type_infoE},
    {"_ZTVN10__cxxabiv120__function_type_infoE", (void*)&_ZTVN10__cxxabiv120__function_type_infoE},
    {"_ZTVN10__cxxabiv120__si_class_type_infoE", (void*)&_ZTVN10__cxxabiv120__si_class_type_infoE},

    {NULL, NULL}
};

void *resolve_symbol(const char *name) {
    for (int i = 0; g_symbol_map[i].name != NULL; i++) {
        if (strcmp(g_symbol_map[i].name, name) == 0) {
            return g_symbol_map[i].address;
        }
    }

    void *sys_sym = dlsym(RTLD_DEFAULT, name);
    if (sys_sym) {
        return sys_sym;
    }

    printf("[ELF Loader] HINWEIS: Unbekanntes Symbol '%s' -> Nutze dynamischen Dummy Stub\n", name);
    return (void*)&custom_cxa_atexit;
}

// ============================================================================
// MAIN EMULATOR LOADER INIZIALISIERUNG
// ============================================================================

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("[NFS Loader] Starte Custom In-Memory ELF Loader...\n");

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

    void *so_handle = dlopen("./libNFSMW.so", RTLD_NOW | RTLD_GLOBAL);
    if (!so_handle) {
        printf("[NFS Loader] dlopen Fehler: %s\n", dlerror());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("[NFS Loader] libNFSMW.so erfolgreich geladen.\n");

    // Startpunkt des Spiels aufrufen
    jint (*JNI_OnLoad)(void *vm, void *reserved) = (jint (*)(void *, void *))dlsym(so_handle, "JNI_OnLoad");
    if (JNI_OnLoad) {
        printf("[NFS Loader] Führe JNI_OnLoad aus...\n");
        JNI_OnLoad(NULL, NULL);
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
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
