/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCutter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCutter.h"

#include "vtkAppendFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkHyperTreeGridPlaneCutter.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPlane.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

vtkStandardNewMacro(vtkPVCutter);

//----------------------------------------------------------------------------
vtkPVCutter::vtkPVCutter()
{
  this->SetNumberOfOutputPorts(1);
  this->Dual = false;
}

//----------------------------------------------------------------------------
vtkPVCutter::~vtkPVCutter()
{
}

//----------------------------------------------------------------------------
void vtkPVCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVCutter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro(<< "Failed to get input information.");
    return 0;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro(<< "Failed to get output information.");
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  vtkHyperTreeGrid* inHyperTreeGrid = vtkHyperTreeGrid::SafeDownCast(inDataObj);
  if (inHyperTreeGrid)
  {
    vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->CutFunction);
    if (plane && plane->GetAxisAligned())
    {
      vtkNew<vtkHyperTreeGridAxisCut> axisCutter;
      double* normal = plane->GetNormal();
      axisCutter->SetPlanePosition(-plane->EvaluateFunction(0, 0, 0));
      int planeNormalAxis = 0;
      if (normal[1] > normal[0])
      {
        planeNormalAxis = 1;
      }
      if (normal[2] > normal[0])
      {
        planeNormalAxis = 2;
      }
      axisCutter->SetPlanePosition(-plane->EvaluateFunction(0, 0, 0));
      axisCutter->SetPlaneNormalAxis(planeNormalAxis);
      axisCutter->SetInputData(0, inHyperTreeGrid);

      axisCutter->Update();
      outDataObj->ShallowCopy(axisCutter->GetOutput(0));
      return 1;
    }
    else if (plane)
    {
      vtkNew<vtkHyperTreeGridPlaneCutter> planeCutter;
      double* normal = plane->GetNormal();
      planeCutter->SetPlane(normal[0], normal[1], normal[2], -plane->EvaluateFunction(0, 0, 0));
      planeCutter->SetDual(this->GetDual());
      planeCutter->SetInputData(0, inHyperTreeGrid);

      planeCutter->Update();
      outDataObj->ShallowCopy(planeCutter->GetOutput(0));
      return 1;
    }
    return 0;
  }
  // Not dealing with hyper tree grids, we execute RequestData of vktCutter
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVCutter::ProcessRequest(
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
int vtkPVCutter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  vtkInformationStringVectorKey::SafeDownCast(
    info->GetKey(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))
    ->Append(info, "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVCutter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVCutter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  if (!this->GetCutFunction())
  {
    vtkErrorMacro(<< "No cut function");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkHyperTreeGrid::SafeDownCast(inInfo))
  {
    vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->GetCutFunction());
    if (!plane)
    {
      vtkErrorMacro(<< "Cut function not supported for vtkHyperTreeGrid");
      return 0;
    }
    if (plane->GetAxisAligned())
    {
      vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outInfo);
      if (!output)
      {
        output = vtkHyperTreeGrid::New();
        outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
        output->FastDelete();
      }
    }
    else
    {
      vtkPolyData* output = vtkPolyData::GetData(outInfo);
      if (!output)
      {
        output = vtkPolyData::New();
        outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
        output->FastDelete();
      }
    }
    return 1;
  }
  else if (vtkDataSet::SafeDownCast(inInfo))
  {
    vtkPolyData* output = vtkPolyData::GetData(outInfo);
    if (!output)
    {
      output = vtkPolyData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  return 0;
}
