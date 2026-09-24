#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <elf.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#define JNI_FALSE 0
#define JNI_TRUE  1
#define JNI_OK    0

typedef uint8_t jboolean;
typedef int32_t jint;
typedef void*   jobject;

// ---------------------------------------------------------------------------
// JNI DUMMY ENVIRONMENT
// ---------------------------------------------------------------------------
struct JNINativeInterface_ {
    void* reserved0; void* reserved1; void* reserved2; void* reserved3;
    jint (*GetVersion)(void* env);
};

struct JNIInvokeInterface_ {
    void* reserved0; void* reserved1; void* reserved2;
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

// ---------------------------------------------------------------------------
// STUBS & HOOKS
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

    void AndroidBitmap_getInfo() {}
    void AndroidBitmap_lockPixels() {}
    void AndroidBitmap_unlockPixels() {}

    void AAssetManager_open() {}
    void AAsset_read() {}
    void AAsset_close() {}

    jboolean Java_com_ea_games_nfs13_1row_NFS_isGamepadConnected(void* env, jobject thiz) {
        return JNI_TRUE;
    }

    jint Java_com_ea_games_nfs13_1row_NFS_getGamepadType(void* env, jobject thiz) {
        return 1;
    }

} // extern "C"

static void dummy_function() {}

// ---------------------------------------------------------------------------
// SYMBOL MAPPER TABLE
// ---------------------------------------------------------------------------
typedef struct {
    const char* name;
    void* func;
} SymbolMap;

