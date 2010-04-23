/*=========================================================================

  Program:   ParaView
  Module:    vtkCPFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPFieldBuilder - Abstract class for specifying fields over grids.
// .SECTION Description
// Abstract class for specifying fields over grids for a test driver.  

#ifndef __vtkCPFieldBuilder_h
#define __vtkCPFieldBuilder_h

#include "vtkCPBaseFieldBuilder.h"

class vtkCPTensorFieldFunction;

class VTK_EXPORT vtkCPFieldBuilder : public vtkCPBaseFieldBuilder
{
public:
  vtkTypeMacro(vtkCPFieldBuilder, vtkCPBaseFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a field on Grid. 
  virtual void BuildField(unsigned long TimeStep, double Time,
                          vtkDataSet* Grid) = 0;

  // Description:
  // Return the highest order of discretization of the field.
  //virtual unsigned int GetHighestFieldOrder() = 0;

  // Description:
  // Set/get the name of the field array.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // Set/get TensorFieldFunction.
  void SetTensorFieldFunction(vtkCPTensorFieldFunction* TFF);
  vtkCPTensorFieldFunction* GetTensorFieldFunction();

protected:
  vtkCPFieldBuilder();
  ~vtkCPFieldBuilder();

private:
  vtkCPFieldBuilder(const vtkCPFieldBuilder&); // Not implemented
  void operator=(const vtkCPFieldBuilder&); // Not implemented

  // Description:
  // The name of the array that will be inserted into the point/cell data.
  char* ArrayName;

  // Description:
  // The function that actually computes the tensor field values at 
  // specified points.
  vtkCPTensorFieldFunction* TensorFieldFunction;
};

#endif
