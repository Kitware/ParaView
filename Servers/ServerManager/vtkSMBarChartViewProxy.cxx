/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBarChartViewProxy.h"

#include "vtkObjectFactory.h"

#include "vtkQtBarChartView.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkSMChartOptionsProxy.h"

vtkStandardNewMacro(vtkSMBarChartViewProxy);
vtkCxxRevisionMacro(vtkSMBarChartViewProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMBarChartViewProxy::vtkSMBarChartViewProxy()
{
  this->ChartView = vtkQtBarChartView::New();

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->ChartView->AddChartSelectionHandlers(selector);

  // Set default color scheme to blues
  this->ChartView->SetColorSchemeToBlues();
}

//----------------------------------------------------------------------------
vtkSMBarChartViewProxy::~vtkSMBarChartViewProxy()
{
  this->ChartView->Delete();
  this->ChartView = 0;
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkSMChartOptionsProxy::SafeDownCast(
    this->GetSubProxy("ChartOptions"))->SetChartView(
    this->GetBarChartView());
}

//----------------------------------------------------------------------------
vtkQtChartWidget* vtkSMBarChartViewProxy::GetChartWidget()
{
  return this->ChartView->GetChartWidget();
}

//----------------------------------------------------------------------------
vtkQtBarChartView* vtkSMBarChartViewProxy::GetBarChartView()
{
  return this->ChartView;
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetHelpFormat(const char* format)
{
  this->ChartView->SetHelpFormat(format);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetOutlineStyle(int outline)
{
  this->ChartView->SetOutlineStyle(outline);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetBarGroupFraction(float fraction)
{
  this->ChartView->SetBarGroupFraction(fraction);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetBarWidthFraction(float fraction)
{
  this->ChartView->SetBarWidthFraction(fraction);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::PerformRender()
{
  this->ChartView->Update();
  this->ChartView->Render();
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


