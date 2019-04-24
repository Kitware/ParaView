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

# TODO: Restore Cray ATP support via a new module.
# TODO: Support static plugins.
# TODO: Link pvserver and friends to ParaView::PythonInitializer for static builds?

# Set up rpaths
set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
if (UNIX AND NOT APPLE)
  file(RELATIVE_PATH _paraview_client_relpath
    "/prefix/${CMAKE_INSTALL_BINDIR}"
    "/prefix/${CMAKE_INSTALL_LIBDIR}")
  set(_paraview_client_origin_rpath
    "$ORIGIN/${_paraview_client_relpath}")

  list(APPEND CMAKE_INSTALL_RPATH
    "${_paraview_client_origin_rpath}")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/ParaViewCommandLineExecutables.cmake")

set(paraview_tools)
foreach (exe IN ITEMS pvserver pvdataserver pvrenderserver)
  paraview_add_executable("${exe}" "${exe}.cxx")
  list(APPEND paraview_tools
    "${exe}")
endforeach ()

if (PARAVIEW_ENABLE_PYTHON)
  foreach (exe IN ITEMS pvbatch pvpython)
    paraview_add_executable("${exe}" "${exe}.cxx")
    target_link_libraries("${exe}"
      PRIVATE
        VTK::PythonInterpreter)
    list(APPEND paraview_tools
      "${exe}")
  endforeach ()
endif ()

if (PARAVIEW_BUILD_QT_GUI)
  list(APPEND paraview_tools
    paraview)
endif ()

option(PARAVIEW_BUILD_MESA_LAUNCHER "Build a launcher for using Mesa binaries" OFF)
mark_as_advanced(PARAVIEW_BUILD_MESA_LAUNCHER)
if (PARAVIEW_BUILD_MESA_LAUNCHER)
  set(PARAVIEW_MESA_LIBDIR "<LIBDIR>/mesa"
    CACHE STRING "The directory containing the Mesa libraries (<LIBDIR> is replaced with the runtime library install location)")
  if (WIN32)
    set(_paraview_mesa_replacement "${CMAKE_INSTALL_BINDIR}")
  else ()
    set(_paraview_mesa_replacement "${CMAKE_INSTALL_LIBDIR}")
  endif ()
  string(REPLACE "<LIBDIR>" "${_paraview_mesa_replacement}" _paraview_mesa_libdir "${PARAVIEW_MESA_LIBDIR}")
  file(RELATIVE_PATH _paraview_mesa_relpath
    "/prefix/${CMAKE_INSTALL_BINDIR}"
    "/prefix/${_paraview_mesa_libdir}")

  add_executable(paraview-mesa
    paraview_mesa.cxx)
  add_executable(ParaView::paraview-mesa ALIAS paraview-mesa)
  target_link_libraries(paraview-mesa
    PRIVATE
      VTK::vtksys)
  target_compile_definitions(paraview-mesa
    PRIVATE
      "MESA_RELATIVE_LIBDIR=\"${_paraview_mesa_relpath}\"")
  foreach (paraview_tool IN LISTS paraview_tools)
    target_compile_definitions(paraview-mesa
      PRIVATE
        "HAVE_${paraview_tool}")
  endforeach ()
  install(
    TARGETS     "paraview-mesa"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT   runtime
    EXPORT      ParaView)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/paraview-config"
  "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/paraview-config"
  COPYONLY)
install(
  PROGRAMS    "${CMAKE_CURRENT_SOURCE_DIR}/paraview-config"
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT   development)
