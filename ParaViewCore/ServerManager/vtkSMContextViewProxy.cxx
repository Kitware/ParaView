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
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkEventForwarderCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVContextView.h"
#include "vtkRenderWindow.h"
#include "vtkSMUtilities.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

//-----------------------------------------------------------------------------
// Minimal storage class for STL containers etc.
class vtkSMContextViewProxy::Private
{
public:
  Private()
    {
    ViewBounds[0] = ViewBounds[2] = ViewBounds[4] = ViewBounds[6] = 0.0;
    ViewBounds[1] = ViewBounds[3] = ViewBounds[5] = ViewBounds[7] = 1.0;
    }

  ~Private()
    {
    if (this->Proxy && this->Proxy->GetContextItem() &&
        this->Forwarder.GetPointer() != NULL)
      {
      this->Proxy->GetContextItem()->RemoveObserver(this->Forwarder.GetPointer());
      }
    }

  void AttachCallback(vtkSMContextViewProxy* proxy)
    {
    this->Forwarder->SetTarget(proxy);
    this->Proxy = proxy;
    if(this->Proxy && this->Proxy->GetContextItem())
      {
      this->Proxy->GetContextItem()->AddObserver(
          vtkChart::UpdateRange, this->Forwarder.GetPointer());
      }
    }

  void UpdateBounds()
    {
    if(this->Proxy && this->Proxy->GetContextItem())
      {
      for(int i=0; i < 4; i++)
        {
        // FIXME: Generalize to support charts with zero to many axes.
        vtkChartXY *chart = vtkChartXY::SafeDownCast(this->Proxy->GetContextItem());
        if (chart)
          {
          chart->GetAxis(i)->GetRange(&this->ViewBounds[i*2]);
          }
        }
      }
    }

public:
  double ViewBounds[8];
  vtkNew<vtkEventForwarderCommand> Forwarder;

private:
  vtkWeakPointer<vtkSMContextViewProxy> Proxy;
};

vtkStandardNewMacro(vtkSMContextViewProxy);
//----------------------------------------------------------------------------
vtkSMContextViewProxy::vtkSMContextViewProxy()
{
  this->ChartView = NULL;
  this->Storage = NULL;
}

//----------------------------------------------------------------------------
vtkSMContextViewProxy::~vtkSMContextViewProxy()
{
  delete this->Storage;
  this->Storage = NULL;
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

  this->Storage = new Private;
  this->ChartView = pvview->GetContextView();

  // Try to attach viewport listener on chart
  this->Storage->AttachCallback(this);
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
void vtkSMContextViewProxy::ResetDisplay()
{
  // FIXME: We should generalize this to support all charts (zero to many axes).
  vtkChartXY *chart = vtkChartXY::SafeDownCast(this->GetContextItem());
  if (chart)
    {
    int previousBehaviour[4];
    for (int i = 0; i < 4; ++i)
      {
      previousBehaviour[i] = chart->GetAxis(i)->GetBehavior();
      chart->GetAxis(i)->SetBehavior(vtkAxis::AUTO);
      }

    chart->RecalculateBounds();
    this->GetContextView()->Render();

    // Revert behaviour as it use to be...
    for (int i = 0; i < 4; ++i)
      {
      chart->GetAxis(i)->SetBehavior(previousBehaviour[i]);
      }
    }
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMContextViewProxy::CaptureWindowInternal(int magnification)
{
  vtkRenderWindow* window = this->GetRenderWindow();

  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.
#if !defined(__APPLE__)
  int prevOffscreen = window->GetOffScreenRendering();
  window->SetOffScreenRendering(1);
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

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
double* vtkSMContextViewProxy::GetViewBounds()
{
  this->Storage->UpdateBounds();
  return this->Storage->ViewBounds;
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::SetViewBounds(double* bounds)
{
  if(this->GetContextItem())
    {
    // Disable notification...
    this->Storage->Forwarder->SetTarget(NULL);
    // FIXME: This also needs generalizing to support all chart types.
    vtkChartXY *chart = vtkChartXY::SafeDownCast(this->GetContextItem());

    if (chart)
      {
      for (int i = 0; i < 4; i++)
        {
        this->Storage->ViewBounds[i*2] = bounds[i*2];
        this->Storage->ViewBounds[i*2+1] = bounds[i*2+1];

        chart->GetAxis(i)->SetBehavior(vtkAxis::FIXED);
        chart->GetAxis(i)->SetRange(bounds[i*2], bounds[i*2+1]);
        chart->GetAxis(i)->RecalculateTickSpacing();
        }
      }

    // Do the rendering with the new range
    this->StillRender();
    this->GetContextView()->Render();

    // Bring the notification back
    this->Storage->Forwarder->SetTarget(this);
    }
}
