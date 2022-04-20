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
  vtkPEnSightGoldBinaryReader
  vtkPEnSightGoldReader
  vtkPEnSightReader
  vtkPGenericEnSightReader
  vtkPVEnSightMasterServerReader
  vtkPVEnSightMasterServerReader2
  vtkPVEnSightMasterServerTranslator)

if (NOT VTK_LEGACY_REMOVE)
  option(VTK_ENSIGHT_SPARSE_MODE "Enable \"Sparse Mode\" support for Parallel EnSight Reader (DEPRECATED)." OFF)
  mark_as_advanced(VTK_ENSIGHT_SPARSE_MODE)
endif ()

if (VTK_ENSIGHT_SPARSE_MODE)
  message(WARNING "Warning: EnSight \"Sparse Mode\" is activated (legacy behavior when the number of"
  " MPI processes > 12). This mode can leads to crashes in some cases, we don't recommend to use it.")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPEnSightSparseMode.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPEnSightSparseMode.h")

vtk_module_add_module(ParaView::VTKExtensionsIOEnSight
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_pv_ioensight.xml)
