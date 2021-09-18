cmake_minimum_required(VERSION 3.7)
#set (LIBHERMIT_ROOT_DIR "/home/teawater/libhermit/")
include(${LIBHERMIT_ROOT_DIR}/cmake/HermitCore-Application.cmake)

project(libhermit-kata-agent C)

set (WAMR_BUILD_LIBC_BUILTIN 1)
set (WAMR_BUILD_LIBC_WASI 1)

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

add_executable(kata-agent main.c wamr.c ${UNCOMMON_SHARED_SOURCE})
target_compile_options(kata-agent PRIVATE -g)

target_link_libraries (kata-agent vmlib ${LLVM_AVAILABLE_LIBS} ${UV_A_LIBS} -lpthread)
