/*=========================================================================

  Program:   ParaView
  Module:    vtkGroup.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGroup - For grouping in ParaView.
// .SECTION Description
// vtkGroup is very simple.  It takes multiple inputs and passes
// them to multiple outputs.  There is a one to one relationship
// between inputs and outputs.  The outputs are shallow copies
// of the corresponding inputs.  This works in paraview because
// multiple outputs are groupd in the user interface.

#ifndef __vtkGroup_h
#define __vtkGroup_h

#include "vtkSource.h"

class vtkDataSet;

class VTK_EXPORT vtkGroup : public vtkSource
{
public:
  static vtkGroup *New();

  vtkTypeRevisionMacro(vtkGroup,vtkSource);
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
    {return this->GetOutput( 0 );}

  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkGroup();
  ~vtkGroup();

  // Usual data generation method
  virtual void Execute();
  virtual void ExecuteInformation();
  virtual void PropagateUpdateExtent(vtkDataObject *output);

private:

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
private:
  vtkGroup(const vtkGroup&);  // Not implemented.
  void operator=(const vtkGroup&);  // Not implemented.
};


#endif


