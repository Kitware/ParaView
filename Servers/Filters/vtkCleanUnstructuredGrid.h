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
// input and generates polygonal data as output. vtkCleanUnstructuredGrid can 
// merge duplicate points (within specified tolerance and if enabled).
// Tolerance is specified precisely = 0.00001*input->GetLength(), then 
// vtkCleanUnstructuredGrid will use the vtkPointLocator object to merge points
//  (which is slow).

// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkCleanUnstructuredGrid_h
#define __vtkCleanUnstructuredGrid_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkUnstructuredGrid;
class vtkPointLocator;

class VTK_EXPORT vtkCleanUnstructuredGrid: public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkCleanUnstructuredGrid *New();

  vtkTypeRevisionMacro(vtkCleanUnstructuredGrid, 
    vtkDataSetToUnstructuredGridFilter);

  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkCleanUnstructuredGrid();
  ~vtkCleanUnstructuredGrid();

  vtkPointLocator *Locator;

  void Execute();

private:

  vtkCleanUnstructuredGrid(const vtkCleanUnstructuredGrid&); // Not implemented
  void operator=(const vtkCleanUnstructuredGrid&); // Not implemented
};
#endif
