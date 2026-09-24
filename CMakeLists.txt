cmake_minimum_required(VERSION 3.10)
project(nfs_mw_loader C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suche benötigte Pakete
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLES2 REQUIRED glesv2)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${GLES2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(nfs_mw_loader src/main.cpp)

target_link_libraries(nfs_mw_loader
    PRIVATE
    ${SDL2_LIBRARIES}
    ${GLES2_LIBRARIES}
    ${CMAKE_DL_LIBS}
    Threads::Threads
    m
)
