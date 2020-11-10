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
set(classes
  vtkInSituPipeline
  vtkInSituPipelineIO
  vtkInSituPipelinePython
  vtkInSituInitializationHelper)

vtk_module_add_module(ParaView::InSitu
  CLASSES ${classes})
vtk_module_client_server_exclude()
# vtk_module_link(ParaView::InSitu
#   PRIVATE
#     # for plugins, in case of static builds.
#     ParaView::paraview_plugins)

# catalyst library is always built SHARED.
# this is necessary so that it be loaded in simulations built with
# stub catalyst build.
add_library(catalyst SHARED
  ParaViewCatalyst.cxx
  vtkCatalystBlueprint.cxx)

target_link_libraries(catalyst
  PRIVATE
    ParaView::InSitu
    ParaView::VTKExtensionsCore
    ParaView::VTKExtensionsConduit
    ParaView::RemotingServerManager)

if (TARGET VTK::ParallelMPI)
  target_link_libraries(catalyst
    PRIVATE
      VTK::ParallelMPI)
  target_compile_definitions(catalyst
    PRIVATE
      VTK_MODULE_ENABLE_VTK_ParallelMPI=1)
else()
  target_compile_definitions(catalyst
    PRIVATE
      VTK_MODULE_ENABLE_VTK_ParallelMPI=0)
endif()

if (TARGET VTK::WrappingPythonCore AND VTK_PYTHON_OPTIONAL_LINK)
  # if Python is enabled and optionally linking with Python,
  # we need to link against Python non-optionally here.
  find_package(Python3 COMPONENTS Development)
  target_link_libraries(catalyst
    PRIVATE
      Python3::Python)
endif()

include("${_catalyst_macros_path}/catalyst-macros.cmake")
_vtk_module_install(catalyst)
_vtk_module_apply_properties(catalyst)


# Update install locations
catalyst_library(
  TARGET catalyst
  LIBRARY_DESTINATION "${_vtk_build_LIBRARY_DESTINATION}"
  INSTALL_EXPORT_DEFAULT)
