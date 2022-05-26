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

#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPlane.h"
#include "vtkPVPlaneCutter.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGridBase.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCutter);

//----------------------------------------------------------------------------
vtkPVCutter::vtkPVCutter() = default;

//----------------------------------------------------------------------------
vtkPVCutter::~vtkPVCutter() = default;

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
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->CutFunction);
  if (plane && this->GetNumberOfContours() == 1 && this->GetGenerateCutScalars() == 0)
  {
    if (vtkHyperTreeGrid::SafeDownCast(input))
    {
      if (this->Locator == nullptr)
      {
        this->CreateDefaultLocator();
      }
      this->PlaneCutter->SetInputData(input);
      this->PlaneCutter->SetPlane(plane);
      this->PlaneCutter->SetGeneratePolygons(!this->GetGenerateTriangles());
      this->PlaneCutter->BuildTreeOff();
      bool mergePoints =
        this->GetLocator() && !this->GetLocator()->IsA("vtkNonMergingPointLocator");
      this->PlaneCutter->SetMergePoints(mergePoints);
      this->PlaneCutter->SetDual(this->GetDual());
      this->PlaneCutter->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
      this->PlaneCutter->ComputeNormalsOff();
      this->PlaneCutter->Update();
      output->ShallowCopy(this->PlaneCutter->GetOutput());
      return 1;
    }
    else
    {
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }
  }
  else
  {
    // vtkHyperTreeGrid support only plane as cut function
    if (vtkHyperTreeGrid::SafeDownCast(input))
    {
      vtkErrorMacro(<< input->GetClassName()
                    << " doesn't support the cut function: " << this->CutFunction->GetClassName());
      return 0;
    }
    else
    {
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }
  }
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
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
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
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  auto inputDO = vtkDataObject::GetData(inInfo);
  if (!this->GetCutFunction())
  {
    vtkErrorMacro(<< "No cut function");
    return 0;
  }

  if (vtkHyperTreeGrid::SafeDownCast(inputDO))
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
  else if (vtkDataSet::SafeDownCast(inputDO))
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
