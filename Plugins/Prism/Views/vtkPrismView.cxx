// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPrismView.h"

#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkInformationRequestKey.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVGridAxes3DActor.h"
#include "vtkPVLODActor.h"
#include "vtkPVSession.h"
#include "vtkPrismGeometryRepresentation.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkWidgetRepresentation.h"

#include <cmath>
#ifndef M_E
#define M_E 2.7182818284590452354 /* e */
#endif

//----------------------------------------------------------------------------
vtkInformationKeyMacro(vtkPrismView, REQUEST_BOUNDS, Request);

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPrismView);

//------------------------------------------------------------------------------
vtkPrismView::vtkPrismView()
{
  this->SetXAxisName(nullptr);
  this->SetYAxisName(nullptr);
  this->SetZAxisName(nullptr);
}

//------------------------------------------------------------------------------
vtkPrismView::~vtkPrismView()
{
  this->SetXAxisName(nullptr);
  this->SetYAxisName(nullptr);
  this->SetZAxisName(nullptr);
}

//------------------------------------------------------------------------------
void vtkPrismView::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EnableThresholding: " << (this->EnableThresholding ? "On" : "Off") << endl;
  os << indent << "Threshold Bounds: " << this->LowerThresholdX << " " << this->UpperThresholdX
     << " " << this->LowerThresholdX << " " << this->UpperThresholdX << " " << this->LowerThresholdX
     << " " << this->UpperThresholdX << endl;
  os << indent << "LogScaleX: " << (this->LogScaleX ? "On" : "Off") << endl;
  os << indent << "LogScaleY: " << (this->LogScaleY ? "On" : "Off") << endl;
  os << indent << "LogScaleZ: " << (this->LogScaleZ ? "On" : "Off") << endl;
  os << indent << "AspectRatio: " << this->AspectRatio[0] << ", " << this->AspectRatio[1] << ", "
     << this->AspectRatio[2] << endl;
  os << indent << "RequestDataMode: " << (int)this->RequestDataMode << endl;
  os << indent << "PrismBounds: " << this->PrismBounds[0] << ", " << this->PrismBounds[1] << ", "
     << this->PrismBounds[2] << ", " << this->PrismBounds[3] << ", " << this->PrismBounds[4] << ", "
     << this->PrismBounds[5] << endl;
  os << indent << "EnableNonSimulationDataSelection: "
     << (this->EnableNonSimulationDataSelection ? "On" : "Off") << endl;
}

//------------------------------------------------------------------------------
namespace
{
double powExp(double x)
{
  return std::pow(M_E, x);
};
}

//------------------------------------------------------------------------------
void vtkPrismView::AboutToRenderOnLocalProcess(bool interactive)
{
  if (this->GridAxes3DActor)
  {
    this->GridAxes3DActor->UseModelTransformOn();
    double prismBounds[6];
    std::copy_n(this->PrismBounds, 6, prismBounds);
    if (this->LogScaleX)
    {
      prismBounds[0] = prismBounds[0] > 0 ? std::log(prismBounds[0]) : 0;
      prismBounds[1] = prismBounds[1] > 0 ? std::log(prismBounds[1]) : 0;
    }
    if (this->LogScaleY)
    {
      prismBounds[2] = prismBounds[2] > 0 ? std::log(prismBounds[2]) : 0;
      prismBounds[3] = prismBounds[3] > 0 ? std::log(prismBounds[3]) : 0;
    }
    if (this->LogScaleZ)
    {
      prismBounds[4] = prismBounds[4] > 0 ? std::log(prismBounds[4]) : 0;
      prismBounds[5] = prismBounds[5] > 0 ? std::log(prismBounds[5]) : 0;
    }
    double scale[3] = { 1.0, 1.0, 1.0 };
    if (this->AspectRatio[0] > 0)
    {
      scale[0] = (prismBounds[1] - prismBounds[0]) / this->AspectRatio[0];
    }
    if (this->AspectRatio[1] > 0)
    {
      scale[1] = (prismBounds[3] - prismBounds[2]) / this->AspectRatio[1];
    }
    if (this->AspectRatio[2] > 0)
    {
      scale[2] = (prismBounds[5] - prismBounds[4]) / this->AspectRatio[2];
    }
    double dataPosition[3] = { -prismBounds[0], -prismBounds[2], -prismBounds[4] };

    // compute the grid bounds
    vtkBoundingBox bbox(this->GridAxes3DActor->GetTransformedBounds());
    bbox.Scale(scale[0], scale[1], scale[2]);
    double bds[6];
    bbox.GetBounds(bds);
    bds[0] -= dataPosition[0];
    bds[1] -= dataPosition[0];
    bds[2] -= dataPosition[1];
    bds[3] -= dataPosition[1];
    bds[4] -= dataPosition[2];
    bds[5] -= dataPosition[2];
    this->GridAxes3DActor->SetModelBounds(bds);

    // compute the actor's transform
    vtkNew<vtkTransform> transform;
    transform->Identity();
    transform->PreMultiply();
    transform->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
    transform->Translate(dataPosition);
    transform->Update();
    this->GridAxes3DActor->SetModelTransformMatrix(transform->GetMatrix()->GetData());

    // Set Label function
    if (this->LogScaleX)
    {
      this->GridAxes3DActor->SetTickLabelFunction(0, ::powExp);
    }
    else
    {
      this->GridAxes3DActor->SetTickLabelFunction(0, nullptr);
    }
    if (this->LogScaleY)
    {
      this->GridAxes3DActor->SetTickLabelFunction(1, ::powExp);
    }
    else
    {
      this->GridAxes3DActor->SetTickLabelFunction(1, nullptr);
    }
    if (this->LogScaleZ)
    {
      this->GridAxes3DActor->SetTickLabelFunction(2, ::powExp);
    }
    else
    {
      this->GridAxes3DActor->SetTickLabelFunction(2, nullptr);
    }
  }
  this->Superclass::AboutToRenderOnLocalProcess(interactive);
}

