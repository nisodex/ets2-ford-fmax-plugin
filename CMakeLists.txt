cmake_minimum_required(VERSION 3.20)
project(ford_fmax_plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ford_fmax_plugin SHARED
    src/ford_fmax_plugin.cpp
)

target_compile_definitions(ford_fmax_plugin PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if (MSVC)
    target_compile_options(ford_fmax_plugin PRIVATE /W3 /O2 /GS- /sdl-)
endif()

set_target_properties(ford_fmax_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ford_fmax_plugin"
)
