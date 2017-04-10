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
set(NVPIPE "nvpipe")

project(${NVPIPE} C)

# Configuration
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")
set(project_config "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include_directories(BEFORE "${PROJECT_BINARY_DIR}")
include_directories(AFTER "${PROJECT_SOURCE_DIR}")

find_package(CUDA REQUIRED)
list(APPEND CUDA_NVCC_FLAGS "--std c++11")

include_directories(${PROJECT_SOURCE_DIR} ${CUDA_SDK_ROOT_DIR}/common/inc)
link_directories("${CUDA_TOOLKIT_ROOT_DIR}/lib64")
link_directories("${CMAKE_INSTALL_PREFIX}/lib")

# PV does not use ffmpeg backend.
set(NvPipe_FFMPEG 0)

cuda_compile_ptx(ptxprograms
	convert.cu
)

cuda_add_library(${NVPIPE} SHARED
  ${FFMPEG_SOURCES}
	${PROJECT_SOURCE_DIR}/debug.c
	${PROJECT_SOURCE_DIR}/decode.c
	${PROJECT_SOURCE_DIR}/encode.c
	${PROJECT_SOURCE_DIR}/error.c
	${PROJECT_SOURCE_DIR}/module.c
	${PROJECT_SOURCE_DIR}/yuv.c
	${PROJECT_SOURCE_DIR}/api.c
	${ptxprograms}
	SHARED
)

# x86-64 libraries always need to be PIC.
set_property(TARGET ${NVPIPE} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Linking with C for supercomputer-friendliness.
set_target_properties(${NVPIPE} PROPERTIES LINKER_LANGUAGE C)
# The direct nvEnc/cuvid backends use c99.
set_property(TARGET ${NVPIPE} PROPERTY C_STANDARD 99)

target_link_libraries(${NVPIPE} PRIVATE
  ${nvpipe_libs} cuda nvToolsExt nvcuvid dl pthread
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

install(TARGETS ${NVPIPE}
  EXPORT nvpipeTargets
  LIBRARY DESTINATION lib COMPONENT shlib
)
install(FILES
  ${PROJECT_SOURCE_DIR}/nvpipe.h
  ${PROJECT_SOURCE_DIR}/mangle_nvpipe.h
  ${PROJECT_BINARY_DIR}/config.nvp.h
  DESTINATION include
)
install(FILES ${PROJECT_BINARY_DIR}/convert.ptx DESTINATION share/nvpipe)
install(FILES "${project_config}" DESTINATION lib/cmake/${NVPIPE})
install(EXPORT nvpipeTargets DESTINATION lib/cmake/${NVPIPE} COMPONENT dev)
