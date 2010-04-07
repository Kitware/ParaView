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
#include "vtkSMApplication.h"
#include "vtkSMChartOptionsProxy.h"

#include <QWidget>

vtkCxxRevisionMacro(vtkSMChartViewProxy, "1.8.2.1");
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

  this->GetApplication()->EnsureQApplicationIsInitialized();

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
bool vtkSMChartViewProxy::WriteImage(const char* filename)
{
  this->PerformRender();
  return this->ChartView->SaveImage(filename);
}

//----------------------------------------------------------------------------
void vtkSMChartViewProxy::PerformRender()
{
  vtkSMChartOptionsProxy::SafeDownCast(
    this->GetSubProxy("ChartOptions"))->PrepareForRender(this);

  // For now use this to know if we are running in the gui or not
  bool isGui = this->ChartView->GetWidget()->parent() != NULL;

  // If not using the gui then we'll call Show() the first time we get here
  // to make the chart appear in a new window.  Then we have to manually
  // process events so that the window system can create the window.
  if (!isGui && !this->ChartView->GetWidget()->isVisible())
    {
    this->ChartView->GetWidget()->resize(800, 600);
    this->ChartView->Show();
    this->ChartView->ProcessQtEventsNoUserInput();
    }

  // This will update the representations and series options for the view
  this->ChartView->Update();
  this->ChartView->Render();

  // When we aren't using the gui we don't have an active Qt event loop
  // so we have to manually process events here.
  if (!isGui)
    {
    this->ChartView->ProcessQtEventsNoUserInput(); // once
    this->ChartView->ProcessQtEventsNoUserInput(); // twice for good luck ;-)
    }
}

//----------------------------------------------------------------------------
void vtkSMChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


