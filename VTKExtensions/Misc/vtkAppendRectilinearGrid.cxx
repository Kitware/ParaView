/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendRectilinearGrid.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkAppendRectilinearGrid);
//-----------------------------------------------------------------------------
vtkAppendRectilinearGrid::vtkAppendRectilinearGrid() = default;

//-----------------------------------------------------------------------------
vtkAppendRectilinearGrid::~vtkAppendRectilinearGrid() = default;

//-----------------------------------------------------------------------------
int vtkAppendRectilinearGrid::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAppendRectilinearGrid::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int* outUpdateExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int cc = 0; cc < numInputs; cc++)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(cc);
    int inWholeExtent[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExtent);

    int inUpdateExt[6];
    for (int i = 0; i < 3; i++)
    {
      inUpdateExt[2 * i] =
        (inWholeExtent[2 * i] > outUpdateExt[2 * i]) ? inWholeExtent[2 * i] : outUpdateExt[2 * i];
      inUpdateExt[2 * i + 1] = (inWholeExtent[2 * i + 1] < outUpdateExt[2 * i + 1])
        ? inWholeExtent[2 * i + 1]
        : outUpdateExt[2 * i + 1];
    }

    // if min>max, the input is not needed at all.
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inUpdateExt, 6);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAppendRectilinearGrid::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs <= 0)
  {
    return 0;
  }

  int outWholeExtent[6];
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExtent);

  for (int cc = 1; cc < numInputs; cc++)
  {
    inInfo = inputVector[0]->GetInformationObject(cc);
    int inWholeExtent[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExtent);

    for (int i = 0; i < 3; i++)
    {
      outWholeExtent[2 * i] = (outWholeExtent[2 * i] < inWholeExtent[2 * i]) ? outWholeExtent[2 * i]
                                                                             : inWholeExtent[2 * i];
      outWholeExtent[2 * i + 1] = (outWholeExtent[2 * i + 1] > inWholeExtent[2 * i + 1])
        ? outWholeExtent[2 * i + 1]
        : inWholeExtent[2 * i + 1];
    }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExtent, 6);
  }
  return 1;
}

static void vtkAppendRectilinearGridConvertToCell(int* extent, const int* inExtent)
{
  memcpy(extent, inExtent, sizeof(int) * 6);
  for (int cc = 0; cc < 3; cc++)
  {
    extent[2 * cc + 1] =
      (extent[2 * cc + 1] > extent[2 * cc]) ? extent[2 * cc + 1] - 1 : extent[2 * cc + 1];
  }
}

