# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  CPythonAdaptorAPI
  vtkCPPythonAdaptorAPI
  vtkCPPythonPipeline
  vtkCPPythonScriptPipeline
  vtkCPPythonScriptV2Helper
  vtkCPPythonScriptV2Pipeline
  vtkCPPythonStringPipeline)

set(sources)

set(catalyst_fortran_using_mangling)

if (CMAKE_Fortran_COMPILER_WORKS)
  set(CMAKE_Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Fortran")

  list(APPEND sources
    FortranPythonAdaptorAPI.f90)

  include(FortranCInterface)
  FortranCInterface_HEADER(FortranPythonAdaptorAPIMangling.h
    SYMBOLS
      coprocessorinitializewithpython
      coprocessoraddpythonscript)

  if (FortranCInterface_GLOBAL_PREFIX OR
      FortranCInterface_GLOBAL_SUFFIX OR
      (NOT FortranCInterface_GLOBAL_CASE STREQUAL "LOWER"))
    set(catalyst_fortran_using_mangling "${FortranCInterface_GLOBAL_FOUND}")
  else()
    # otherwise there's no mangling necessary for Fortran/C interchange and
    # hence we leave catalyst_fortran_using_mangling unset.
  endif()

  list(APPEND classes
    FortranPythonAdaptorAPI)
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/FortranPythonAdaptorAPIMangling.h")
endif ()

vtk_module_add_module(ParaView::PythonCatalyst
  SPDX_SKIP_REGEX "FortranPythonAdaptorAPIMangling.h"
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers})
vtk_module_client_server_exclude()

if (catalyst_fortran_using_mangling)
  vtk_module_definitions(ParaView::PythonCatalyst
    PRIVATE
      CATALYST_FORTRAN_USING_MANGLING=1)
endif ()
vtk_module_definitions(ParaView::PythonCatalyst
  PRIVATE
  "PARAVIEW_USE_EXTERNAL_VTK=$<BOOL:${PARAVIEW_USE_EXTERNAL_VTK}>")

if (CMAKE_Fortran_COMPILER_WORKS)
  vtk_module_include(ParaView::PythonCatalyst
    INTERFACE
      "$<BUILD_INTERFACE:${CMAKE_Fortran_MODULE_DIRECTORY}>")
  install(
    DIRECTORY   "${CMAKE_Fortran_MODULE_DIRECTORY}/"
    DESTINATION "${_vtk_build_HEADERS_DESTINATION}"
    COMPONENT   "${_vtk_build_HEADERS_COMPONENT}")
endif ()
