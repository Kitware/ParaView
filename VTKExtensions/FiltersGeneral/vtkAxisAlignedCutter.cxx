// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisAlignedCutter.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPVPlane.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxisAlignedCutter);

//----------------------------------------------------------------------------
vtkAxisAlignedCutter::vtkAxisAlignedCutter() = default;

//----------------------------------------------------------------------------
vtkAxisAlignedCutter::~vtkAxisAlignedCutter() = default;

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->CutFunction->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
vtkMTimeType vtkAxisAlignedCutter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->CutFunction != nullptr)
  {
    vtkMTimeType planeMTime = this->CutFunction->GetMTime();
    return (planeMTime > mTime ? planeMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::SetOffsetValue(int i, double value)
{
  this->OffsetValues->SetValue(i, value);
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkAxisAlignedCutter::GetOffsetValue(int i)
{
  return this->OffsetValues->GetValue(i);
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::SetNumberOfOffsetValues(int number)
{
  this->OffsetValues->SetNumberOfContours(number);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::GetNumberOfOffsetValues()
{
  return this->OffsetValues->GetNumberOfContours();
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkHyperTreeGrid* htgInput = vtkHyperTreeGrid::GetData(inInfo);
  if (htgInput)
  {
    // Input dataset support can be stored in a composite output (multi-slice available)
    // so output is a vtkPartitionedDataSetCollection of vtkHyperTreeGrid
    bool outputAlreadyCreated = vtkPartitionedDataSetCollection::GetData(outInfo);
    if (!outputAlreadyCreated)
    {
      vtkNew<vtkPartitionedDataSetCollection> outputPdc;
      this->GetExecutive()->SetOutputData(0, outputPdc);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), outputPdc->GetExtentType());
    }
    return 1;
  }

  vtkOverlappingAMR* amrInput = vtkOverlappingAMR::GetData(inInfo);
  if (amrInput)
  {
    // Input is an AMR and can't be stored in a vtkPartitionedDataSetCollection,
    // so output is a vtkOverlappingAMR (single slice support only)
    bool outputAlreadyCreated = vtkOverlappingAMR::GetData(outInfo);
    if (!outputAlreadyCreated)
    {
      vtkNew<vtkOverlappingAMR> amrOutput;
      this->GetExecutive()->SetOutputData(0, amrOutput);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), amrOutput->GetExtentType());
    }
    return 1;
  }

  vtkCompositeDataSet* compositeInput = vtkCompositeDataSet::GetData(inInfo);
  if (compositeInput)
  {
    vtkErrorMacro("This filter can't process composite data other than vtkOverlappingAMR.");
    return 0;
  }

  vtkErrorMacro("Unable to retrieve input as vtkOverlappingAMR or vtkHyperTreeGrid.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->CutFunction);
  if (plane && plane->GetAxisAligned())
  {
    vtkHyperTreeGrid* htgInput = vtkHyperTreeGrid::GetData(inInfo);
    if (htgInput)
    {
      // vtkHyperTreeGrid support multi-slice, result is stored in a vtkPartitionedDataSetCollection
      vtkPartitionedDataSetCollection* pdcOutput =
        vtkPartitionedDataSetCollection::GetData(outInfo);
      if (!pdcOutput)
      {
        vtkErrorMacro(<< "Unable to retrieve output as vtkPartitionedDataSetCollection.");
        return 0;
      }

      pdcOutput->SetNumberOfPartitionedDataSets(this->GetNumberOfOffsetValues());

      for (int i = 0; i < this->GetNumberOfOffsetValues(); i++)
      {
        vtkNew<vtkHyperTreeGrid> htgOutput;
        double offset = this->GetOffsetValue(i);

        this->CutHTGWithAAPlane(htgInput, htgOutput, plane, offset);

        vtkNew<vtkPartitionedDataSet> pds;
        pds->SetNumberOfPartitions(1);
        pds->SetPartition(0, htgOutput);
        pdcOutput->SetPartitionedDataSet(i, pds);
      }
      return 1;
    }

    vtkOverlappingAMR* amrInput = vtkOverlappingAMR::GetData(inInfo);
    if (amrInput)
    {
      // vtkOverlappingAMR only support one slice
      vtkOverlappingAMR* amrOutput = vtkOverlappingAMR::GetData(outInfo);
      if (!amrOutput)
      {
        vtkErrorMacro(<< "Unable to retrieve output as vtkOverlappingAMR.");
        return 0;
      }

      this->CutAMRWithAAPlane(amrInput, amrOutput, plane);
      return 1;
    }

    vtkErrorMacro(
      "Wrong input type, expected to be a vtkHyperTreeGrid or vtkOverlappingAMR instance.");
    return 0;
  }

  vtkErrorMacro("Unable to retrieve valid axis-aligned implicit function to cut with.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::CutHTGWithAAPlane(
  vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, vtkPVPlane* plane, double offset)
{
  double normal[3] = { 0., 0., 0. };
  plane->GetNormal(normal);

  int planeNormalAxis = 0;
  if (normal[1] > normal[0])
  {
    planeNormalAxis = 1;
  }
  if (normal[2] > normal[0])
  {
    planeNormalAxis = 2;
  }

  vtkNew<vtkPVPlane> newPlane;
  newPlane->SetNormal(plane->GetNormal());
  newPlane->SetOrigin(plane->GetOrigin());
  // We should not use `Push` here since it does not apply on
  // the internal plane of vtkPVPlane
  newPlane->SetOffset(plane->GetOffset() + offset);

  this->HTGCutter->SetPlanePosition(-newPlane->EvaluateFunction(0, 0, 0));
  this->HTGCutter->SetPlaneNormalAxis(planeNormalAxis);
  this->HTGCutter->SetInputData(input);
  this->HTGCutter->Update();

  output->ShallowCopy(this->HTGCutter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::CutAMRWithAAPlane(
  vtkOverlappingAMR* input, vtkOverlappingAMR* output, vtkPVPlane* plane)
{
  double* normal = plane->GetNormal();
  double* origin = plane->GetOrigin();
  int planeNormalAxis = vtkAMRSliceFilter::X_NORMAL;
  if (normal[1] > normal[0])
  {
    planeNormalAxis = vtkAMRSliceFilter::Y_NORMAL;
  }
  if (normal[2] > normal[0])
  {
    planeNormalAxis = vtkAMRSliceFilter::Z_NORMAL;
  }

  double bounds[6] = { 0. };
  input->GetBounds(bounds);

  double offset = 0;
  switch (planeNormalAxis)
  {
    case vtkAMRSliceFilter::X_NORMAL:
      offset = origin[0] - bounds[0];
      break;
    case vtkAMRSliceFilter::Y_NORMAL:
      offset = origin[1] - bounds[2];
      break;
    case vtkAMRSliceFilter::Z_NORMAL:
    default:
      offset = origin[2] - bounds[4];
      break;
  }

  this->AMRCutter->SetOffsetFromOrigin(plane->GetOffset() + offset);
  this->AMRCutter->SetNormal(planeNormalAxis);
  this->AMRCutter->SetMaxResolution(this->LevelOfResolution);
  this->AMRCutter->SetInputData(input);
  this->AMRCutter->Update();

  output->ShallowCopy(this->AMRCutter->GetOutput());
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");

    // We cannot support composite inputs since this algorithm can produce a
    // vtkPartitionedDatasetCollection instance.
    // We add this type as supported type to avoid the vtkCompositeDataPipeline to
    // handle it automatically (and we treat this case in RequestDataObject).
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    return 1;
  }
  return 0;
}
