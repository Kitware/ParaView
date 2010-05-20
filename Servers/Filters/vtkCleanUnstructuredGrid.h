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

// .NAME vtkCleanUnstructuredGrid - merge duplicate points
//
// .SECTION Description
// vtkCleanUnstructuredGrid is a filter that takes unstructured grid data as 
// input and generates unstructured grid data as output. vtkCleanUnstructuredGrid can 
// merge duplicate points (with coincident coordinates) using the vtkMergePoints object
// to merge points.

// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkCleanUnstructuredGrid_h
#define __vtkCleanUnstructuredGrid_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkPointLocator;

class VTK_EXPORT vtkCleanUnstructuredGrid: public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCleanUnstructuredGrid *New();

  vtkTypeMacro(vtkCleanUnstructuredGrid, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkCleanUnstructuredGrid();
  ~vtkCleanUnstructuredGrid();

  vtkPointLocator *Locator;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:

  vtkCleanUnstructuredGrid(const vtkCleanUnstructuredGrid&); // Not implemented
  void operator=(const vtkCleanUnstructuredGrid&); // Not implemented
};
#endif
