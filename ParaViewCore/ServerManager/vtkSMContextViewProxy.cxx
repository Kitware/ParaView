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

//-----------------------------------------------------------------------------
// Minimal storage class for STL containers etc.
class vtkSMContextViewProxy::Private
{
public:
  Private() { }
  ~Private()
  {
  }
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
  this->GetChart()->RecalculateBounds();
  this->GetChartView()->Render();
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
