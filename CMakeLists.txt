cmake_minimum_required(VERSION 3.7)

project(texture_zoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sanitize w/o xcode
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined")
# set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined")

set(SOKOL_DIR CACHE FILEPATH "")
set(SOKOL_TOOLS_BIN CACHE FILEPATH "Path to the sokol tools binaries, must contain sokol-shdc.")
set(STB_HEADERS CACHE FILEPATH "")

set(SOKOL_HEADERS
    ${SOKOL_DIR}/sokol_gfx.h
    ${SOKOL_DIR}/sokol_app.h
    ${SOKOL_DIR}/sokol_time.h
    # ${SOKOL_DIR}/sokol_imgui.h
    ${SOKOL_DIR}/sokol_glue.h
)

include_directories(${STB_HEADERS} ${SOKOL_DIR})

set(SRC
    tex_zoom.c
)

# shader compilation
add_custom_command(
  OUTPUT shader.glsl.h
  COMMAND ${SOKOL_TOOLS_BIN}/sokol-shdc
          -i ${CMAKE_CURRENT_SOURCE_DIR}/shader.glsl 
          -o ${CMAKE_CURRENT_SOURCE_DIR}/shader.glsl.h 
          -l metal_macos:glsl100 # glsl100 is for WebGL 1
  VERBATIM
)

add_custom_target(shaders
    SOURCES shader.glsl
)

# Linux -pthread shenanigans
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_library(sokol STATIC sokol_lib/sokol.m ${SOKOL_HEADERS} ${STB_HEADERS})
    target_link_libraries(sokol PUBLIC
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal")
else()
    add_library(sokol STATIC sokol_lib/sokol.c ${SOKOL_HEADERS} ${STB_HEADERS})
    if (CMAKE_SYSTEM_NAME STREQUAL Linux)
        target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL dl m)
        target_link_libraries(sokol PUBLIC Threads::Threads)
    endif()
endif()
target_include_directories(sokol INTERFACE sokol)

set(HEADER
    shader.glsl.h
)

add_executable(tex_zoom ${SRC} ${HEADER})
target_link_libraries(tex_zoom sokol)