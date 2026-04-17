cmake_minimum_required(VERSION 3.16)
project(mod_a C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(mod_a STATIC
    module_mod_a__foo_real____0__26516395776.c
)

target_include_directories(mod_a PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "/Users/boyu/code/lfortran/src/libasr/runtime"
)
target_link_directories(mod_a PRIVATE
    "/Users/boyu/code/lfortran/build/src/bin/../runtime"
)
target_link_libraries(mod_a PRIVATE lfortran_runtime m)
target_link_options(mod_a PRIVATE "-Wl,-rpath,/Users/boyu/code/lfortran/build/src/bin/../runtime")
if(APPLE)
    target_link_libraries(mod_a PRIVATE "-framework Accelerate")
endif()
