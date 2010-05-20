/*=========================================================================

  Program:   ParaView
  Module:    vtkCPUnstructuredGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPUnstructuredGridBuilder - Class for creating unstructured grids.
// .SECTION Description
// Class for creating vtkUnstructuredGrids for a test driver.  Note that
// the user must call SetPoints(), Allocate(), and InsertNextCell().

#ifndef __vtkCPUnstructuredGridBuilder_h
#define __vtkCPUnstructuredGridBuilder_h

#include "vtkCPGridBuilder.h"

class vtkDataObject;
class vtkIdList;
class vtkPoints;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkCPUnstructuredGridBuilder : public vtkCPGridBuilder
{
public:
  static vtkCPUnstructuredGridBuilder* New();
  vtkTypeMacro(vtkCPUnstructuredGridBuilder, vtkCPGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a grid.  BuiltNewGrid is set to 0 if the grids
  // that were returned were already built before.
  // vtkCPUnstructuredGridBuilder will also delete the grid.
  virtual vtkDataObject* GetGrid(unsigned long timeStep, double time,
                                 int & builtNewGrid);

  // Description:
  // Get the UnstructuredGrid.
  vtkUnstructuredGrid* GetUnstructuredGrid();

  // Description:
  // Set the vtkPoints of the vtkUnstructuredGrid. Returns true
  // if successful.
  bool SetPoints(vtkPoints* points);

  // Description:
  // Allocate memory for the cells on UnstructuredGrid.
  virtual void Allocate(vtkIdType numCells=1000, int extSize=1000);

  // Description:
  // Insert/create cell in object by type and list of point 
  // ids defining cell topology. 
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *pts);
  vtkIdType InsertNextCell(int type, vtkIdList *ptIds);

protected:
  vtkCPUnstructuredGridBuilder();
  ~vtkCPUnstructuredGridBuilder();

  // Description:
  // Flag to indicate if UnstructuredGrid has been modified since last call
  // to GetGrid().
  bool IsGridModified;

  // Description:
  // Macro to set UnstructuredGrid.
  void SetUnstructuredGrid(vtkUnstructuredGrid* grid);

private:
  vtkCPUnstructuredGridBuilder(const vtkCPUnstructuredGridBuilder&); // Not implemented
  void operator=(const vtkCPUnstructuredGridBuilder&); // Not implemented

  // Description:
  // The unstructured grid that is created.
  vtkUnstructuredGrid* UnstructuredGrid;
};
#endif
