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
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMUtilities.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"
#include "vtkProcessModule.h"

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

static void update_property(vtkAxis* axis, vtkSMProperty* prop)
{
  if (axis && prop)
    {
    double range[2];
    axis->GetUnscaledRange(range);
    vtkSMPropertyHelper(prop).Set(range, 2);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::CopyAxisRangesFromChart()
{
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    update_property(
      chartXY->GetAxis(vtkAxis::LEFT), this->GetProperty("LeftAxisRange"));
    update_property(
      chartXY->GetAxis(vtkAxis::RIGHT), this->GetProperty("RightAxisRange"));
    update_property(
      chartXY->GetAxis(vtkAxis::TOP), this->GetProperty("TopAxisRange"));
    update_property(
      chartXY->GetAxis(vtkAxis::BOTTOM), this->GetProperty("BottomAxisRange"));
    this->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::OnInteractionEvent()
{
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    // On interaction, we ensure that the axis range properties reflect the
    // values that the user ended up picking due to the interactions. Thus, even
    // when user picks custom ranges for charts, he can still interact with the
    // view, we just update the custom range that was specified.
    this->CopyAxisRangesFromChart();
    this->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::InteractionEvent);
    }
}

//-----------------------------------------------------------------------------
void vtkSMContextViewProxy::ResetDisplay()
{
  // To simulate reset display, we turn-off using of custom ranges temporarily,
  // compute the bounds and then restore the state.
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    bool axis_behavior_fixed[4] = {false, false, false, false};
    for (int cc=0; cc < 4; cc++)
      {
      vtkAxis* axis = chartXY->GetAxis(cc);
      if (axis && axis->GetBehavior() == vtkAxis::FIXED)
        {
        axis_behavior_fixed[cc] = true;
        axis->SetBehavior(vtkAxis::AUTO);
        }
      }
    this->StillRender();
    chartXY->RecalculateBounds();
    this->CopyAxisRangesFromChart();
    this->UpdateVTKObjects();
    for (int cc=0; cc < 4; cc++)
      {
      vtkAxis* axis = chartXY->GetAxis(cc);
      if (axis && axis_behavior_fixed[cc])
        {
        axis->SetBehavior(vtkAxis::FIXED);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
