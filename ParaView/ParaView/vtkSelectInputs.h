/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectInputs.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectInputs - Mask whole input data sets.
// .SECTION Description
// vtkSelectInputs takes multiple inputs and has multiple outputs.
// An output is equivalent to one of the inputs.  
// A mask is used to eliminate outputs so there are fewer outputs
// than inputs.

#ifndef __vtkSelectInputs_h
#define __vtkSelectInputs_h

#include "vtkSource.h"

class vtkDataSet;
class vtkIntArray;

class VTK_EXPORT vtkSelectInputs : public vtkSource
{
public:
  static vtkSelectInputs *New();

  vtkTypeRevisionMacro(vtkSelectInputs,vtkSource);
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
  virtual int GetNumberOfOutputs();
  vtkDataSet *GetOutput(int idx);
  vtkDataSet *GetOutput() 
    {return this->GetInput( 0 );}
  
  // Description:
  // Methods to select which inputs become outputs.
  // These flags can be set even before the inputs
  // are added.  Flags default to 1 (on).
  void SetInputMask(int idx, int flag);
  int GetInputMask(int idx);

  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkSelectInputs();
  ~vtkSelectInputs();

  // Usual data generation method
  virtual void Execute();
  virtual void ExecuteInformation();

  vtkIntArray *InputMask;

private:

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
private:
  vtkSelectInputs(const vtkSelectInputs&);  // Not implemented.
  void operator=(const vtkSelectInputs&);  // Not implemented.
};


#endif


