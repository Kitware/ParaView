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

// .NAME vtkCleanUnstructuredGrid

#ifndef __vtkCleanUnstructuredGrid_h
#define __vtkCleanUnstructuredGrid_h

#include <vtkDataSetToUnstructuredGridFilter.h>

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
