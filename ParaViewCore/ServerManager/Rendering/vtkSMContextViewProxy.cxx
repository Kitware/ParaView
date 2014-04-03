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
#include "vtkChartXY.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkEventForwarderCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUtilities.h"
#include "vtkStructuredData.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

//****************************************************************************
// vtkSMContextViewInteractorStyle makes it possible for us to call
// StillRender() as the user interacts with the chart views on the client side
// instead of directly calling Render() on the render-window. This makes no
// difference in general, except in tile-display mode, where the StillRender()
// ensures that the server-side views are updated as well.
class vtkSMContextViewInteractorStyle : public vtkContextInteractorStyle
{
public:
  static vtkSMContextViewInteractorStyle*New();
  vtkTypeMacro(vtkSMContextViewInteractorStyle, vtkContextInteractorStyle);

  void SetView(vtkSMContextViewProxy* view)
    { this->ViewProxy = view; }
protected:
  virtual void RenderNow()
    {
    if (this->ViewProxy)
      {
      this->ViewProxy->StillRender();
      }
    }

private:
  vtkSMContextViewInteractorStyle() {}
  ~vtkSMContextViewInteractorStyle() {}
  vtkWeakPointer<vtkSMContextViewProxy> ViewProxy;
};

vtkStandardNewMacro(vtkSMContextViewInteractorStyle);
//****************************************************************************

namespace {
const char* XY_CHART_VIEW = "XYChartView";
}

vtkStandardNewMacro(vtkSMContextViewProxy);
//----------------------------------------------------------------------------
vtkSMContextViewProxy::vtkSMContextViewProxy()
{
  this->ChartView = NULL;
}

//----------------------------------------------------------------------------
vtkSMContextViewProxy::~vtkSMContextViewProxy()
{
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
  if(this->Location == 0)
    {
    return;
    }

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(
    this->GetClientSideObject());
  this->ChartView = pvview->GetContextView();

  // if user interacts with the chart, we need to ensure that we set the axis
  // behaviors to "FIXED" so that the user chosen axis ranges are preserved in
  // state files, etc.
  this->GetContextItem()->AddObserver(
    vtkCommand::InteractionEvent, this,
    &vtkSMContextViewProxy::OnInteractionEvent);

  // update the interactor style.
  vtkSMContextViewInteractorStyle* style =
    vtkSMContextViewInteractorStyle::New();
  style->SetScene(this->ChartView->GetScene());
  style->SetView(this);
  this->ChartView->GetInteractor()->SetInteractorStyle(style);
  style->Delete();
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
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(
    this->GetClientSideObject());
  return pvview? pvview->GetContextItem() : NULL;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMContextViewProxy::CaptureWindowInternal(int magnification)
{
  vtkRenderWindow* window = this->GetRenderWindow();

  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.
#if !defined(__APPLE__)
  int prevOffscreen = window->GetOffScreenRendering();

  vtkPVContextView* view = vtkPVContextView::SafeDownCast(
    this->GetClientSideObject());
  bool use_offscreen = view->GetUseOffscreenRendering() ||
    view->GetUseOffscreenRenderingForScreenshots();
  window->SetOffScreenRendering(use_offscreen? 1: 0);
#endif

  window->SwapBuffersOff();

  this->StillRender();
  this->GetContextView()->Render();

  vtkSmartPointer<vtkWindowToImageFilter> w2i =
    vtkSmartPointer<vtkWindowToImageFilter>::New();
  w2i->SetInput(window);
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOff();

  // BUG #8715: We go through this indirection since the active connection needs
  // to be set during update since it may request re-renders if magnification >1.
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << w2i.GetPointer() << "Update"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkProcessModule::CLIENT);

  window->SwapBuffersOn();
#if !defined(__APPLE__)
  window->SetOffScreenRendering(prevOffscreen);
#endif

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  window->Frame();
  return capture;
}

static void update_property(vtkAxis* axis, vtkSMProperty* propMin,
                            vtkSMProperty* propMax)
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
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    update_property(
      chartXY->GetAxis(vtkAxis::LEFT),
      this->GetProperty("LeftAxisRangeMinimum"),
      this->GetProperty("LeftAxisRangeMaximum"));
    update_property(
      chartXY->GetAxis(vtkAxis::BOTTOM),
      this->GetProperty("BottomAxisRangeMinimum"),
      this->GetProperty("BottomAxisRangeMaximum"));
    if (this->GetXMLName() == XY_CHART_VIEW)
      {
      update_property(
        chartXY->GetAxis(vtkAxis::RIGHT),
        this->GetProperty("RightAxisRangeMinimum"),
        this->GetProperty("RightAxisRangeMaximum"));
      update_property(
        chartXY->GetAxis(vtkAxis::TOP),
        this->GetProperty("TopAxisRangeMinimum"),
        this->GetProperty("TopAxisRangeMaximum"));
      }
    this->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::OnInteractionEvent()
{
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    // Charts by default update axes ranges as needed. On interaction, we force
    // the chart to preserve the user-selected ranges.
    this->CopyAxisRangesFromChart();
    vtkSMPropertyHelper(this, "LeftAxisUseCustomRange").Set(1);
    vtkSMPropertyHelper(this, "BottomAxisUseCustomRange").Set(1);
    if (this->GetXMLName() == XY_CHART_VIEW)
      {
      vtkSMPropertyHelper(this, "RightAxisUseCustomRange").Set(1);
      vtkSMPropertyHelper(this, "TopAxisUseCustomRange").Set(1);
      }
    this->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::InteractionEvent);
    }
}

//-----------------------------------------------------------------------------
void vtkSMContextViewProxy::ResetDisplay()
{
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    // simply unlock all the axes ranges. That results in the chart determine
    // new ranges to use in the Update call.
    vtkSMPropertyHelper(this, "LeftAxisUseCustomRange").Set(0);
    vtkSMPropertyHelper(this, "BottomAxisUseCustomRange").Set(0);
    if (this->GetXMLName() == XY_CHART_VIEW)
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

  if (producer->GetHints() &&
    producer->GetHints()->FindNestedElementByName("Plotable"))
    {
    return true;
    }

  vtkPVDataInformation* dataInfo = producer->GetDataInformation(outputPort);
  if (dataInfo->DataSetTypeIsA("vtkTable"))
    {
    return true;
    }
  // also accept 1D structured datasets.
  if (dataInfo->DataSetTypeIsA("vtkImageData") ||
    dataInfo->DataSetTypeIsA("vtkRectilinearGrid"))
    {
    int extent[6];
    dataInfo->GetExtent(extent);
    int temp[6]={0, 0, 0, 0, 0, 0};
    int dimensionality = vtkStructuredData::GetDataDimension(
      vtkStructuredData::SetExtent(extent, temp));
    if (dimensionality == 1)
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
