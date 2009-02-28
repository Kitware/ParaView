/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineChartViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLineChartViewProxy.h"

#include "vtkObjectFactory.h"

#include "vtkQtLineChartView.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartInteractorSetup.h"

vtkStandardNewMacro(vtkSMLineChartViewProxy);
vtkCxxRevisionMacro(vtkSMLineChartViewProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMLineChartViewProxy::vtkSMLineChartViewProxy()
{
  this->ChartView = vtkQtLineChartView::New();

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->ChartView->AddChartSelectionHandlers(selector);

  // Set default color scheme to blues
  this->ChartView->SetColorSchemeToBlues();
}

//----------------------------------------------------------------------------
vtkSMLineChartViewProxy::~vtkSMLineChartViewProxy()
{
  this->ChartView->Delete();
  this->ChartView = 0;
}

//----------------------------------------------------------------------------
vtkQtChartWidget* vtkSMLineChartViewProxy::GetChartWidget()
{
  return this->ChartView->GetChartWidget();
}

//----------------------------------------------------------------------------
vtkQtLineChartView* vtkSMLineChartViewProxy::GetLineChartView()
{
  return this->ChartView;
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::SetHelpFormat(const char* format)
{
  this->ChartView->SetHelpFormat(format);
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::PerformRender()
{
  this->ChartView->Update();
  this->ChartView->Render();
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


