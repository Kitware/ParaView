/*=========================================================================

  Program:   ParaView
  Module:    vtkColorByPart.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColorByPart.h"

#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"

vtkCxxRevisionMacro(vtkColorByPart, "1.6");
vtkStandardNewMacro(vtkColorByPart);


//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkColorByPart::AddInput(vtkDataSet *ds)
{
  this->Superclass::AddInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkColorByPart::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}


//----------------------------------------------------------------------------
vtkDataSet* vtkColorByPart::GetOutput(int idxOut)
{
  vtkDataSet* input = NULL;
  vtkDataObject* output;

  input = this->GetInput(idxOut);
  if (!input)
    {
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
int vtkColorByPart::GetNumberOfOutputs()
{
  return this->NumberOfInputs;
}

//----------------------------------------------------------------------------
// Copy the update information across
void vtkColorByPart::ComputeInputUpdateExtents(vtkDataObject *)
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

    input->SetUpdatePiece( output->GetUpdatePiece() );
    input->SetUpdateNumberOfPieces( output->GetUpdateNumberOfPieces() );
    input->SetUpdateGhostLevel( output->GetUpdateGhostLevel() );
    input->SetUpdateExtent( output->GetUpdateExtent() );
    }
}



//----------------------------------------------------------------------------
void vtkColorByPart::ExecuteInformation()
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
      output->CopyInformation(input);
      }
    } 
}


//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkColorByPart::Execute()
{
  int idx;
  int num;
  vtkDataSet *input;
  vtkDataSet *output;
  vtkIntArray* colorArray;
  int numPoints, j;

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

      numPoints = output->GetNumberOfPoints();
      colorArray = vtkIntArray::New();
      colorArray->SetNumberOfTuples(numPoints);
      for (j = 0; j < numPoints; ++j)
        {
        colorArray->SetValue(j, idx);
        }
      colorArray->SetName("Part Id");
      output->GetPointData()->SetScalars(colorArray);
      colorArray->Delete();
      colorArray = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkColorByPart::PrintSelf(ostream& os, vtkIndent indent)
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
