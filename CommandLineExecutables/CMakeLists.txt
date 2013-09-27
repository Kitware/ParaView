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
if (NOT TARGET vtkPVServerManagerApplication)
  message(STATUS "vtkPVServerManagerApplication module is not enabled. "
    "Command line executables will not be built.")
  return()
endif()

vtk_module_dep_includes(vtkPVServerManagerApplication)
include_directories(${vtkPVServerManagerApplication_DEPENDS_INCLUDE_DIRS}
                    ${vtkPVServerManagerApplication_INCLUDE_DIRS})

set(STATIC_LINK_PLUGINS)
if (NOT BUILD_SHARED_LIBS)
  set(STATIC_LINK_PLUGINS ${PARAVIEW_PLUGINLIST})
endif()

foreach (name pvserver pvdataserver pvrenderserver)
  pv_add_executable(${name} ${name}.cxx)
  target_link_libraries(${name}
    vtkPVServerManagerApplication
    ${STATIC_LINK_PLUGINS})
endforeach()

if (PARAVIEW_ENABLE_PYTHON)
  foreach (name pvbatch pvpython)
    pv_add_executable(${name} ${name}.cxx)
    target_link_libraries(${name}
      vtkPVServerManagerApplication
      vtkUtilitiesPythonInitializer
      ${VTK_PYTHON_LIBRARIES}
      ${STATIC_LINK_PLUGINS})
  endforeach()

  # add python module dependencies to the server-executables for static builds.
  foreach (name pvserver pvdataserver pvrenderserver)
    target_link_libraries(${name}
        vtkUtilitiesPythonInitializer)
  endforeach()

endif()
