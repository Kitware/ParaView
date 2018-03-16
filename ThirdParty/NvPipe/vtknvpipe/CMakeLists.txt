# Copyright (c) 2016-2017, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
cmake_minimum_required(VERSION 3.2)

set(NVPIPE "nvpipe")

project(${NVPIPE})

# Configuration
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")
set(project_config "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(sdkhelp "Directory where ReadMe.txt from the Video Codec SDK lives.")
option(NVIDIA_VIDEO_CODEC_SDK ${sdkhelp} "")
if("${NVIDIA_VIDEO_CODEC_SDK}" STREQUAL "OFF")
  # We include a copy of the v5 SDK header to use as a fallback.
  # But to keep our source unchanged, we always include "nvEncodeAPI.h", so
  # copy the file to that name in our local directory.
  file(COPY
    "${PROJECT_SOURCE_DIR}/nvEncodeAPI-v5.h"
    DESTINATION
    "${PROJECT_BINARY_DIR}"
  )
  # file(COPY) can't rename the file.
  file(RENAME
    "${PROJECT_BINARY_DIR}/nvEncodeAPI-v5.h"
    "${PROJECT_BINARY_DIR}/nvEncodeAPI.h"
  )
else()
  include_directories(AFTER "${NVIDIA_VIDEO_CODEC_SDK}/Samples/common/inc")
  # If we switched from NOT using an extracted SDK (above) to USING an
  # extracted SDK, we might inadvertently pick up the old file.  Make sure
  # it's been cleaned up.
  file(REMOVE "${PROJECT_BINARY_DIR}/nvEncodeAPI.h")
endif()

include_directories(BEFORE "${PROJECT_BINARY_DIR}")
include_directories(AFTER "${PROJECT_SOURCE_DIR}")

find_package(CUDA REQUIRED)

cmake_policy(SET CMP0057 NEW)
if(NOT MSVC)
  if(NOT "--std" IN_LIST CUDA_NVCC_FLAGS)
    list(APPEND CUDA_NVCC_FLAGS "--std c++11")
  endif()
endif()

list(APPEND CUDA_NVCC_FLAGS "-Wno-deprecated-gpu-targets")

# CUDA + NvPipe variables that PV does not want exposed.
mark_as_advanced(
  CUDA_NVTX_LIB
  CUDA_SDK_ROOT_DIR
  CUDA_USE_STATIC_CUDA_RUNTIME
  CUDA_rt_LIBRARY
  NVPIPE_LIBRARY_PATH
  nvpipe_DIR
)

include_directories(${PROJECT_SOURCE_DIR} ${CUDA_SDK_ROOT_DIR}/common/inc)
link_directories("${CMAKE_INSTALL_PREFIX}/lib")


# PV does not use ffmpeg backend.
set(NvPipe_FFMPEG 0)


if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(winsrc ${PROJECT_SOURCE_DIR}/winposix.c)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W2")
  include_directories($ENV{NVTOOLSEXT_PATH}/include)
  link_directories("${CUDA_TOOLKIT_ROOT_DIR}/lib/x64")
  link_directories($ENV{NVTOOLSEXT_PATH}/lib/x64)
  link_directories($ENV{CUDA_PATH}/lib/x64)
  # "nvToolsExt" naming differs on windows.
  set(nvpipe_libs ${nvpipe_libs} nvToolsExt64_1)
else()
  link_directories("${CUDA_TOOLKIT_ROOT_DIR}/lib64")
  # "dl" and "pthread" only needed on Unix platforms.
  set(nvpipe_libs ${nvpipe_libs} nvToolsExt dl pthread)
endif(MSVC)

cuda_add_library(${NVPIPE} SHARED
  ${winsrc}
  ${PROJECT_SOURCE_DIR}/convert.cu
  ${PROJECT_SOURCE_DIR}/debug.c
  ${PROJECT_SOURCE_DIR}/decode.c
  ${PROJECT_SOURCE_DIR}/encode.c
  ${PROJECT_SOURCE_DIR}/error.c
  ${PROJECT_SOURCE_DIR}/yuv.c
  ${PROJECT_SOURCE_DIR}/api.c
  SHARED
)

# x86-64 libraries always need to be PIC.
set_property(TARGET ${NVPIPE} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Linking with C for supercomputer-friendliness.
set_target_properties(${NVPIPE} PROPERTIES LINKER_LANGUAGE C)

# The direct nvEnc/cuvid backends use c99.
set_property(TARGET ${NVPIPE} PROPERTY C_STANDARD 99)
set_property(TARGET ${NVPIPE} PROPERTY CXX_STANDARD 11)

target_link_libraries(${NVPIPE} PRIVATE
  ${nvpipe_libs}
  cuda nvcuvid
)

configure_file(
    "cmake/Config.cmake.in"
    "${project_config}" @ONLY
)
set(NvPipe_DECODE 1)
set(NvPipe_ENCODE 1)
set(NvPipe_PREFIX ${CMAKE_INSTALL_PREFIX})
configure_file(
  "${PROJECT_SOURCE_DIR}/config.nvp.h.in"
  "${PROJECT_BINARY_DIR}/config.nvp.h"
)
export(TARGETS ${NVPIPE}
  FILE "${PROJECT_BINARY_DIR}/nvpipeTargets.cmake"
)
export(PACKAGE ${NVPIPE})

set_target_properties(${NVPIPE} PROPERTIES
  PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/nvpipe.h;${CMAKE_BINARY_DIR}/config.nvp.h"
)

install(TARGETS ${NVPIPE}
  EXPORT nvpipeTargets
  DESTINATION ${NvPipe_PREFIX}
  LIBRARY DESTINATION lib COMPONENT shlib
)
install(FILES
  ${PROJECT_SOURCE_DIR}/nvpipe.h
  ${PROJECT_SOURCE_DIR}/mangle_nvpipe.h
  ${PROJECT_BINARY_DIR}/config.nvp.h
  DESTINATION include
)
install(FILES "${project_config}" DESTINATION lib/cmake/${NVPIPE})
install(EXPORT nvpipeTargets DESTINATION lib/cmake/${NVPIPE} COMPONENT dev)
install(EXPORT nvpipeTargets DESTINATION lib/cmake/${NVPIPE})
