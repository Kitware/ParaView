# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

#
# This script tests the ParaView install tree by building the examples
#
# The script expects the following input variables:
# PARAVIEW_BINARY_DIR : Build path for ParaView (To run 'make install' here)
# PARAVIEW_INSTALL_DIR : Install path for ParaView Examples are built against
#                        (This should be CMAKE_INSTALL_PREFIX set for ParaView)
# PARAVIEW_SOURCE_DIR : ParaView source dir (For source location of Examples)
# PARAVIEW_TEST_DIR : Temporary directory for location of Examples build tree
# PARAVIEW_VERSION : ParaView version string used when creating the install tree

message (STATUS "Building Examples against ParaView install tree")
message("CTEST_FULL_OUTPUT") # Don't truncate test output.
# Remove the drive letter from `PARAVIEW_INSTALL_DIR` so we can append it to
# DESTDIR safely.
if (WIN32 AND IS_ABSOLUTE "${PARAVIEW_INSTALL_DIR}")
  string(REGEX REPLACE "^.:" "" PARAVIEW_INSTALL_DIR "${PARAVIEW_INSTALL_DIR}")
endif ()
set (paraview_prefix
  $ENV{DESTDIR}${PARAVIEW_INSTALL_DIR})
set (ParaView_DIR
  ${paraview_prefix}/${PARAVIEW_CMAKE_DESTINATION})
if (WIN32)
  set(ENV{PATH}
    "$ENV{PATH};${paraview_prefix}/${PARAVIEW_BINDIR}")
endif ()
message(STATUS "ParaView_DIR: ${ParaView_DIR}")

# Build target "INSTALL" for paraview
if (MSVC)
  set (install_tgt "INSTALL")
  set (opt "/maxcpucount")
else ()
  set (install_tgt "install")
  set (opt "-j5")
endif ()
execute_process (COMMAND ${CMAKE_COMMAND}
  --build ${PARAVIEW_BINARY_DIR}
  --target ${install_tgt}
  -- ${opt}
  WORKING_DIRECTORY ${PARAVIEW_BINARY_DIR}
  RESULT_VARIABLE irv)
if (NOT irv EQUAL 0)
  message(FATAL_ERROR "Could not build target 'install' for ParaView")
endif ()

set(generator_args)
if (CMAKE_GENERATOR)
  list(APPEND generator_args
    -G "${CMAKE_GENERATOR}")
endif ()
if (CMAKE_GENERATOR_PLATFORM)
  list(APPEND generator_args
    -A "${CMAKE_GENERATOR_PLATFORM}")
endif ()
if (CMAKE_GENERATOR_TOOLSET)
  list(APPEND generator_args
    -T "${CMAKE_GENERATOR_TOOLSET}")
endif ()

set (INSTALL_TEST_BUILD_DIR ${PARAVIEW_TEST_DIR}/Examples-bld)
if (NOT EXISTS ${INSTALL_TEST_BUILD_DIR})
  file(MAKE_DIRECTORY
    ${INSTALL_TEST_BUILD_DIR})
endif ()
execute_process (
  COMMAND ${CMAKE_COMMAND}
  ${generator_args}
  -DParaView_DIR:PATH=${ParaView_DIR}
  -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  ${PARAVIEW_SOURCE_DIR}/Examples
  WORKING_DIRECTORY ${INSTALL_TEST_BUILD_DIR}
  RESULT_VARIABLE crv)
if (NOT crv EQUAL 0)
  message(FATAL_ERROR "Configuration failed with return code ${crv}")
endif ()
execute_process (
  COMMAND ${CMAKE_COMMAND} --build ${INSTALL_TEST_BUILD_DIR} --clean-first
  WORKING_DIRECTORY ${INSTALL_TEST_BUILD_DIR}
  RESULT_VARIABLE rv)
if (NOT rv EQUAL 0)
  message(FATAL_ERROR "Build failed with return code ${rv}")
endif ()
