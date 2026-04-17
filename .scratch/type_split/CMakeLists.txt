cmake_minimum_required(VERSION 3.16)
project(type C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(type STATIC
    module_tblite_wavefunction_type__get_alpha_beta_occupation__52098003192.c
    module_tblite_wavefunction_type__get_density_matrix_real____0_real_______1_real_______2__26416376440.c
    module_tblite_wavefunction_type__new_wavefunction__52097998608.c
    global_procedures.c
)

target_include_directories(type PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "/Users/boyu/code/lfortran/src/libasr/runtime"
)
target_link_directories(type PRIVATE
    "/Users/boyu/code/lfortran/build/src/bin/../runtime"
)
target_link_libraries(type PRIVATE lfortran_runtime m)
target_link_options(type PRIVATE "-Wl,-rpath,/Users/boyu/code/lfortran/build/src/bin/../runtime")
if(APPLE)
    target_link_libraries(type PRIVATE "-framework Accelerate")
endif()
