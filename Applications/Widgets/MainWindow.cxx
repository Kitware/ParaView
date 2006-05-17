/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "MainWindow.h"

#include <pqConnect.h>
#include <pqSetName.h>

#include <vtkCommand.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>

#include <QMenu>

#include "pqApplicationCore.h"
#include "pqRenderModule.h"

class MainWindow::pqObserver :
  public vtkCommand
{
public:
  static pqObserver *New()
  {
    return new pqObserver;
  }
  
  virtual void Execute(vtkObject* , unsigned long , void*)
  {
  }
};

//-----------------------------------------------------------------------------
MainWindow::MainWindow() : Observer(pqObserver::New())
{
  this->setWindowTitle("WidgetTester");

  this->createStandardFileMenu();
  this->createStandardViewMenu();
  this->createStandardServerMenu();
  this->createStandardSourcesMenu();
  this->createStandardFiltersMenu();
  this->createStandardToolsMenu();

  this->createStandardPipelineBrowser(true);
  this->createStandardObjectInspector(true);
  this->createStandardElementInspector(false);
  
  this->createStandardVCRToolBar();
  this->createStandardVariableToolBar();
  this->createStandardCompoundProxyToolBar();

  this->toolsMenu()->addAction(tr("Create Slider Widget"))
    << pqSetName("CreateSliderWidget")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onCreateSliderWidget()));

  this->toolsMenu()->addAction(tr("Create Implicit Plane Widget"))
    << pqSetName("CreateImplicitPlaneWidget")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onCreateImplicitPlaneWidget()));
}

//-----------------------------------------------------------------------------
MainWindow::~MainWindow()
{
  this->Observer->Delete();
  this->Observer = 0;
}

//-----------------------------------------------------------------------------
void MainWindow::onCreateSliderWidget()
{
  vtkSMNew3DWidgetProxy* const widget = vtkSMNew3DWidgetProxy::SafeDownCast(
    vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "SliderWidgetDisplay"));

/*
  vtkSMDoubleVectorProperty* p1 = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("Point1"));
  p1->SetElements3(-0.75, -0.5, 0);

  vtkSMDoubleVectorProperty* p2 = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("Point2"));
  p2->SetElements3( 0.75, -0.5, 0);

  vtkSMDoubleVectorProperty* min = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("MinimumValue"));
  min->SetElements1(6);

  vtkSMDoubleVectorProperty* max = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("MaximumValue"));
  max->SetElements1(128);
*/

  this->initializeWidget(widget);
}

//-----------------------------------------------------------------------------
void MainWindow::onCreateImplicitPlaneWidget()
{
  vtkSMNew3DWidgetProxy* const widget = vtkSMNew3DWidgetProxy::SafeDownCast(
    vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "ImplicitPlaneWidgetDisplay"));
      
  this->initializeWidget(widget);
}

void MainWindow::initializeWidget(vtkSMNew3DWidgetProxy* widget)
{
  widget->UpdateVTKObjects();

  widget->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);

  pqRenderModule* renModule = 
    pqApplicationCore::instance()->getActiveRenderModule();

  vtkSMRenderModuleProxy* rm = renModule->getProxy() ;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  if(pp)
    {
    pp->AddProxy(widget);
    rm->UpdateVTKObjects();
    }
}
