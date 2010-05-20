/*=========================================================================

  Program:   ParaView
  Module:    vtkCPGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPGridBuilder - Abstract class for creating grids.
// .SECTION Description
// Abstract class for creating grids for a test driver.  

#ifndef __vtkCPGridBuilder_h
#define __vtkCPGridBuilder_h

#include "vtkCPBaseGridBuilder.h"

class vtkDataObject;
class vtkCPBaseFieldBuilder;

class VTK_EXPORT vtkCPGridBuilder : public vtkCPBaseGridBuilder
{
public:
  vtkTypeMacro(vtkCPGridBuilder, vtkCPBaseGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a grid.  BuiltNewGrid is set to 0 if the grids
  // that were returned were already built before.
  // vtkCPGridBuilder will also delete the grid.
  virtual vtkDataObject* GetGrid(unsigned long timeStep, double time,
                                 int & builtNewGrid) = 0;

  // Description:
  // Set/get the FieldBuilder.  
  void SetFieldBuilder(vtkCPBaseFieldBuilder* fieldBuilder);
  vtkCPBaseFieldBuilder* GetFieldBuilder();

protected:
  vtkCPGridBuilder();
  ~vtkCPGridBuilder();

private:
  vtkCPGridBuilder(const vtkCPGridBuilder&); // Not implemented

  void operator=(const vtkCPGridBuilder&); // Not implemented
  // Description:
  // The field builder for creating the input fields to the coprocessing 
  // library.
  vtkCPBaseFieldBuilder* FieldBuilder;
};

#endif
