/*=========================================================================

  Program:   ParaView
  Module:    vtkCPFileGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPFileGridBuilder - Class for creating grids from a VTK file.
// .SECTION Description
// Class for creating grids from a VTK file.  

#ifndef __vtkCPFileGridBuilder_h
#define __vtkCPFileGridBuilder_h

#include "vtkCPGridBuilder.h"

class vtkDataObject;
class vtkCPFieldBuilder;

class VTK_EXPORT vtkCPFileGridBuilder : public vtkCPGridBuilder
{
public:
  vtkTypeMacro(vtkCPFileGridBuilder, vtkCPGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a grid.  BuiltNewGrid is set to 0 if the grids
  // that were returned were already built before.
  // vtkCPFileGridBuilder will also delete the grid.
  virtual vtkDataObject* GetGrid(unsigned long timeStep, double time,
                                 int & builtNewGrid);

  // Description:
  // Set/get the FileName.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  // Description:
  // Set/get KeepPointData.
  vtkGetMacro(KeepPointData, bool);
  vtkSetMacro(KeepPointData, bool);

  // Description:
  // Set/get KeepPointData.
  vtkGetMacro(KeepCellData, bool);
  vtkSetMacro(KeepCellData, bool);

  // Description:
  // Get the current grid.
  vtkDataObject* GetGrid();

protected:
  vtkCPFileGridBuilder();
  ~vtkCPFileGridBuilder();

  // Description:
  // Function to set the grid and take care of the reference counting.
  virtual void SetGrid(vtkDataObject*);

private:
  vtkCPFileGridBuilder(const vtkCPFileGridBuilder&); // Not implemented

  void operator=(const vtkCPFileGridBuilder&); // Not implemented

  // Description:
  // The name of the VTK file to be read.
  char * FileName;

  // Description:
  // Flag to indicate that any vtkPointData arrays that are set by the
  // file reader are to be cleared out.  By default this is true.
  bool KeepPointData;

  // Description:
  // Flag to indicate that any vtkCellData arrays that are set by the
  // file reader are to be cleared out.  By default this is true.
  bool KeepCellData;

  // Description:
  // The grid that is returned.
  vtkDataObject* Grid;
};

#endif
