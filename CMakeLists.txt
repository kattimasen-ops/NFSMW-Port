cmake_minimum_required(VERSION 3.10)
project(nfs_mw_loader C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suche nach Abhängigkeiten
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

find_library(GLESV2_LIB GLESv2 REQUIRED)
find_library(DL_LIB dl REQUIRED)
find_library(PTHREAD_LIB pthread REQUIRED)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Target definieren
add_executable(nfs_mw_loader src/main.cpp)

# Verlinkung der Bibliotheken
target_link_libraries(nfs_mw_loader
    ${SDL2_LIBRARIES}
    ${GLESV2_LIB}
    ${DL_LIB}
    ${PTHREAD_LIB}
)