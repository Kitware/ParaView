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

#include "vtkObjectFactory.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartView.h"
#include "vtkSMChartOptionsProxy.h"

#include "QVTKWidget.h"
#include <QPointer>

//-----------------------------------------------------------------------------
// Minimal storage class for STL containers etc.
class vtkSMContextViewProxy::Private
{
public:
  Private() { this->Widget = new QVTKWidget; }
  ~Private()
  {
    if (this->Widget)
      {
      delete this->Widget;
      this->Widget = NULL;
      }
  }

  QPointer<QVTKWidget> Widget;
};


vtkCxxRevisionMacro(vtkSMContextViewProxy, "1.6");
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
  this->ChartView->SetInteractor(this->Storage->Widget->GetInteractor());
  this->Storage->Widget->SetRenderWindow(this->ChartView->GetRenderWindow());

  this->NewChartView();

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
QVTKWidget* vtkSMContextViewProxy::GetChartWidget()
{
  return this->Storage->Widget;
}

//----------------------------------------------------------------------------
vtkContextView* vtkSMContextViewProxy::GetChartView()
{
  return this->ChartView;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMContextViewProxy::CaptureWindow(int magnification)
{
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

//----------------------------------------------------------------------------
bool vtkSMContextViewProxy::WriteImage(const char*)
{
//  this->PerformRender();
//  return this->ChartView->SaveImage(filename);
  return false;
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PerformRender()
{
  int size[2];
  this->GetGUISize(size);
}

//----------------------------------------------------------------------------
void vtkSMContextViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


