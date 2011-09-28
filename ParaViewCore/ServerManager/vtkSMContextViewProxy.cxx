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

#include "vtkChart.h"
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVContextView.h"
#include "vtkRenderWindow.h"
#include "vtkSMUtilities.h"
#include "vtkWindowToImageFilter.h"
#include "vtkAxis.h"
#include "vtkWeakPointer.h"
#include "vtkNew.h"
#include "vtkEventForwarderCommand.h"

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
    if(this->Proxy && this->Proxy->GetChart() && this->Forwarder.GetPointer() != NULL)
      {
      this->Proxy->GetChart()->RemoveObserver(this->Forwarder.GetPointer());
      }
    }

  void AttachCallback(vtkSMContextViewProxy* proxy)
    {
    this->Forwarder->SetTarget(proxy);
    this->Proxy = proxy;
    if(this->Proxy && this->Proxy->GetChart())
      {
      this->Proxy->GetChart()->AddObserver(
          vtkChart::UpdateRange, this->Forwarder.GetPointer());
      }
    }

  void UpdateBounds()
    {
    if(this->Proxy && this->Proxy->GetChart())
      {
      for(int i=0; i < 4; i++)
        {
        this->Proxy->GetChart()->GetAxis(i)->GetRange(&this->ViewBounds[i*2]);
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
vtkContextView* vtkSMContextViewProxy::GetChartView()
{
  return this->ChartView;
}

//----------------------------------------------------------------------------
vtkChart* vtkSMContextViewProxy::GetChart()
{
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(
    this->GetClientSideObject());
  return pvview? pvview->GetChart() : NULL;
}
//-----------------------------------------------------------------------------
void vtkSMContextViewProxy::ResetDisplay()
{
  int previousBehaviour[4];
  for(int i=0; i < 4; i++)
    {
    previousBehaviour[i] = this->GetChart()->GetAxis(i)->GetBehavior();
    this->GetChart()->GetAxis(i)->SetBehavior(vtkAxis::AUTO);
    }

  this->GetChart()->RecalculateBounds();
  this->GetChartView()->Render();

  // Revert behaviour as it use to be...
  for(int i=0; i < 4; i++)
    {
    this->GetChart()->GetAxis(i)->SetBehavior(previousBehaviour[i]);
    }
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMContextViewProxy::CaptureWindowInternal(int magnification)
{
  this->StillRender();

  this->GetChartView()->Render();

  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetChartView()->GetRenderWindow());
  w2i->SetMagnification(magnification);

  // Use front buffer on Windows for now until we can figure out
  // the bug with Charts when using the back buffer.
#ifdef WIN32
  w2i->Update();
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
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
  if(this->GetChart())
    {
    // Disable notification...
    this->Storage->Forwarder->SetTarget(NULL);

    for(int i=0; i < 4; i++)
      {
      this->Storage->ViewBounds[i*2] = bounds[i*2];
      this->Storage->ViewBounds[i*2+1] = bounds[i*2+1];

      this->GetChart()->GetAxis(i)->SetBehavior(vtkAxis::FIXED);
      this->GetChart()->GetAxis(i)->SetRange(bounds[i*2], bounds[i*2+1]);
      this->GetChart()->GetAxis(i)->RecalculateTickSpacing();
      }

//    cout << "New bounds: ["
//         << this->Storage->ViewBounds[0] << ", " << this->Storage->ViewBounds[1] << ", "
//         << this->Storage->ViewBounds[2] << ", " << this->Storage->ViewBounds[3] << ", "
//         << this->Storage->ViewBounds[4] << ", " << this->Storage->ViewBounds[5] << ", "
//         << this->Storage->ViewBounds[6] << ", " << this->Storage->ViewBounds[7] << "]"
//         << endl;

    // Do the rendering with the new range
    this->StillRender();
    this->GetChartView()->Render();

    // Bring the notification back
    this->Storage->Forwarder->SetTarget(this);
    }
}