static SymbolMap g_symbol_map[] = {
    // Android Stubs
    { "__android_log_print", (void*)__android_log_print },
    { "__android_log_write", (void*)__android_log_write },
    { "__android_log_assert", (void*)__android_log_assert },
    { "__android_log_buf_print", (void*)__android_log_buf_print },
    { "__android_log_vprint", (void*)__android_log_vprint },
    { "AndroidBitmap_getInfo", (void*)AndroidBitmap_getInfo },
    { "AndroidBitmap_lockPixels", (void*)AndroidBitmap_lockPixels },
    { "AndroidBitmap_unlockPixels", (void*)AndroidBitmap_unlockPixels },
    { "AAssetManager_open", (void*)AAssetManager_open },
    { "AAsset_read", (void*)AAsset_read },
    { "AAsset_close", (void*)AAsset_close },

    // Standard C / Libc
    { "malloc", (void*)malloc },
    { "calloc", (void*)calloc },
    { "realloc", (void*)realloc },
    { "free", (void*)free },
    { "memcpy", (void*)memcpy },
    { "memset", (void*)memset },
    { "memmove", (void*)memmove },
    { "strcpy", (void*)strcpy },
    { "strncpy", (void*)strncpy },
    { "strcat", (void*)strcat },
    { "strcmp", (void*)strcmp },
    { "strncmp", (void*)strncmp },
    { "strlen", (void*)strlen },
    { "strstr", (void*)(char*(*)(char*, const char*))strstr },
    { "strchr", (void*)(char*(*)(char*, int))strchr },
    { "strrchr", (void*)(char*(*)(char*, int))strrchr },
    { "sprintf", (void*)sprintf },
    { "snprintf", (void*)snprintf },
    { "vsprintf", (void*)vsprintf },
    { "vsnprintf", (void*)vsnprintf },
    { "sscanf", (void*)sscanf },
    { "printf", (void*)printf },
    { "fopen", (void*)fopen },
    { "fclose", (void*)fclose },
    { "fread", (void*)fread },
    { "fwrite", (void*)fwrite },
    { "fflush", (void*)fflush },
    { "fseek", (void*)fseek },
    { "ftell", (void*)ftell },
    { "getenv", (void*)getenv },
    { "exit", (void*)exit },
    { "abort", (void*)abort },
    { "qsort", (void*)qsort },
    { "bsearch", (void*)bsearch },
    { "clock_gettime", (void*)clock_gettime },
    { "gettimeofday", (void*)gettimeofday },
    { "time", (void*)time },

    // Math (mit expliziter Typsignatur gegen Overload-Fehler)
    { "sin", (void*)(double(*)(double))sin },
    { "cos", (void*)(double(*)(double))cos },
    { "tan", (void*)(double(*)(double))tan },
    { "asin", (void*)(double(*)(double))asin },
    { "acos", (void*)(double(*)(double))acos },
    { "atan", (void*)(double(*)(double))atan },
    { "atan2", (void*)(double(*)(double, double))atan2 },
    { "sqrt", (void*)(double(*)(double))sqrt },
    { "pow", (void*)(double(*)(double, double))pow },
    { "exp", (void*)(double(*)(double))exp },
    { "log", (void*)(double(*)(double))log },
    { "log10", (void*)(double(*)(double))log10 },
    { "floor", (void*)(double(*)(double))floor },
    { "ceil", (void*)(double(*)(double))ceil },
    { "fabs", (void*)(double(*)(double))fabs },
    { "fmod", (void*)(double(*)(double, double))fmod },
    { "sinf", (void*)(float(*)(float))sinf },
    { "cosf", (void*)(float(*)(float))cosf },
    { "sqrtf", (void*)(float(*)(float))sqrtf },
    { "powf", (void*)(float(*)(float, float))powf },
    { "floorf", (void*)(float(*)(float))floorf },
    { "ceilf", (void*)(float(*)(float))ceilf },
    { "fabsf", (void*)(float(*)(float))fabsf },
    { "fmodf", (void*)(float(*)(float, float))fmodf },
    { "atan2f", (void*)(float(*)(float, float))atan2f },

    // Pthreads
    { "pthread_create", (void*)pthread_create },
    { "pthread_join", (void*)pthread_join },
    { "pthread_detach", (void*)pthread_detach },
    { "pthread_self", (void*)pthread_self },
    { "pthread_mutex_init", (void*)pthread_mutex_init },
    { "pthread_mutex_destroy", (void*)pthread_mutex_destroy },
    { "pthread_mutex_lock", (void*)pthread_mutex_lock },
    { "pthread_mutex_unlock", (void*)pthread_mutex_unlock },
    { "pthread_cond_init", (void*)pthread_cond_init },
    { "pthread_cond_destroy", (void*)pthread_cond_destroy },
    { "pthread_cond_signal", (void*)pthread_cond_signal },
    { "pthread_cond_broadcast", (void*)pthread_cond_broadcast },
    { "pthread_cond_wait", (void*)pthread_cond_wait },
    { "pthread_key_create", (void*)pthread_key_create },
    { "pthread_key_delete", (void*)pthread_key_delete },
    { "pthread_getspecific", (void*)pthread_getspecific },
    { "pthread_setspecific", (void*)pthread_setspecific },
    { "pthread_once", (void*)pthread_once },

    // OpenGL ES 2.0
    { "glActiveTexture", (void*)glActiveTexture },
    { "glBindBuffer", (void*)glBindBuffer },
    { "glBindTexture", (void*)glBindTexture },
    { "glBlendFunc", (void*)glBlendFunc },
    { "glClear", (void*)glClear },
    { "glClearColor", (void*)glClearColor },
    { "glClearDepthf", (void*)glClearDepthf },
    { "glClearStencil", (void*)glClearStencil },
    { "glCompressedTexImage2D", (void*)glCompressedTexImage2D },
    { "glCreateProgram", (void*)glCreateProgram },
    { "glCreateShader", (void*)glCreateShader },
    { "glDeleteBuffers", (void*)glDeleteBuffers },
    { "glDeleteProgram", (void*)glDeleteProgram },
    { "glDeleteShader", (void*)glDeleteShader },
    { "glDeleteTextures", (void*)glDeleteTextures },
    { "glDepthFunc", (void*)glDepthFunc },
    { "glDepthMask", (void*)glDepthMask },
    { "glDisable", (void*)glDisable },
    { "glDisableVertexAttribArray", (void*)glDisableVertexAttribArray },
    { "glDrawArrays", (void*)glDrawArrays },
    { "glDrawElements", (void*)glDrawElements },
    { "glEnable", (void*)glEnable },
    { "glEnableVertexAttribArray", (void*)glEnableVertexAttribArray },
    { "glGenBuffers", (void*)glGenBuffers },
    { "glGenTextures", (void*)glGenTextures },
    { "glGetAttribLocation", (void*)glGetAttribLocation },
    { "glGetError", (void*)glGetError },
    { "glGetIntegerv", (void*)glGetIntegerv },
    { "glGetString", (void*)glGetString },
    { "glGetUniformLocation", (void*)glGetUniformLocation },
    { "glLineWidth", (void*)glLineWidth },
    { "glPixelStorei", (void*)glPixelStorei },
    { "glShaderSource", (void*)glShaderSource },
    { "glCompileShader", (void*)glCompileShader },
    { "glGetShaderiv", (void*)glGetShaderiv },
    { "glGetShaderInfoLog", (void*)glGetShaderInfoLog },
    { "glAttachShader", (void*)glAttachShader },
    { "glLinkProgram", (void*)glLinkProgram },
    { "glGetProgramiv", (void*)glGetProgramiv },
    { "glGetProgramInfoLog", (void*)glGetProgramInfoLog },
    { "glUseProgram", (void*)glUseProgram },
    { "glUniform1f", (void*)glUniform1f },
    { "glUniform1i", (void*)glUniform1i },
    { "glUniform2f", (void*)glUniform2f },
    { "glUniform3f", (void*)glUniform3f },
    { "glUniform4f", (void*)glUniform4f },
    { "glUniformMatrix4fv", (void*)glUniformMatrix4fv },
    { "glVertexAttribPointer", (void*)glVertexAttribPointer },
    { "glViewport", (void*)glViewport },
    { "glScissor", (void*)glScissor },

    // Gamepad JNI Spoofing
    { "Java_com_ea_games_nfs13_1row_NFS_isGamepadConnected", (void*)Java_com_ea_games_nfs13_1row_NFS_isGamepadConnected },
    { "Java_com_ea_games_nfs13_1row_NFS_getGamepadType", (void*)Java_com_ea_games_nfs13_1row_NFS_getGamepadType },

    { NULL, NULL }
};

