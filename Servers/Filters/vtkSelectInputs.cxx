/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectInputs.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectInputs.h"

#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"

vtkCxxRevisionMacro(vtkSelectInputs, "1.5");
vtkStandardNewMacro(vtkSelectInputs);

//----------------------------------------------------------------------------
vtkSelectInputs::vtkSelectInputs()
{
  this->InputMask = vtkIntArray::New();
}

//----------------------------------------------------------------------------
vtkSelectInputs::~vtkSelectInputs()
{
  this->InputMask->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectInputs::SetInputMask(int idx, int flag)
{
  int length = this->InputMask->GetNumberOfTuples();
  int i2;

  for (i2 = length; i2 <= idx; ++i2)
    { // Default value on.
    this->InputMask->InsertValue(i2, 1);
    }
  this->InputMask->SetValue(idx, flag);

  // Wipe out all outputs when the mask is changed
  this->SetNumberOfOutputs(0);
}

//----------------------------------------------------------------------------
int vtkSelectInputs::GetInputMask(int idx)
{
  int length = this->InputMask->GetNumberOfTuples();

  if (idx >= length)
    {
    return 1;
    }

  return this->InputMask->GetValue(idx);
}

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkSelectInputs::AddInput(vtkDataSet *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkSelectInputs::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
int vtkSelectInputs::GetNumberOfOutputs()
{
  int count = 0;
  int idx, num;

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    if (this->GetInputMask(idx))
      {
      ++count;
      }
    }

  return count;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkSelectInputs::GetOutput(int idxOut)
{
  vtkDataSet* input = NULL;
  vtkDataObject* output;
  int idxIn = 0;
  int numIn = this->GetNumberOfInputs();
  int count = 0;

  // Find the corresponding input for this output.
  while (idxIn < numIn)
    {
    if (this->GetInputMask(idxIn))
      {
      if (count == idxOut)
        {
        input = this->GetInput(idxIn);
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
      ++count;
      }
    
    ++idxIn;
    }

  vtkErrorMacro("Not enough true mask elements to produce requested output.");
  return NULL;    
}


//----------------------------------------------------------------------------
void vtkSelectInputs::ExecuteInformation()
{
  int idx;
  int num;
  int count = 0;
  vtkDataSet *input;
  vtkDataSet *output;

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    if (this->GetInputMask(idx))
      {
      output = this->GetOutput(count);
      if (input == NULL || output == NULL ||
          input->GetDataObjectType() != output->GetDataObjectType())
        {
        vtkErrorMacro("Input/Output mismatch.");
        }
      else
        {
        output->CopyInformation(input);
        }
      ++count;
      }
    } 
}


//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkSelectInputs::Execute()
{
  int idx;
  int num;
  int count = 0;
  vtkDataSet *input;
  vtkDataSet *output;

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    if (this->GetInputMask(idx))
      {
      output = this->GetOutput(count);
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
      ++count;
      }
    } 
}

//----------------------------------------------------------------------------
void vtkSelectInputs::ComputeInputUpdateExtents(vtkDataObject *)
{
  int idx;
  int num;
  int count = 0;
  vtkDataSet *input;
  vtkDataSet *output;

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    if (this->GetInputMask(idx))
      {
      output = this->GetOutput(count);
      if (input == NULL || output == NULL ||
          input->GetDataObjectType() != output->GetDataObjectType())
        {
        vtkErrorMacro("Input/Output mismatch.");
        }
      else
        {
        input->SetUpdatePiece( output->GetUpdatePiece() );
        input->SetUpdateNumberOfPieces( output->GetUpdateNumberOfPieces() );
        input->SetUpdateGhostLevel( output->GetUpdateGhostLevel() );
        input->SetUpdateExtent( output->GetUpdateExtent() );
        }
      ++count;
      }
    else
      {
      // This input is not selected.  Ask for empty data.
      input->SetUpdateExtent(0, 1, 0);
      }
    } 
}



//----------------------------------------------------------------------------
void vtkSelectInputs::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  int num;
  int count = 0;
  vtkDataSet *input;
  vtkDataSet *output;

  this->Superclass::PrintSelf(os,indent);

  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    if (this->GetInputMask(idx))
      {
      output = this->GetOutput(count);
      ++count;
      os << indent << "Input: (" << input << "), passed, Output: (" << output << ").\n";
      }
    else
      {
      os << indent << "Input: (" << input << "), masked.\n";
      }
    } 
}
