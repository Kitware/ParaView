#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
set(SQTK_CUDA OFF CACHE BOOL "Enable CUDA accelerated filters.")
mark_as_advanced(SQTK_CUDA)
if (SQTK_CUDA)
  find_package(CUDA)
endif()
if (SQTK_CUDA AND CUDA_FOUND)
  mark_as_advanced(
    CUDA_BUILD_CUBIN
    CUDA_BUILD_EMULATION
    CUDA_HOST_COMPILER
    CUDA_SDK_ROOT_DIR
    CUDA_SEPARABLE_COMPILATION
    CUDA_TOOLKIT_ROOT_DIR
    CUDA_VERBOSE_BUILD
    )
  message(STATUS "SQTK Including CUDA accelerated filters.")
  message(STATUS "SQTK Cuda ${CUDA_VERSION_STRING} found.")
  #set(CUDA_NVCC_FLAGS "-arch=sm_20;--compiler-options;-fPIC" CACHE STRING "nvcc flags")
  #set(CUDA_NVCC_DEBUG_FLAGS "-g;-G;" CACHE STRING "nvcc debug flags")
  #set(CUDA_NVCC_RELEASE_FLAGS "-O3" CACHE STRING "nvcc release flags")
  #message(STATUS "SQTK CUDA_NVCC_FLAGS=${CUDA_NVCC_FLAGS}")
  #message(STATUS "SQTK CUDA_NVCC_DEBUG_FLAGS=${CUDA_NVCC_DEBUG_FLAGS}")
  #message(STATUS "SQTK CUDA_NVCC_RELEASE_FLAGS=${CUDA_NVCC_FLAGS}")
endif()
