# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  CAdaptorAPI
  vtkCPAdaptorAPI
  vtkCPCxxHelper
  vtkCPDataDescription
  vtkCPInputDataDescription
  vtkCPPipeline
  vtkCPProcessor
  vtkCPXMLPWriterPipeline)
set(sources)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkCPConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkCPConfig.h"
  @ONLY)
set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkCPConfig.h")

set(catalyst_fortran_using_mangling)

if (CMAKE_Fortran_COMPILER_WORKS)
  set(CMAKE_Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Fortran")

  list(APPEND sources
    FortranAdaptorAPI.f90)

  include(FortranCInterface)
  FortranCInterface_HEADER(FortranAdaptorAPIMangling.h
    SYMBOLS
      coprocessorinitialize
      coprocessorfinalize
      requestdatadescription
      needtocreategrid
      coprocess)

  if (FortranCInterface_GLOBAL_PREFIX OR
      FortranCInterface_GLOBAL_SUFFIX OR
      (NOT FortranCInterface_GLOBAL_CASE STREQUAL "LOWER"))
    set(catalyst_fortran_using_mangling "${FortranCInterface_GLOBAL_FOUND}")
  else()
    # otherwise there's no mangling necessary for Fortran/C interchange and
    # hence we leave catalyst_fortran_using_mangling unset.
  endif()

  list(APPEND classes
    FortranAdaptorAPI)
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/FortranAdaptorAPIMangling.h")
endif ()

vtk_module_add_module(ParaView::Catalyst
  SPDX_SKIP_REGEX "FortranAdaptorAPIMangling.h"
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers})
vtk_module_client_server_exclude()
vtk_module_link(ParaView::Catalyst
  PRIVATE
    ParaView::paraview_plugins)

if (catalyst_fortran_using_mangling)
  vtk_module_definitions(ParaView::Catalyst
    PRIVATE
      CATALYST_FORTRAN_USING_MANGLING=1)
endif ()

if (CMAKE_Fortran_COMPILER_WORKS)
  vtk_module_include(ParaView::Catalyst
    INTERFACE
      "$<BUILD_INTERFACE:${CMAKE_Fortran_MODULE_DIRECTORY}>")
  install(
    DIRECTORY   "${CMAKE_Fortran_MODULE_DIRECTORY}/"
    DESTINATION "${_vtk_build_HEADERS_DESTINATION}"
    COMPONENT   "${_vtk_build_HEADERS_COMPONENT}")
endif ()