//----------------------------------------------------------------------------
void vtkPrismView::SynchronizeGeometryBounds()
{
  // to allow interactive change of threshold bounds, we need to only consider
  // the widget bounds when this->GetRenderer()->ComputeVisiblePropBounds(prop_bounds)
  // is called inside this->Superclass::SynchronizeGeometryBounds()
  std::vector<vtkProp*> propsToHide;
  if (this->GetLocalProcessDoesRendering(/*using_distributed_rendering*/ false))
  {
    vtkProp* prop;
    auto props = this->GetRenderer()->GetViewProps();
    vtkCollectionSimpleIterator pit;
    for (props->InitTraversal(pit); (prop = props->GetNextProp(pit));)
    {

      if (prop && !vtkWidgetRepresentation::SafeDownCast(prop) && prop->GetVisibility() &&
        prop->GetUseBounds())
      {
        prop->SetUseBounds(0);
        propsToHide.push_back(prop);
      }
    }
  }

  this->Superclass::SynchronizeGeometryBounds();

  for (auto prop : propsToHide)
  {
    prop->SetUseBounds(1);
  }
}

//------------------------------------------------------------------------------
void vtkPrismView::AllReduceString(
  const std::string& axisNameSource, std::string& axisNameDestination)
{
  auto session = this->GetSession();
  assert(session);

  std::string axisName = axisNameSource;

  auto pController = vtkMultiProcessController::GetGlobalController();
  // no need to perform reduce because the first process of the pController will have the data

  auto cController = session->GetController(vtkPVSession::CLIENT);
  if (cController)
  {
    assert(pController == nullptr || pController->GetLocalProcessId() == 0);
    int size = static_cast<int>(axisName.size()) + 1;
    std::vector<char> axisNameBuffer(axisName.data(), axisName.data() + size);
    cController->Send(&size, 1, 1, 41236);
    cController->Send(axisNameBuffer.data(), size, 1, 41237);
  }

  auto crController = session->GetController(vtkPVSession::RENDER_SERVER_ROOT);
  auto cdController = session->GetController(vtkPVSession::DATA_SERVER_ROOT);
  if (crController == cdController)
  {
    cdController = nullptr;
  }

  if (crController)
  {
    int size;
    crController->Receive(&size, 1, 1, 41236);
    std::vector<char> axisNameBuffer(axisName.data(), axisName.data() + size);
    crController->Receive(axisNameBuffer.data(), size, 1, 41237);
    if (axisNameBuffer.size() > 1)
    {
      axisName = axisNameBuffer.data();
    }
  }

  if (cdController)
  {
    int size;
    cdController->Receive(&size, 1, 1, 41236);
    std::vector<char> axisNameBuffer(axisName.data(), axisName.data() + size);
    cdController->Receive(axisNameBuffer.data(), size, 1, 41237);
    if (axisNameBuffer.size() > 1)
    {
      axisName = axisNameBuffer.data();
    }
  }

  if (pController)
  {
    // the first process is the one that holds the axis name information
    // first get the size of the string and broadcast it
    int size;
    if (pController->GetLocalProcessId() == 0)
    {
      size = static_cast<int>(axisName.size()) + 1; // +1 for \0
    }
    pController->Broadcast(&size, 1, 0);
    // now broadcast the string
    std::vector<char> buffer(size);
    if (pController->GetLocalProcessId() == 0)
    {
      std::copy(axisName.begin(), axisName.end(), buffer.begin());
      buffer[size - 1] = '\0';
    }
    pController->Broadcast(buffer.data(), size, 0);
    axisName = buffer.data();
  }

  axisNameDestination = axisName;
}

