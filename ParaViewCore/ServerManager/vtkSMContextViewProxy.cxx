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
#include "vtkCommand.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyHelper.h"
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
  this->StillRender();

  this->GetContextView()->Render();

  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetContextView()->GetRenderWindow());
  w2i->SetMagnification(magnification);

  // Use front buffer on Windows for now until we can figure out
  // the bug with Charts when using the back buffer.
#ifdef WIN32
  w2i->Update();
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
#elif defined(__APPLE__)
  w2i->ReadFrontBufferOn();
  w2i->ShouldRerenderOn();
  w2i->Update();
#else
  // Everywhere else use back buffer.
  w2i->ReadFrontBufferOff();
  // ShouldRerender was turned off previously. Why? Since we told w2i to read
  // backbuffer, shouldn't we re-render again?
  w2i->ShouldRerenderOn();
  w2i->Update();
#endif

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  w2i->Delete();
  return capture;
}

static void update_property(vtkAxis* axis, vtkSMProperty* prop)
{
  if (axis && prop)
    {
    double range[2];
    axis->GetRange(range);
    vtkSMPropertyHelper(prop).SetNumberOfElements(2);
    vtkSMPropertyHelper(prop).Set(range, 2);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::OnInteractionEvent()
{
  vtkChartXY *chartXY = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chartXY)
    {
    // FIXME: Generalize to support charts with zero to many axes.
    update_property(
      chartXY->GetAxis(vtkAxis::LEFT), this->GetProperty("LeftAxisRange"));
    update_property(
      chartXY->GetAxis(vtkAxis::RIGHT), this->GetProperty("RightAxisRange"));
    update_property(
      chartXY->GetAxis(vtkAxis::TOP), this->GetProperty("TopAxisRange"));
    update_property(
      chartXY->GetAxis(vtkAxis::BOTTOM), this->GetProperty("BottomAxisRange"));
    this->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::InteractionEvent);
    }
}

//-----------------------------------------------------------------------------
void vtkSMContextViewProxy::ResetDisplay()
{
  vtkSMPropertyHelper(this, "LeftAxisRange", true).SetNumberOfElements(0);
  vtkSMPropertyHelper(this, "RightAxisRange", true).SetNumberOfElements(0);
  vtkSMPropertyHelper(this, "TopAxisRange", true).SetNumberOfElements(0);
  vtkSMPropertyHelper(this, "BottomAxisRange", true).SetNumberOfElements(0);
  this->UpdateVTKObjects();
  this->StillRender();
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