static void* resolve_import_symbol(const char* name) {
    if (!name || name[0] == '\0') return (void*)dummy_function;

    for (size_t i = 0; g_symbol_map[i].name != NULL; i++) {
        if (strcmp(g_symbol_map[i].name, name) == 0) {
            return g_symbol_map[i].func;
        }
    }

    printf("[ELF Loader] WARNUNG: Unbekanntes Symbol '%s' -> Nutze Dummy Stub\n", name);
    return (void*)dummy_function;
}

// ---------------------------------------------------------------------------
// IN-MEMORY ELF RELOCATOR (ARMv7)
// ---------------------------------------------------------------------------
typedef struct {
    uintptr_t base;
    size_t size;
    Elf32_Sym* symtab;
    const char* strtab;
    size_t num_syms;
} SoModule;

static int so_load(const char* filename, SoModule* mod) {
    printf("[ELF Loader] Oeffne %s direkt als Binaerdatei...\n", filename);

    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("[ELF Loader] FEHLER: Datei %s konnte nicht geoeffnet werden!\n", filename);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* file_buf = (uint8_t*)malloc(file_size);
    if (!file_buf) {
        fclose(f);
        return -1;
    }

    if (fread(file_buf, 1, file_size, f) != file_size) {
        printf("[ELF Loader] FEHLER: Lesen der Datei fehlgeschlagen!\n");
        free(file_buf);
        fclose(f);
        return -1;
    }
    fclose(f);

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)file_buf;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("[ELF Loader] FEHLER: Ungueltiger ELF Magic Header!\n");
        free(file_buf);
        return -1;
    }

    Elf32_Phdr* phdr = (Elf32_Phdr*)(file_buf + ehdr->e_phoff);
    uintptr_t min_vaddr = (uintptr_t)-1;
    uintptr_t max_vaddr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (phdr[i].p_vaddr < min_vaddr) min_vaddr = phdr[i].p_vaddr;
            if (phdr[i].p_vaddr + phdr[i].p_memsz > max_vaddr) {
                max_vaddr = phdr[i].p_vaddr + phdr[i].p_memsz;
            }
        }
    }

    size_t total_size = max_vaddr - min_vaddr;
    printf("[ELF Loader] Allokiere %zu KB Arbeitsspeicher per mmap...\n", total_size / 1024);

    uint8_t* base_addr = (uint8_t*)mmap(NULL, total_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (base_addr == MAP_FAILED) {
        printf("[ELF Loader] FEHLER: mmap Allokation fehlgeschlagen!\n");
        free(file_buf);
        return -1;
    }

    memset(base_addr, 0, total_size);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            memcpy(base_addr + phdr[i].p_vaddr, file_buf + phdr[i].p_offset, phdr[i].p_filesz);
        }
    }

    Elf32_Dyn* dyn = NULL;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            dyn = (Elf32_Dyn*)(base_addr + phdr[i].p_vaddr);
            break;
        }
    }

    Elf32_Sym* symtab = NULL;
    const char* strtab = NULL;
    Elf32_Rel* rel = NULL;
    size_t rel_sz = 0;
    Elf32_Rel* plt_rel = NULL;
    size_t plt_rel_sz = 0;

    if (dyn) {
        for (Elf32_Dyn* d = dyn; d->d_tag != DT_NULL; d++) {
            switch (d->d_tag) {
                case DT_SYMTAB:   symtab = (Elf32_Sym*)(base_addr + d->d_un.d_ptr); break;
                case DT_STRTAB:   strtab = (const char*)(base_addr + d->d_un.d_ptr); break;
                case DT_REL:      rel = (Elf32_Rel*)(base_addr + d->d_un.d_ptr); break;
                case DT_RELSZ:    rel_sz = d->d_un.d_val; break;
                case DT_JMPREL:   plt_rel = (Elf32_Rel*)(base_addr + d->d_un.d_ptr); break;
                case DT_PLTRELSZ: plt_rel_sz = d->d_un.d_val; break;
            }
        }
    }

    mod->base = (uintptr_t)base_addr;
    mod->size = total_size;
    mod->symtab = symtab;
    mod->strtab = strtab;

    printf("[ELF Loader] Luese ARM32 Relocations auf...\n");

    auto process_relocations = [&](Elf32_Rel* rel_table, size_t size_bytes) {
        if (!rel_table) return;
        size_t count = size_bytes / sizeof(Elf32_Rel);
        for (size_t i = 0; i < count; i++) {
            Elf32_Addr* target = (Elf32_Addr*)(base_addr + rel_table[i].r_offset);
            uint32_t type = ELF32_R_TYPE(rel_table[i].r_info);
            uint32_t sym_idx = ELF32_R_SYM(rel_table[i].r_info);

            const char* sym_name = NULL;
            uintptr_t sym_addr = 0;

            if (sym_idx != 0 && symtab && strtab) {
                sym_name = strtab + symtab[sym_idx].st_name;
                sym_addr = (uintptr_t)resolve_import_symbol(sym_name);
            }

            switch (type) {
                case 23: // R_ARM_RELATIVE
                    *target += (Elf32_Addr)base_addr;
                    break;
                case 2:  // R_ARM_ABS32
                    *target += (Elf32_Addr)sym_addr;
                    break;
                case 21: // R_ARM_GLOB_DAT
                case 22: // R_ARM_JUMP_SLOT
                    *target = (Elf32_Addr)sym_addr;
                    break;
                default:
                    break;
            }
        }
    };

    process_relocations(rel, rel_sz);
    process_relocations(plt_rel, plt_rel_sz);

    free(file_buf);
    printf("[ELF Loader] libNFSMW.so erfolgreich im Speicher platziert und gelinkt!\n");
    return 0;
}

