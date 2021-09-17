cmake_minimum_required(VERSION 3.7)
set (LIBHERMIT_ROOT_DIR "/home/teawater/libhermit/")
include(${LIBHERMIT_ROOT_DIR}/cmake/HermitCore-Application.cmake)

project(libhermit-kata-agent C)

if("${TARGET_ARCH}" STREQUAL "x86_64-hermit")
add_executable(kata-agent main.c)
target_compile_options(kata-agent PRIVATE -g)
endif()
