/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeArrays.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMergeArrays - Multiple inputs with same geometry, one output.
// .SECTION Description
// vtkMergeArrays Expects that all inputs have the same geometry.
// Arrays from all inputs are put into out output.
// The filter checks for a consistent number of points and cells, but
// not check any more.  Any inputs which do not have the correct number
// of points and cells are ignored.

#ifndef __vtkMergeArrays_h
#define __vtkMergeArrays_h

#include "vtkSource.h"

class vtkDataSet;

class VTK_EXPORT vtkMergeArrays : public vtkSource
{
public:
  static vtkMergeArrays *New();

  vtkTypeRevisionMacro(vtkMergeArrays,vtkSource);
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
  virtual int GetNumberOfOutputs() { return 1;}
  vtkDataSet *GetOutput(); 
  vtkDataSet *GetOutput(int idx); 

  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );  
  
protected:
  vtkMergeArrays();
  ~vtkMergeArrays();

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
  vtkMergeArrays(const vtkMergeArrays&);  // Not implemented.
  void operator=(const vtkMergeArrays&);  // Not implemented.
};


#endif


