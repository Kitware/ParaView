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

#include "vtkContextView.h"
#include "vtkRenderWindow.h"
#include "vtkWindowToImageFilter.h"
#include "vtkProcessModule.h"
#include "vtkErrorCode.h"

#include "vtkObjectFactory.h"
#include "vtkSMUtilities.h"

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


//----------------------------------------------------------------------------
vtkSMContextViewProxy::vtkSMContextViewProxy()
{
  this->ChartView = NULL;
  this->Storage = NULL;
}

//----------------------------------------------------------------------------
vtkSMContextViewProxy::~vtkSMContextViewProxy()
{
  if (this->ChartView)
    {
    this->ChartView->Delete();
    this->ChartView = NULL;
    }
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

  this->Storage = new Private;
  this->ChartView = vtkContextView::New();
  this->NewChartView();

  this->Superclass::CreateVTKObjects();
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
  return NULL;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMContextViewProxy::CaptureWindow(int magnification)
{
  // Now update all representation pipelines - should not be necessary once
  // ParaView is ported to use vtkRenderViewBase, and handle the logic
  // centrally for all views derived from the base view.
  this->UpdateAllRepresentations();

  this->GetChartView()->Render();

  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetChartView()->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->Update();
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  w2i->Delete();

  // Update image extents based on ViewPosition
  int extents[6];
  capture->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] += this->ViewPosition[cc/2]*magnification;
    }
  capture->SetExtent(extents);

  return capture;
}

//-----------------------------------------------------------------------------
int vtkSMContextViewProxy::WriteImage(const char* filename,
  const char* writerName, int magnification)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));
  return vtkSMUtilities::SaveImageOnProcessZero(shot, filename, writerName);
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PerformRender()
{
  int size[2];
  this->GetGUISize(size);
  this->GetChartView()->Render();
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
