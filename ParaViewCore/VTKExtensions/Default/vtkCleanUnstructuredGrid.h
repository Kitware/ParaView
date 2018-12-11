/*=========================================================================

  Program:   ParaView
  Module:    vtkCleanUnstructuredGrid.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCleanUnstructuredGrid
 * @brief   merge duplicate points
 *
 *
 * vtkCleanUnstructuredGrid is a filter that takes unstructured grid data as
 * input and generates unstructured grid data as output. vtkCleanUnstructuredGrid can
 * merge duplicate points (with coincident coordinates) using the vtkMergePoints object
 * to merge points.
 *
 * @sa
 * vtkCleanPolyData
*/

#ifndef vtkCleanUnstructuredGrid_h
#define vtkCleanUnstructuredGrid_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkUnstructuredGridAlgorithm.h"

class vtkPointLocator;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkCleanUnstructuredGrid
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCleanUnstructuredGrid* New();

  vtkTypeMacro(vtkCleanUnstructuredGrid, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCleanUnstructuredGrid();
  ~vtkCleanUnstructuredGrid() override;

  vtkPointLocator* Locator;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCleanUnstructuredGrid(const vtkCleanUnstructuredGrid&) = delete;
  void operator=(const vtkCleanUnstructuredGrid&) = delete;
};
#endif
