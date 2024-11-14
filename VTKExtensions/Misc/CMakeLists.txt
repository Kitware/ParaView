# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAppendReduceFilter
  vtkAppendRectilinearGrid
  vtkAttributeDataReductionFilter
  vtkCompleteArrays
  vtkEnvironmentAnnotationFilter
  vtkMinMax
  vtkPExtractHistogram
  vtkPVExtractHistogram2D
  vtkPVBox
  vtkPVChangeOfBasisHelper
  vtkPVCone
  vtkPVCylinder
  vtkPVFrustum
  vtkPVMergeTables
  vtkPVMergeTablesMultiBlock
  vtkPVPlane
  vtkPVTransform
  vtkPVRotateAroundOriginTransform
  vtkReductionFilter
  vtkSelectionSerializer)

vtk_module_add_module(ParaView::VTKExtensionsMisc
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/misc_filters.xml
        Resources/misc_utilities.xml)
