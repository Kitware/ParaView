/*=========================================================================

  Program:   ParaView
  Module:    vtkMergeArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeArrays.h"

#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"

vtkCxxRevisionMacro(vtkMergeArrays, "1.4");
vtkStandardNewMacro(vtkMergeArrays);

//----------------------------------------------------------------------------
vtkMergeArrays::vtkMergeArrays()
{
}

//----------------------------------------------------------------------------
vtkMergeArrays::~vtkMergeArrays()
{
}

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkMergeArrays::AddInput(vtkDataSet *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkMergeArrays::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkMergeArrays::GetOutput(int idx)
{
  if (idx == 0)
    {
    return this->GetOutput();
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkMergeArrays::GetOutput()
{
  vtkDataSet* input = NULL;
  vtkDataObject* output;

  // Find the corresponding input for this output.
  input = this->GetInput(0);
  if (input == NULL)
    {
    vtkErrorMacro("You need to set an input before you get the output.");
    return NULL;
    }
  output = this->Superclass::GetOutput(0);
  if (output == NULL)
    { // Create a new output.
    output = input->NewInstance();
    if (this->NumberOfOutputs <= 0)
      {
      this->SetNumberOfOutputs(1);
      }
    this->Outputs[0] = output;
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
// Copy the update information across
void vtkMergeArrays::ComputeInputUpdateExtents(vtkDataObject *)
{
  int idx;
  int num;
  vtkDataSet *input;
  vtkDataSet *output;

  num = this->NumberOfInputs;
  output = this->GetOutput();
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetInput(idx);

    input->SetUpdatePiece( output->GetUpdatePiece() );
    input->SetUpdateNumberOfPieces( output->GetUpdateNumberOfPieces() );
    input->SetUpdateGhostLevel( output->GetUpdateGhostLevel() );
    input->SetUpdateExtent( output->GetUpdateExtent() );
    }
}



//----------------------------------------------------------------------------
void vtkMergeArrays::ExecuteInformation()
{
  vtkDataSet *input;
  vtkDataSet *output;

  input = this->GetInput(0);
  output = this->GetOutput();
  if (input == NULL || output == NULL ||
      input->GetDataObjectType() != output->GetDataObjectType())
    {
    vtkErrorMacro("Input/Output mismatch.");
    }
  else
    {
    output->CopyInformation(input);
    }
}


//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkMergeArrays::Execute()
{
  int idx;
  int num;
  int numCells, numPoints;
  int numArrays, arrayIdx;
  vtkDataSet *input;
  vtkDataSet *output;
  vtkDataArray *array;

  num = this->NumberOfInputs;
  if (num == 0)
    {
    return;
    }

  output = this->GetOutput();
  input = this->GetInput(0);
  numCells = input->GetNumberOfCells();
  numPoints = input->GetNumberOfPoints();
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  for (idx = 1; idx < num; ++idx)
    {
    input = this->GetInput(idx);
    if (output->GetNumberOfPoints() == numPoints &&
        output->GetNumberOfCells() == numCells)
      {
      numArrays = input->GetPointData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
        {
        array = input->GetPointData()->GetArray(arrayIdx);
        // What should we do about arrays with the same name?
        output->GetPointData()->AddArray(array);
        }
      numArrays = input->GetCellData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
        {
        array = input->GetCellData()->GetArray(arrayIdx);
        // What should we do about arrays with the same name?
        output->GetCellData()->AddArray(array);
        }
      numArrays = input->GetFieldData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
        {
        array = input->GetFieldData()->GetArray(arrayIdx);
        // What should we do about arrays with the same name?
        output->GetFieldData()->AddArray(array);
        }
      }
    } 
}

//----------------------------------------------------------------------------
void vtkMergeArrays::PrintSelf(ostream& os, vtkIndent indent)
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
    if( input )
      {
      os << indent << "Input: (" << input << ")\n";
      }
    else
      {
      os << indent << "No Input\n";
      }
    } 
  if( ( output = this->GetOutput() ) )
    {
    os << indent << "Output: (" << output << ")\n";
    }
  else
    {
    os << indent << "No Output\n";
    }  
}
