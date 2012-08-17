#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
SET(PV_TEST_USE_RANDOM_PORTS ON CACHE BOOL
  "Use random port numbers when testing client-server configurations.")
MARK_AS_ADVANCED(PV_TEST_USE_RANDOM_PORTS)

SET(PV_TEST_INIT_COMMAND "" CACHE STRING
  "Command to run before a test begins. Multiple commands are separated by ';'.")
MARK_AS_ADVANCED(PV_TEST_INIT_COMMAND)

SET(PV_TEST_CLEAN_COMMAND "" CACHE STRING
  "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
MARK_AS_ADVANCED(PV_TEST_CLEAN_COMMAND)

SET (PV_TEST_CLIENT "localhost" CACHE STRING
  "Node which serves as the client node, used to connect from the server side in reverse connection mode.")
MARK_AS_ADVANCED(PV_TEST_CLIENT)

# Add the test driver executable. This is not done in the above loop to avoid
# adding dependencies ${LIBS} since we only need vtksys for this executable.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkSMTestDriverConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTestDriverConfig.h @ONLY ESCAPE_QUOTES)

include(vtkForwardingExecutable)

vtk_module_impl()
vtk_module_export("")
vtk_add_executable_with_forwarding(out_exe_suffix
  smTestDriver
  vtkSMTestDriver.cxx
  vtkSMTestDriver.h)
set_property(GLOBAL APPEND PROPERTY VTK_TARGETS smTestDriver)

target_link_libraries(smTestDriver
  vtksys
  vtkCommonCore)
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  pv_executable_install(smTestDriver "${out_exe_suffix}")
endif()
