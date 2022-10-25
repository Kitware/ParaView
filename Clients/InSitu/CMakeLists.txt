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
  vtkInSituInitializationHelper
  vtkInSituPipeline
  vtkInSituPipelineIO
  vtkInSituPipelinePython
  )

vtk_module_add_module(ParaView::InSitu
  CLASSES ${classes})
vtk_module_client_server_exclude()

if (PARAVIEW_ENABLE_CATALYST)
  if (NOT TARGET VTK::IOCatalystConduit)
    message(FATAL_ERROR "Cannot build Catalyst implementation without VTK::IOCatalystConduit module enabled")
  endif ()

  vtk_module_find_package(PACKAGE catalyst VERSION 2.0 COMPONENTS SDK)
  set_property(GLOBAL PROPERTY vtk_catalyst_dir_ClientsInSitu "${catalyst_DIR}")

  add_subdirectory(catalyst)

  vtk_module_link(ParaView::InSitu
    PRIVATE
      catalyst::catalyst)
endif ()
