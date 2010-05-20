/*=========================================================================

  Program:   ParaView
  Module:    vtkCPUniformGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPUniformGridBuilder - Class for creating uniform grids.
// .SECTION Description
// Class for creating vtkUniformGrids for a test driver.  

#ifndef __vtkCPUniformGridBuilder_h
#define __vtkCPUniformGridBuilder_h

#include "vtkCPGridBuilder.h"

class vtkDataObject;
class vtkUniformGrid;

class VTK_EXPORT vtkCPUniformGridBuilder : public vtkCPGridBuilder
{
public:
  static vtkCPUniformGridBuilder* New();
  vtkTypeMacro(vtkCPUniformGridBuilder, vtkCPGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a grid.  BuiltNewGrid is set to 0 if the grids
  // that were returned were already built before.
  // vtkCPUniformGridBuilder will also delete the grid.
  virtual vtkDataObject* GetGrid(unsigned long timeStep, double time,
                                 int & builtNewGrid);

  // Description:
  // Set/get the Dimensions of the uniform grid.
  vtkSetVector3Macro(Dimensions, int);
  int* GetDimensions();

  // Description:
  // Set/get the Dimensions of the uniform grid.
  vtkSetVector3Macro(Spacing, double);
  double* GetSpacing();

  // Description:
  // Set/get the Dimensions of the uniform grid.
  vtkSetVector3Macro(Origin, double);
  double* GetOrigin();

  // Description:
  // Get the UniformGrid.
  vtkUniformGrid* GetUniformGrid();

  // Description:
  // Create UniformGrid with the current parameters.  Returns true if
  // a new grid was created and false otherwise.
  bool CreateUniformGrid();

protected:
  vtkCPUniformGridBuilder();
  ~vtkCPUniformGridBuilder();

private:
  vtkCPUniformGridBuilder(const vtkCPUniformGridBuilder&); // Not implemented
  void operator=(const vtkCPUniformGridBuilder&); // Not implemented

  // Description:
  // The dimensions of the vtkUniformGrid.
  int Dimensions[3];

  // Description:
  // The spacing of the vtkUniformGrid.
  double Spacing[3];

  // Description:
  // The origin of the vtkUniformGrid.
  double Origin[3];

  // Description:
  // The uniform grid that is created.
  vtkUniformGrid* UniformGrid;

  // Description:
  // Macro to set UniformGrid.
  void SetUniformGrid(vtkUniformGrid* UG);
};
#endif
