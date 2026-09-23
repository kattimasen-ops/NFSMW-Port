cmake_minimum_required(VERSION 3.10)
project(nfs_mw_loader C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PkgConfig für SDL2 und GLESv2 nutzen
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLES2 glesv2)

# Offizielle CMake-Unterstützung für pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Fallback für GLESv2, falls pkg-config die Bibliothek nicht direkt auflöst
if(NOT GLES2_FOUND)
    find_library(GLESV2_LIB NAMES GLESv2 glesv2 PATHS /usr/lib/arm-linux-gnueabihf /usr/lib)
else()
    set(GLESV2_LIB ${GLES2_LIBRARIES})
endif()

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${GLES2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Target definieren
add_executable(nfs_mw_loader src/main.cpp)

# Verlinkung mit sauberen CMake-Targets/Variablen
target_link_libraries(nfs_mw_loader
    PRIVATE
    ${SDL2_LIBRARIES}
    ${GLESV2_LIB}
    ${CMAKE_DL_LIBS}
    Threads::Threads
)
