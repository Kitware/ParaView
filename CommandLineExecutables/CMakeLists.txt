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
vtk_module_impl()
vtk_module_export("")

foreach (name pvserver pvdataserver pvrenderserver)
  add_executable_with_forwarding(exe_suffix ${name} ${name}.cxx)
  #vtk_target_name(${name})
  #vtk_target_label(${name})
  #vtk_target_install(${name})
  if (NOT VTK_INSTALL_NO_RUNTIME)
    pv_executable_install(${name} "${exe_suffix}")
  endif()
  target_link_libraries(${name}${exe_suffix}
    vtkPVServerManagerApplication)
endforeach()

if (PARAVIEW_ENABLE_PYTHON)
  foreach (name pvbatch pvpython)
    add_executable_with_forwarding(exe_suffix ${name} ${name}.cxx
      vtkPVPythonOptions.cxx)
    #vtk_target_name(${name})
    #vtk_target_label(${name})
    #vtk_target_install(${name})
    if (NOT VTK_INSTALL_NO_RUNTIME)
      pv_executable_install(${name} "${exe_suffix}")
    endif()
    target_link_libraries(${name}${exe_suffix}
      vtkPVServerManagerApplication
      vtkPVPythonSupport)
  endforeach()
endif()

#FIXME:MODULARIZATION -- link line for plugins when doing static builds.


#------------------------------------------------------------------------
## FIXME:MODULARIZATION -- NEED TO VERIFY THIS
## We setup the CPack scripts for packaging the server.
## This comes in handy when the Qt client is not built, or we want to package
## the server executables alone (pvpython, pvbatch as well, if python was
## enabled).
#include(ParaViewBrandingCPack)
## Install ParaView client
#build_paraview_client_cpack_config_init(
#    PACKAGE_NAME "ParaViewServers"
#    ORGANIZATION "Kitware Inc."
#    VERSION_MAJOR "${PARAVIEW_VERSION_MAJOR}"
#    VERSION_MINOR "${PARAVIEW_VERSION_MINOR}"
#    VERSION_PATCH "${PARAVIEW_VERSION_PATCH}"
#    DESCRIPTION "ParaView is a scientific visualization tool"
#    LICENSE_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
#    DESCRIPTION_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
#)
#
#SET (CPACK_INSTALL_CMAKE_PROJECTS_DRAGNDROP
#  ${CPACK_INSTALL_CMAKE_PROJECTS})
#build_paraview_client_cpack_config()