//------------------------------------------------------------------------------
void vtkPrismView::Update()
{
  // first call update data on all vtkPrismGeometryRepresentation representations to compute
  // the input bounds of non simulation data, a.k.a. prism bounds and axis name (if available)
  this->SetXAxisName(nullptr);
  this->SetYAxisName(nullptr);
  this->SetZAxisName(nullptr);
  this->PrismBoundsBBox.Reset();
  this->RequestDataMode = RequestDataModes::REQUEST_BOUNDS;
  this->CallProcessViewRequest(
    vtkPrismView::REQUEST_BOUNDS(), this->RequestInformation, this->ReplyInformationVector);

  // aggregate the bounds of the non simulation data. This done here instead inside
  // ProcessViewRequest, because it will only process visible representations, but we want to
  // always have the bounds of the prism surface as a reference, not only when it's visible.
  for (int cc = 0, numReprs = this->GetNumberOfRepresentations(); cc < numReprs; cc++)
  {
    auto repr = vtkPrismGeometryRepresentation::SafeDownCast(this->GetRepresentation(cc));
    if (repr && !repr->GetIsSimulationData())
    {
      this->PrismBoundsBBox.AddBounds(repr->GetNonSimulationDataInputBounds());
    }
  }

  // synchronize the prism bounds
  vtkBoundingBox result;
  this->AllReduce(this->PrismBoundsBBox, result);
  if (result.IsValid())
  {
    this->PrismBoundsBBox = result;
  }
  else
  {
    this->PrismBoundsBBox.Reset();
  }
  double bounds[6];
  this->PrismBoundsBBox.GetBounds(bounds);
  if (!std::equal(bounds, bounds + 6, this->PrismBounds))
  {
    std::copy_n(bounds, 6, this->PrismBounds);
    this->Modified();
  }

  // synchronize the axis names
  std::string xAxisNameResult, yAxisNameResult, zAxisNameResult;
  this->AllReduceString(this->XAxisName ? this->XAxisName : "", xAxisNameResult);
  this->AllReduceString(this->YAxisName ? this->YAxisName : "", yAxisNameResult);
  this->AllReduceString(this->ZAxisName ? this->ZAxisName : "", zAxisNameResult);
  this->SetXAxisName(!xAxisNameResult.empty() ? xAxisNameResult.c_str() : "X Title");
  this->SetYAxisName(!yAxisNameResult.empty() ? yAxisNameResult.c_str() : "Y Title");
  this->SetZAxisName(!zAxisNameResult.empty() ? zAxisNameResult.c_str() : "Z Title");

  // now call update data on all representations to get the geometry data
  this->RequestDataMode = RequestDataModes::REQUEST_DATA;
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
bool vtkPrismView::PrepareSelect(int fieldAssociation, const char* array)
{
  if (!this->EnableNonSimulationDataSelection)
  {
    for (int i = 0; i < this->GetNumberOfRepresentations(); i++)
    {
      auto repr = vtkPrismGeometryRepresentation::SafeDownCast(this->GetRepresentation(i));
      if (repr)
      {
        auto prop = repr->GetRenderedProp();
        if (prop)
        {
          if (!repr->GetIsSimulationData() && repr->GetVisibility() && prop->GetPickable())
          {
            this->NonSimulationPropsToHideForSelection.push_back(prop);
            prop->SetPickable(false);
          }
        }
      }
    }
  }
  return this->Superclass::PrepareSelect(fieldAssociation, array);
}

//----------------------------------------------------------------------------
void vtkPrismView::PostSelect(vtkSelection* sel, const char* array)
{
  if (!this->EnableNonSimulationDataSelection)
  {
    for (auto prop : this->NonSimulationPropsToHideForSelection)
    {
      if (prop)
      {
        prop->SetPickable(true);
      }
    }
    this->NonSimulationPropsToHideForSelection.clear();
  }
  this->Superclass::PostSelect(sel, array);
}
