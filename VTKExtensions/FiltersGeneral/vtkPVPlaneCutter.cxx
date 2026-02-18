// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVPlaneCutter.h"

#include "vtkAMRCutPlane.h"
#include "vtkAMRSliceFilter.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkHyperTreeGridPlaneCutter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPlane.h"
#include "vtkPlaneCutter.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPlaneCutter);

//----------------------------------------------------------------------------
vtkPVPlaneCutter::vtkPVPlaneCutter() = default;

//----------------------------------------------------------------------------
vtkPVPlaneCutter::~vtkPVPlaneCutter() = default;

//----------------------------------------------------------------------------
void vtkPVPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dual: " << this->Dual << endl;
  os << indent << "LevelOfResolution: " << this->LevelOfResolution << endl;
  os << indent << "UseNativeCutter: " << (this->UseNativeCutter ? "On" : "Off") << endl;
}

//----------------------------------------------------------------------------
int vtkPVPlaneCutter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  vtkPlane* plane = vtkPlane::SafeDownCast(this->Plane);
  if (!plane)
  {
    vtkErrorMacro(<< "Failed to get plane.");
    return 0;
  }

  if (auto inHyperTreeGrid = vtkHyperTreeGrid::SafeDownCast(input))
  {
    double* normal = plane->GetNormal();
    this->HTGPlaneCutter->SetPlane(normal[0], normal[1], normal[2], -plane->FunctionValue(0, 0, 0));
    this->HTGPlaneCutter->SetDual(this->GetDual());
    this->HTGPlaneCutter->SetInputData(inHyperTreeGrid);
    this->HTGPlaneCutter->Update();
    output->ShallowCopy(this->HTGPlaneCutter->GetOutput());
    return 1;
  }
  else if (auto inOverlappingAMR = vtkOverlappingAMR::SafeDownCast(input))
  {
    double* normal = plane->GetNormal();
    double* origin = plane->GetOrigin();
    this->AMRPlaneCutter->SetInitialRequest(false);
    this->AMRPlaneCutter->SetNormal(normal);
    this->AMRPlaneCutter->SetCenter(origin);
    this->AMRPlaneCutter->SetLevelOfResolution(this->GetLevelOfResolution());
    this->AMRPlaneCutter->SetUseNativeCutter(this->GetUseNativeCutter());
    this->AMRPlaneCutter->SetInputData(inOverlappingAMR);
    this->AMRPlaneCutter->Update();
    vtkMultiBlockDataSet::SafeDownCast(output)->CompositeShallowCopy(
      this->AMRPlaneCutter->GetOutput());
    return 1;
  }
  // Not dealing with hyper tree grids, we execute RequestData of vktCutter
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVPlaneCutter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVPlaneCutter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVPlaneCutter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVPlaneCutter::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  int outputType = -1;

  vtkPlane* plane = vtkPlane::SafeDownCast(this->GetPlane());
  if (!plane)
  {
    vtkErrorMacro(<< "No plane function");
    return 0;
  }

  if (vtkHyperTreeGrid::SafeDownCast(inputDO))
  {
    outputType = VTK_POLY_DATA;
  }
  else if (vtkOverlappingAMR::SafeDownCast(inputDO))
  {
    outputType = VTK_MULTIBLOCK_DATA_SET;
  }
  if (outputType != -1)
  {
    return vtkDataObjectAlgorithm::SetOutputDataObject(
             outputType, outputVector->GetInformationObject(0), /*exact*/ true)
      ? 1
      : 0;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}