//-----------------------------------------------------------------------------
int vtkAppendRectilinearGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkRectilinearGrid* output =
    vtkRectilinearGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int* outUpdateExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  output->SetExtent(outUpdateExt);

  int numTuples = (outUpdateExt[1] - outUpdateExt[0] + 1) *
    (outUpdateExt[3] - outUpdateExt[2] + 1) * (outUpdateExt[5] - outUpdateExt[4] + 1);

  vtkRectilinearGrid* input0 = vtkRectilinearGrid::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataArray* temp = input0->GetXCoordinates()->NewInstance();
  temp->SetNumberOfComponents(1);
  temp->SetNumberOfTuples(numTuples);
  output->SetXCoordinates(temp);
  temp->Delete();

  temp = input0->GetYCoordinates()->NewInstance();
  temp->SetNumberOfComponents(1);
  temp->SetNumberOfTuples(numTuples);
  output->SetYCoordinates(temp);
  temp->Delete();

  temp = input0->GetZCoordinates()->NewInstance();
  temp->SetNumberOfComponents(1);
  temp->SetNumberOfTuples(numTuples);
  output->SetZCoordinates(temp);
  temp->Delete();

  output->GetCellData()->CopyAllocate(input0->GetCellData(), numTuples);
  output->GetPointData()->CopyAllocate(input0->GetPointData(), numTuples);

  int outCellExtent[6];
  vtkAppendRectilinearGridConvertToCell(outCellExtent, outUpdateExt);
  int numCellTuples = (outCellExtent[1] - outCellExtent[0] + 1) *
    (outCellExtent[3] - outCellExtent[2] + 1) * (outCellExtent[5] - outCellExtent[4] + 1);

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int inputNum = numInputs - 1; inputNum >= 0; inputNum--)
  {
    vtkRectilinearGrid* input = vtkRectilinearGrid::SafeDownCast(
      inputVector[0]->GetInformationObject(inputNum)->Get(vtkDataObject::DATA_OBJECT()));
    int inputExtent[6];
    input->GetExtent(inputExtent);

    this->CopyArray(output->GetXCoordinates(), outUpdateExt, input->GetXCoordinates(), inputExtent);
    this->CopyArray(output->GetYCoordinates(), outUpdateExt, input->GetXCoordinates(), inputExtent);
    this->CopyArray(output->GetZCoordinates(), outUpdateExt, input->GetXCoordinates(), inputExtent);

    for (int cc = 0; cc < output->GetPointData()->GetNumberOfArrays(); cc++)
    {
      // Set number of tuples explicitly since CopyAllocate() does not set
      // number of tuples.
      output->GetPointData()->GetArray(cc)->SetNumberOfTuples(numTuples);
      this->CopyArray(output->GetPointData()->GetArray(cc), outUpdateExt,
        input->GetPointData()->GetArray(cc), inputExtent);
    }

    int inCellExtent[6];
    vtkAppendRectilinearGridConvertToCell(inCellExtent, inputExtent);

    for (int cc = 0; cc < output->GetCellData()->GetNumberOfArrays(); cc++)
    {
      // Set number of tuples explicitly since CopyAllocate() does not set
      // number of tuples.
      output->GetCellData()->GetArray(cc)->SetNumberOfTuples(numCellTuples);
      this->CopyArray(output->GetCellData()->GetArray(cc), outCellExtent,
        input->GetCellData()->GetArray(cc), inCellExtent);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
class vtkAppendRectilinearGridStructuredSpace
{
protected:
  int WholeExtent[6];
  int Extent[6];
  int Increments[3];
  int Index[3];

  int GetIndex(int x, int y, int z)
  {
    return x + y * this->Increments[1] + z * this->Increments[2];
  }

public:
  void SetWholeExtent(const int ext[6]) { memcpy(this->WholeExtent, ext, sizeof(int) * 6); }

  void SetExtent(const int ext[6]) { memcpy(this->Extent, ext, sizeof(int) * 6); }

  void Initialize()
  {
    this->Increments[0] = 0;
    this->Increments[1] = this->WholeExtent[1] - this->WholeExtent[0];
    this->Increments[2] = this->WholeExtent[3] - this->WholeExtent[2];

    this->Index[0] = this->Extent[0] - this->WholeExtent[0];
    this->Index[1] = this->Extent[2] - this->WholeExtent[2];
    this->Index[2] = this->Extent[4] - this->WholeExtent[4];
  }

  int BeginSpan() { return this->GetIndex(this->Index[0], this->Index[1], this->Index[2]); }

  int EndSpan() { return this->GetIndex(this->Extent[1], this->Index[1], this->Index[2]) + 1; }

  void NextSpan()
  {
    this->Index[1]++;
    if (this->Index[1] > this->Extent[3])
    {
      this->Index[1] = this->Extent[2];
      this->Index[2]++;
    }
  }

  bool IsAtEnd() { return (this->Index[2] > this->Extent[5]); }
};

//-----------------------------------------------------------------------------
void vtkAppendRectilinearGrid::CopyArray(vtkAbstractArray* outArray, const int* outExtents,
  vtkAbstractArray* inArray, const int* inExtents)
{
  vtkAppendRectilinearGridStructuredSpace inSpace;
  vtkAppendRectilinearGridStructuredSpace outSpace;
  inSpace.SetWholeExtent(inExtents);
  inSpace.SetExtent(inExtents);
  outSpace.SetWholeExtent(outExtents);
  outSpace.SetExtent(inExtents);

  inSpace.Initialize();
  outSpace.Initialize();

  for (; !inSpace.IsAtEnd() && !outSpace.IsAtEnd(); inSpace.NextSpan(), outSpace.NextSpan())
  {
    int inIdx = inSpace.BeginSpan();
    int outIdx = outSpace.BeginSpan();
    for (; inIdx < inSpace.EndSpan() && outIdx < outSpace.EndSpan(); inIdx++, outIdx++)
    {
      outArray->SetTuple(outIdx, inIdx, inArray);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkAppendRectilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