static void* so_symbol_find(SoModule* mod, const char* name) {
    if (!mod || !mod->symtab || !mod->strtab) return NULL;

    for (size_t i = 0; i < 20000; i++) {
        uint32_t name_offset = mod->symtab[i].st_name;
        if (name_offset > 0x100000) break;
        const char* sym_name = mod->strtab + name_offset;
        if (strcmp(sym_name, name) == 0) {
            return (void*)(mod->base + mod->symtab[i].st_value);
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// MAIN EXECUTION
// ---------------------------------------------------------------------------
typedef jint (*JNI_OnLoad_t)(void* vm, void* reserved);
typedef void (*NativeInit_t)(int width, int height);
typedef void (*NativeRender_t)();
typedef void (*NativeTouchEvent_t)(int action, float x, float y, int pointer_id);

int main(int argc, char* argv[]) {
    printf("[NFS Loader] Starte Custom In-Memory ELF Loader...\n");

    if (chdir("/roms/ports/nfs_mw") != 0) {
        chdir("/roms2/ports/nfs_mw");
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[NFS Loader] FEHLER: SDL_Init fehlgeschlagen: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("Need for Speed: Most Wanted",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640, 480,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if (!window) {
        printf("[NFS Loader] FEHLER: Fenster konnte nicht erstellt werden!\n");
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    SoModule game_module;
    memset(&game_module, 0, sizeof(SoModule));

    if (so_load("./libNFSMW.so", &game_module) < 0) {
        printf("[NFS Loader] FEHLER beim Laden der ELF-Datei!\n");
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    JNI_OnLoad_t JNI_OnLoad_fn = (JNI_OnLoad_t)so_symbol_find(&game_module, "JNI_OnLoad");
    if (JNI_OnLoad_fn) {
        printf("[NFS Loader] Führe JNI_OnLoad mit Fake-JavaVM aus...\n");
        struct JNIInvokeInterface_* fake_vm_ptr = &dummy_vm_vtbl;
        JNI_OnLoad_fn(&fake_vm_ptr, NULL);
    }

    NativeInit_t NativeInit = (NativeInit_t)so_symbol_find(&game_module, "nativeInit");
    NativeRender_t NativeRender = (NativeRender_t)so_symbol_find(&game_module, "nativeRender");

    if (NativeInit) {
        printf("[NFS Loader] Initialisiere Engine (640x480)...\n");
        NativeInit(640, 480);
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        if (NativeRender) {
            NativeRender();
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
