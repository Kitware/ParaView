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

if (APPLE)
  set (Mac_SRCS
    vtkPVMacFileInformationHelper.mm
  )
endif()

set (Module_SRCS
  vtkNetworkImageSource.cxx
  vtkPVEnsembleDataReaderInformation.cxx
  vtkPVEnvironmentInformation.cxx
  vtkPVEnvironmentInformationHelper.cxx
  vtkPVFileInformation.cxx
  vtkPVSingleOutputExtractSelection.cxx
  vtkSliceAlongPolyPlane.cxx
  ${Mac_SRCS}
)

if (APPLE)
  set_source_files_properties(
    vtkPVMacFileInformationHelper
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
      SKIP_HEADER_INSTALL 1
  )
endif()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Module_SRCS
    vtkAnnotateGlobalDataFilter.cxx
    vtkPythonCalculator.cxx
    vtkAnnotateAttributeDataFilter.cxx
    )
endif()

set_source_files_properties(
  vtkNetworkAccessManager
  ABSTRACT)

vtk_module_library(vtkPVClientServerCoreDefault ${Module_SRCS})

# for vtkPVFileInformation
if (WIN32)
  vtk_module_link_libraries(vtkPVClientServerCoreDefault
    LINK_PRIVATE mpr)
endif()
