/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMContextViewProxy.h"

#include "vtkAxis.h"
#include "vtkChartLegend.h"
#include "vtkChartXY.h"
#include "vtkClientServerStream.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkEventForwarderCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewProxyInteractorHelper.h"
#include "vtkStructuredData.h"
#include "vtkWeakPointer.h"

vtkStandardNewMacro(vtkSMContextViewProxy);
//----------------------------------------------------------------------------
vtkSMContextViewProxy::vtkSMContextViewProxy()
  : InteractorHelper()
{
  this->ChartView = nullptr;
  this->SkipPlotableCheck = false;
  this->InteractorHelper->SetViewProxy(this);
  this->EventForwarder->SetTarget(this);

  this->XYChartViewBase4Axes = false;
}

//----------------------------------------------------------------------------
vtkSMContextViewProxy::~vtkSMContextViewProxy()
{
  this->EventForwarder->SetTarget(nullptr);
  this->InteractorHelper->SetViewProxy(nullptr);
  this->InteractorHelper->CleanupInteractor();
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  if (this->GetLocalProcessSupportsInteraction())
  {
    this->CreateVTKObjects();
    vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(this->GetClientSideObject());
    if (vtkRenderWindowInteractor* oldiren = pvview->GetInteractor())
    {
      oldiren->GetInteractorStyle()->RemoveObserver(this->EventForwarder.Get());
    }

    // Remember, these calls end up changing ivars on iren.
    pvview->SetupInteractor(iren);
    this->InteractorHelper->SetupInteractor(pvview->GetInteractor());

    if (iren)
    {
      // Forward StartInteractionEvent and EndInteractionEvent from the proxy.
      iren->GetInteractorStyle()->AddObserver(
        vtkCommand::StartInteractionEvent, this->EventForwarder.Get());
      iren->GetInteractorStyle()->AddObserver(
        vtkCommand::EndInteractionEvent, this->EventForwarder.Get());
    }
  }
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkSMContextViewProxy::GetInteractor()
{
  this->CreateVTKObjects();
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(this->GetClientSideObject());
  return pvview ? pvview->GetInteractor() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go thurther...
  if (this->Location == 0)
  {
    return;
  }

  if (!this->ObjectsCreated)
  {
    return;
  }

  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(this->GetClientSideObject());
  this->ChartView = pvview->GetContextView();

  // if user interacts with the chart, we need to ensure that we set the axis
  // behaviors to "FIXED" so that the user chosen axis ranges are preserved in
  // state files, etc.
  this->GetContextItem()->AddObserver(
    vtkCommand::InteractionEvent, this, &vtkSMContextViewProxy::OnInteractionEvent);

  this->ChartView->GetScene()->AddObserver(
    vtkCommand::LeftButtonReleaseEvent, this, &vtkSMContextViewProxy::OnLeftButtonReleaseEvent);
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMContextViewProxy::GetRenderWindow()
{
  return this->ChartView->GetRenderWindow();
}

//----------------------------------------------------------------------------
vtkContextView* vtkSMContextViewProxy::GetContextView()
{
  return this->ChartView;
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkSMContextViewProxy::GetContextItem()
{
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(this->GetClientSideObject());
  return pvview ? pvview->GetContextItem() : nullptr;
}

//----------------------------------------------------------------------------
static void update_property(vtkAxis* axis, vtkSMProperty* propMin, vtkSMProperty* propMax)
{
  if (axis && propMin && propMax)
  {
    double range[2];
    axis->GetUnscaledRange(range);
    vtkSMPropertyHelper(propMin).Set(range[0]);
    vtkSMPropertyHelper(propMax).Set(range[1]);
  }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::CopyAxisRangesFromChart()
{
  vtkChartXY* chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
  {
    update_property(chartXY->GetAxis(vtkAxis::LEFT), this->GetProperty("LeftAxisRangeMinimum"),
      this->GetProperty("LeftAxisRangeMaximum"));
    update_property(chartXY->GetAxis(vtkAxis::BOTTOM), this->GetProperty("BottomAxisRangeMinimum"),
      this->GetProperty("BottomAxisRangeMaximum"));
    if (this->XYChartViewBase4Axes)
    {
      update_property(chartXY->GetAxis(vtkAxis::RIGHT), this->GetProperty("RightAxisRangeMinimum"),
        this->GetProperty("RightAxisRangeMaximum"));
      update_property(chartXY->GetAxis(vtkAxis::TOP), this->GetProperty("TopAxisRangeMinimum"),
        this->GetProperty("TopAxisRangeMaximum"));
    }

    // HACK: This overcomes a issue where we mark the chart modified, in Render.
    // We seems to be lacking a mechanism in vtkSMProxy to say "here's the
    // new property value, however it's already set on the server side too,
    // so no need to push it or mark pipelines dirty".
    int prev = this->InMarkModified;
    this->InMarkModified = 1;
    this->UpdateVTKObjects();
    this->InMarkModified = prev;
  }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::OnInteractionEvent()
{
  vtkChartXY* chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
  {
    // Charts by default update axes ranges as needed. On interaction, we force
    // the chart to preserve the user-selected ranges.
    this->CopyAxisRangesFromChart();
    vtkSMPropertyHelper(this, "LeftAxisUseCustomRange").Set(1);
    vtkSMPropertyHelper(this, "BottomAxisUseCustomRange").Set(1);
    if (this->XYChartViewBase4Axes)
    {
      vtkSMPropertyHelper(this, "RightAxisUseCustomRange").Set(1);
      vtkSMPropertyHelper(this, "TopAxisUseCustomRange").Set(1);
    }
    this->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::InteractionEvent);

    // Note: OnInteractionEvent gets called before this->StillRender() gets called
    // as a consequence of this interaction.
  }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::OnLeftButtonReleaseEvent()
{
  vtkChartXY* chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
  {
    int pos[2];
    pos[0] = static_cast<int>(chartXY->GetLegend()->GetPointVector().GetX());
    pos[1] = static_cast<int>(chartXY->GetLegend()->GetPointVector().GetY());
    vtkSMPropertyHelper(this, "LegendPosition", true).Set(pos, 2);
    this->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void vtkSMContextViewProxy::PostRender(bool interactive)
{
  // BUG# 14899. Ensure that the axis ranges are up-to-date after each render
  // ensuring that the property has the property values (similar in spirit to
  // the camera properties on the Render View).
  this->CopyAxisRangesFromChart();
  this->Superclass::PostRender(interactive);
}

//-----------------------------------------------------------------------------
void vtkSMContextViewProxy::ResetDisplay()
{
  vtkChartXY* chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
  {
    // simply unlock all the axes ranges. That results in the chart determine
    // new ranges to use in the Update call.
    vtkSMPropertyHelper(this, "LeftAxisUseCustomRange").Set(0);
    vtkSMPropertyHelper(this, "BottomAxisUseCustomRange").Set(0);
    if (this->XYChartViewBase4Axes)
    {
      vtkSMPropertyHelper(this, "RightAxisUseCustomRange").Set(0);
      vtkSMPropertyHelper(this, "TopAxisUseCustomRange").Set(0);
    }
    this->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
bool vtkSMContextViewProxy::CanDisplayData(vtkSMSourceProxy* producer, int outputPort)
{
  if (!this->Superclass::CanDisplayData(producer, outputPort))
  {
    return false;
  }

  if (this->SkipPlotableCheck)
  {
    // When SkipPlotableCheck is true, we only rely on the representation to
    // select whether it can accept the data produce by the producer. That check
    // happens in Superclass.
    return true;
  }

  if (producer->GetHints() && producer->GetHints()->FindNestedElementByName("Plotable"))
  {
    return true;
  }

  vtkPVDataInformation* dataInfo = producer->GetDataInformation(outputPort);
  if (dataInfo->DataSetTypeIsA("vtkTable"))
  {
    return true;
  }
  // also accept 1D structured datasets.
  if (dataInfo->DataSetTypeIsA("vtkImageData") || dataInfo->DataSetTypeIsA("vtkRectilinearGrid"))
  {
    int extent[6];
    dataInfo->GetExtent(extent);
    int temp[6] = { 0, 0, 0, 0, 0, 0 };
    int dimensionality =
      vtkStructuredData::GetDataDimension(vtkStructuredData::SetExtent(extent, temp));
    if (dimensionality == 1)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
const char* vtkSMContextViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  if (vtkPVXMLElement* hints = producer->GetHints())
  {
    // If the source has an hint as follows, then it's a text producer and must
    // be display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>
    for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
    {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      const char* childName = child->GetName();
      const char* childType = child->GetAttribute("type");
      if (childName && strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) && index == outputPort && childType)
      {
        if (strcmp(childType, "text") == 0)
        {
          return "ChartTextRepresentation";
        }
      }
    }
  }

  return this->Superclass::GetRepresentationType(producer, outputPort);
}

//----------------------------------------------------------------------------
vtkSelection* vtkSMContextViewProxy::GetCurrentSelection()
{
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(this->GetClientSideObject());
  return pvview ? pvview->GetSelection() : nullptr;
}

//----------------------------------------------------------------------------
int vtkSMContextViewProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  int tmp;
  if (element && element->GetScalarAttribute("skip_plotable_check", &tmp))
  {
    this->SkipPlotableCheck = (tmp == 1);
  }

  int ret = this->Superclass::ReadXMLAttributes(pm, element);

  this->XYChartViewBase4Axes = this->GetProperty("RightAxisRangeMinimum") != nullptr &&
    this->GetProperty("RightAxisRangeMaximum") != nullptr &&
    this->GetProperty("RightAxisUseCustomRange") != nullptr &&
    this->GetProperty("TopAxisRangeMinimum") != nullptr &&
    this->GetProperty("TopAxisRangeMaximum") != nullptr &&
    this->GetProperty("TopAxisUseCustomRange") != nullptr;

  return ret;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMContextViewProxy::PreRender(bool)
{
  if (auto pvview = vtkPVView::SafeDownCast(this->GetClientSideObject()))
  {
    return pvview->InTileDisplayMode() ? vtkPVSession::RENDER_SERVER | vtkPVSession::CLIENT
                                       : vtkPVSession::CLIENT;
  }
  return vtkPVSession::CLIENT;
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
