/*=========================================================================

  Program:   ParaView
  Module:    vtkColorByPart.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkColorByPart - Adds a scalar array with input index as values.
// .SECTION Description
// vtkColorByPart specific for paraview.  Adds a scalar array so each part
// will get a separate color.

#ifndef __vtkColorByPart_h
#define __vtkColorByPart_h

#include "vtkSource.h"

class vtkDataSet;
class vtkIntArray;

class VTK_EXPORT vtkColorByPart : public vtkSource
{
public:
  static vtkColorByPart *New();

  vtkTypeRevisionMacro(vtkColorByPart,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to append.
  void AddInput(vtkDataSet *in);

  // Description:
  // Get any input of this filter.
  vtkDataSet *GetInput(int idx);
  vtkDataSet *GetInput() 
    {return this->GetInput( 0 );}
  
  // Description:
  // Get any input of this filter.
  int GetNumberOfOutputs();
  vtkDataSet *GetOutput(int idx);
  vtkDataSet *GetOutput() 
    {return this->GetInput( 0 );}
  
  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkColorByPart() {};
  ~vtkColorByPart() {};

  // Usual data generation method
  virtual void Execute();
  virtual void ExecuteInformation();

private:

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
private:
  vtkColorByPart(const vtkColorByPart&);  // Not implemented.
  void operator=(const vtkColorByPart&);  // Not implemented.
};


#endif


