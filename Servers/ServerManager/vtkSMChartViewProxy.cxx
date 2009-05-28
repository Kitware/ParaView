/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartView.h"
#include "vtkSMChartOptionsProxy.h"

vtkCxxRevisionMacro(vtkSMChartViewProxy, "1.4");
//----------------------------------------------------------------------------
vtkSMChartViewProxy::vtkSMChartViewProxy()
{
  this->ChartView = 0;
}

//----------------------------------------------------------------------------
vtkSMChartViewProxy::~vtkSMChartViewProxy()
{
  if (this->ChartView)
    {
    this->ChartView->Delete();
    this->ChartView = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMChartViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->ChartView = this->NewChartView();

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->ChartView->AddChartSelectionHandlers(selector);

  // Set default color scheme to spectrum.
  this->ChartView->SetColorSchemeToSpectrum();

  vtkSMChartOptionsProxy::SafeDownCast(
    this->GetSubProxy("ChartOptions"))->SetChartView(this->ChartView);

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
vtkQtChartWidget* vtkSMChartViewProxy::GetChartWidget()
{
  return qobject_cast<vtkQtChartWidget*>(this->ChartView->GetWidget());
}

//----------------------------------------------------------------------------
vtkQtChartView* vtkSMChartViewProxy::GetChartView()
{
  return this->ChartView;
}

//----------------------------------------------------------------------------
void vtkSMChartViewProxy::PerformRender()
{
  vtkSMChartOptionsProxy::SafeDownCast(
    this->GetSubProxy("ChartOptions"))->PrepareForRender(this);
  this->ChartView->Update();
  this->ChartView->Render();
}

//----------------------------------------------------------------------------
void vtkSMChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


