/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGroup.cxx
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
#include "vtkGroup.h"

#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"

vtkCxxRevisionMacro(vtkGroup, "1.3");
vtkStandardNewMacro(vtkGroup);

//----------------------------------------------------------------------------
vtkGroup::vtkGroup()
{
}

//----------------------------------------------------------------------------
vtkGroup::~vtkGroup()
{
}

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkGroup::AddInput(vtkDataSet *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkGroup::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
int vtkGroup::GetNumberOfOutputs()
{
  return this->NumberOfInputs;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkGroup::GetOutput(int idxOut)
{
  vtkDataSet* input = NULL;
  vtkDataObject* output;

  // Find the corresponding input for this output.
  input = this->GetInput(idxOut);
  if (input == NULL)
    {
    vtkErrorMacro("Corresponding input for requested output is not set.");
    return NULL;
    }
  output = this->Superclass::GetOutput(idxOut);
  if (output == NULL)
    { // Create a new output.
    output = input->NewInstance();
    if (this->NumberOfOutputs <= idxOut)
      {
      this->SetNumberOfOutputs(idxOut+1);
      }
    this->Outputs[idxOut] = output;
    output->SetSource(this);
    return static_cast<vtkDataSet*>(output);
    }
  if (input->GetDataObjectType() != output->GetDataObjectType())
    {
    vtkErrorMacro("Input and output do not match type.");
    return static_cast<vtkDataSet*>(output);
    }
  return static_cast<vtkDataSet*>(output);
}


//----------------------------------------------------------------------------
void vtkGroup::ExecuteInformation()
{
  int idx;
  int num;
  vtkDataSet *input;
  vtkDataSet *output;

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    output = this->GetOutput(idx);
    output->SetExtentTranslator(input->GetExtentTranslator());
    if (input == NULL || output == NULL ||
        input->GetDataObjectType() != output->GetDataObjectType())
      {
      vtkErrorMacro("Input/Output mismatch.");
      }
    else
      {
      output->CopyInformation(input);
      output->SetPipelineMTime(input->GetPipelineMTime());
      }
    } 
}


//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkGroup::Execute()
{
  int idx;
  int num;
  vtkDataSet *input;
  vtkDataSet *output;

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    output = this->GetOutput(idx);
    if (input == NULL || output == NULL ||
        input->GetDataObjectType() != output->GetDataObjectType())
      {
      vtkErrorMacro("Input/Output mismatch.");
      }
    else
      {
      output->CopyStructure(input);
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
      output->GetFieldData()->PassData(input->GetFieldData());
      }
    } 
}

//----------------------------------------------------------------------------
void vtkGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  int num;
  vtkDataSet *input;
  vtkDataSet *output;

  this->Superclass::PrintSelf(os,indent);

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    output = this->GetOutput(idx);
    os << indent << "Input: (" << input << "), passed, Output: (" << output << ").\n";
    } 
}
