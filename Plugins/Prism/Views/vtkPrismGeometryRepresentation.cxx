// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPrismGeometryRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBox.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkExtractGeometry.h"
#include "vtkFieldData.h"
#include "vtkGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPrismGeometryConverter.h"
#include "vtkPrismView.h"
#include "vtkSimulationPointCloudFilter.h"
#include "vtkSimulationToPrismFilter.h"
#include "vtkStringArray.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPrismGeometryRepresentation);

//------------------------------------------------------------------------------
vtkPrismGeometryRepresentation::vtkPrismGeometryRepresentation()
{
  this->SetXAxisName(nullptr);
  this->SetYAxisName(nullptr);
  this->SetZAxisName(nullptr);
  vtkMath::UninitializeBounds(this->NonSimulationDataInputBounds);
  this->SetupDefaults();
}

//------------------------------------------------------------------------------
vtkPrismGeometryRepresentation::~vtkPrismGeometryRepresentation()
{
  this->SetXAxisName(nullptr);
  this->SetYAxisName(nullptr);
  this->SetZAxisName(nullptr);
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetupDefaults()
{
  this->Superclass::SetupDefaults();

  this->ThresholdFilter->ExtractInsideOn();
  this->ThresholdFilter->ExtractBoundaryCellsOn();

  this->MultiBlockMaker->SetInputConnection(this->GeometryConverter->GetOutputPort());
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetIsSimulationData(bool isSimulationData)
{
  if (this->IsSimulationData != isSimulationData)
  {
    this->IsSimulationData = isSimulationData;
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetAttributeType(int type)
{
  if (this->SimulationToPrismFilter->GetAttributeType() != type)
  {
    this->SimulationPointCloudFilter->SetAttributeType(type);
    this->SimulationToPrismFilter->SetAttributeType(type);
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
int vtkPrismGeometryRepresentation::GetAttributeType()
{
  return this->SimulationToPrismFilter->GetAttributeType();
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetXArrayName(const char* name)
{
  if (!this->SimulationToPrismFilter->GetXArrayName() || !name ||
    strcmp(this->SimulationToPrismFilter->GetXArrayName(), name) != 0)
  {
    this->SimulationToPrismFilter->SetXArrayName(name);
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
const char* vtkPrismGeometryRepresentation::GetXArrayName()
{
  return this->SimulationToPrismFilter->GetXArrayName();
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetYArrayName(const char* name)
{
  if (!this->SimulationToPrismFilter->GetYArrayName() || !name ||
    strcmp(this->SimulationToPrismFilter->GetYArrayName(), name) != 0)
  {
    this->SimulationToPrismFilter->SetYArrayName(name);
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
const char* vtkPrismGeometryRepresentation::GetYArrayName()
{
  return this->SimulationToPrismFilter->GetYArrayName();
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetZArrayName(const char* name)
{
  if (!this->SimulationToPrismFilter->GetZArrayName() || !name ||
    strcmp(this->SimulationToPrismFilter->GetZArrayName(), name) != 0)
  {
    this->SimulationToPrismFilter->SetZArrayName(name);
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
const char* vtkPrismGeometryRepresentation::GetZArrayName()
{
  return this->SimulationToPrismFilter->GetZArrayName();
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::SetEnableThresholding(bool enableThresholding)
{
  if (this->EnableThresholding != enableThresholding)
  {
    this->EnableThresholding = enableThresholding;
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
void vtkPrismGeometryRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IsSimulationData: " << this->IsSimulationData << endl;
  os << indent << "EnableThresholding: " << this->EnableThresholding << endl;
  os << indent << "NonSimulationDataInputBounds: " << this->NonSimulationDataInputBounds[0] << ", "
     << this->NonSimulationDataInputBounds[1] << ", " << this->NonSimulationDataInputBounds[2]
     << ", " << this->NonSimulationDataInputBounds[3] << ", "
     << this->NonSimulationDataInputBounds[4] << ", " << this->NonSimulationDataInputBounds[5]
     << endl;
}

//----------------------------------------------------------------------------
int vtkPrismGeometryRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto prismView = vtkPrismView::SafeDownCast(this->GetView());
  if (prismView &&
    prismView->GetRequestDataMode() == vtkPrismView::RequestDataModes::REQUEST_BOUNDS)
  {
    if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
      vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
      // get bounds
      if (auto ds = vtkDataSet::SafeDownCast(input))
      {
        ds->GetBounds(this->NonSimulationDataInputBounds);
      }
      else if (auto cds = vtkCompositeDataSet::SafeDownCast(input))
      {
        cds->GetBounds(this->NonSimulationDataInputBounds);
      }
      else
      {
        vtkMath::UninitializeBounds(this->NonSimulationDataInputBounds);
      }

      // set view axis if available
      auto xTitle = vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("XTitle"));
      auto yTitle = vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("YTitle"));
      auto zTitle = vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("ZTitle"));
      if (xTitle && yTitle && zTitle)
      {
        this->SetXAxisName(xTitle->GetValue(0).c_str());
        this->SetYAxisName(yTitle->GetValue(0).c_str());
        this->SetZAxisName(zTitle->GetValue(0).c_str());
      }
      else
      {
        this->SetXAxisName(nullptr);
        this->SetYAxisName(nullptr);
        this->SetZAxisName(nullptr);
      }
    }
    return 1;
  }
  else
  {
    if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
      if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
      {
        vtkAlgorithmOutput* aout = this->GetInternalOutputPort();
        vtkPVTrivialProducer* prod = vtkPVTrivialProducer::SafeDownCast(aout->GetProducer());
        if (prod)
        {
          prod->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
        }
      }
      if (this->IsSimulationData)
      {
        this->SimulationPointCloudFilter->SetInputConnection(this->GetInternalOutputPort());
        this->SimulationToPrismFilter->SetInputConnection(
          this->SimulationPointCloudFilter->GetOutputPort());
        this->GeometryFilter->SetInputConnection(this->SimulationToPrismFilter->GetOutputPort());
      }
      else
      {
        this->GeometryFilter->SetInputConnection(this->GetInternalOutputPort());
      }
      if (this->EnableThresholding)
      {
        this->ThresholdFilter->SetInputConnection(this->GeometryFilter->GetOutputPort());
        this->ThresholdGeometryFilter->SetInputConnection(this->ThresholdFilter->GetOutputPort());
        this->GeometryConverter->SetInputConnection(this->ThresholdGeometryFilter->GetOutputPort());
      }
      else
      {
        this->GeometryConverter->SetInputConnection(this->GeometryFilter->GetOutputPort());
      }
    }
    else
    {
      vtkNew<vtkMultiBlockDataSet> placeholder;
      this->GeometryConverter->SetInputDataObject(0, placeholder);
    }

    // essential to re-execute geometry filter consistently on all ranks since it
    // does use parallel communication (see #19963).
    this->GeometryFilter->Modified();
    this->MultiBlockMaker->Update();
    return this->vtkPVDataRepresentation::RequestData(request, inputVector, outputVector);
  }
}

//------------------------------------------------------------------------------
int vtkPrismGeometryRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  // REQUEST_BOUNDS() is basically a REQUEST_UPDATE() in which
  // vtkPrismGeometryRepresentation extract the input bounds for non simulation data
  if (request_type == vtkPrismView::REQUEST_BOUNDS() && !this->GetIsSimulationData())
  {
    auto prismView = vtkPrismView::SafeDownCast(this->GetView());
    if (prismView)
    {
      auto executive = vtkPVDataRepresentationPipeline::SafeDownCast(this->GetExecutive());
      // If this->Update() will be called, we need to call MarkModified again
      // to ensure that update will be called again after getting the bounds
      bool callMarkModified = executive->GetNeedsUpdate() && !prismView->IsCached(this);

      this->vtkPVDataRepresentation::ProcessViewRequest(
        vtkPVView::REQUEST_UPDATE(), inInfo, outInfo);

      // set view axis if available
      if (this->GetXAxisName() && this->GetYAxisName() && this->GetZAxisName())
      {
        prismView->SetXAxisName(this->GetXAxisName());
        prismView->SetYAxisName(this->GetYAxisName());
        prismView->SetZAxisName(this->GetZAxisName());
      }
      if (callMarkModified)
      {
        this->MarkModified();
      }
    }
    return 1;
  }
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    auto prismView = vtkPrismView::SafeDownCast(this->GetView());

    // Propagate parameters from the prism view to the prism representation.
    if (prismView)
    {
      bool callMarkModified = false;
      this->SetEnableThresholding(prismView->GetEnableThresholding());
      if (this->EnableThresholding)
      {
        auto box = vtkBox::SafeDownCast(this->ThresholdFilter->GetImplicitFunction());
        if (!box)
        {
          vtkNew<vtkBox> newBox;
          newBox->SetBounds(prismView->GetLowerThresholdX(), prismView->GetUpperThresholdX(),
            prismView->GetLowerThresholdY(), prismView->GetUpperThresholdY(),
            prismView->GetLowerThresholdZ(), prismView->GetUpperThresholdZ());
          this->ThresholdFilter->SetImplicitFunction(newBox);
          callMarkModified = true;
        }
        else
        {
          auto mTimeBefore = box->GetMTime();
          box->SetBounds(prismView->GetLowerThresholdX(), prismView->GetUpperThresholdX(),
            prismView->GetLowerThresholdY(), prismView->GetUpperThresholdY(),
            prismView->GetLowerThresholdZ(), prismView->GetUpperThresholdZ());
          auto mTimeAfter = box->GetMTime();
          callMarkModified = mTimeAfter > mTimeBefore;
        }
      }
      auto mTimeBefore = this->GeometryConverter->GetMTime();
      this->GeometryConverter->SetPrismBounds(prismView->GetPrismBounds());
      this->GeometryConverter->SetAspectRatio(prismView->GetAspectRatio());
      this->GeometryConverter->SetLogScaleX(prismView->GetLogScaleX());
      this->GeometryConverter->SetLogScaleY(prismView->GetLogScaleY());
      this->GeometryConverter->SetLogScaleZ(prismView->GetLogScaleZ());
      auto mTimeAfter = this->GeometryConverter->GetMTime();
      callMarkModified |= mTimeAfter > mTimeBefore;
      if (callMarkModified)
      {
        this->MarkModified();
      }
    }
  }
  return this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
}
