/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCamera.h"
#include "pqMainWindow.h"
#include "pqParts.h"
#include "pqServer.h"
#include "pqServerFileBrowser.h"
#include "pqTests.h"

#include <QApplication>
#include <QMenu>
#include <QMenuBar>

#include <vtkRenderWindow.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

namespace
{

void pqDumpHierarchy(ostream& Stream, QObject& Object, unsigned long Indent = 0)
{
  Stream << vtkstd::string(Indent, '\t') << Object.name("[unspecified]") << endl;
  
  QList<QObject*> children = Object.findChildren<QObject*>();
  for(QList<QObject*>::iterator child = children.begin(); child != children.end(); ++child)
    pqDumpHierarchy(Stream, **child, Indent+1);
}

struct pqSetName
{
  pqSetName(const vtkstd::string& Name) : name(Name) {}
  const vtkstd::string name;
};

template<typename T>
T* operator<<(T* LHS, const pqSetName& RHS)
{
  LHS->setName(RHS.name.c_str());
  return LHS;
}

} // namespace

pqMainWindow::pqMainWindow(QApplication& Application) :
  Server(new pqServer())
{
  this->setName("mainWindow");
  this->setWindowTitle("ParaQ Client");

  QAction* const fileOpenAction = new QAction(tr("Open..."), this) << pqSetName("fileOpenAction");
  connect(fileOpenAction, SIGNAL(activated()), this, SLOT(onFileOpen()));

  QAction* const fileQuitAction = new QAction(tr("Quit"), this) << pqSetName("fileQuitAction");
  connect(fileQuitAction, SIGNAL(activated()), &Application, SLOT(quit()));

  QAction* const debugHierarchyAction = new QAction(tr("Dump Hierarchy"), this) << pqSetName("debugHierarchyAction");
  connect(debugHierarchyAction, SIGNAL(activated()), this, SLOT(onDebugHierarchy()));

  QAction* const testsRunAction = new QAction(tr("Run"), this) << pqSetName("testsRunAction");
  connect(testsRunAction, SIGNAL(activated()), this, SLOT(onTestsRun()));

  this->menuBar() << pqSetName("menuBar");

  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File")) << pqSetName("fileMenu");
  fileMenu->addAction(fileOpenAction);
  fileMenu->addAction(fileQuitAction);
  
  QMenu* const debugMenu = this->menuBar()->addMenu(tr("Debug")) << pqSetName("debugMenu");
  debugMenu->addAction(debugHierarchyAction);
  
  QMenu* const testMenu = this->menuBar()->addMenu(tr("Tests")) << pqSetName("testMenu");
  testMenu->addAction(testsRunAction);
}

void pqMainWindow::onFileOpen()
{
  pqServerFileBrowser* const file_browser = new pqServerFileBrowser(*this->Server, this, "fileOpenBrowser");
  file_browser->show();
  QObject::connect(file_browser, SIGNAL(fileSelected(const QString&)), this, SLOT(onFileOpen(const QString&)));
}

void pqMainWindow::onFileOpen(const QString& File)
{
  vtkSMProxyManager* proxyM = this->Server->GetProxyManager();
  
  // Here we create a simple pipeline

  // vector text source (see
  // ParaView/Servers/ServerManager/Resources/sources.xml and filters.xml)
  vtkSMProxy *source = proxyM->NewProxy("sources", "CylinderSource");
  proxyM->RegisterProxy("my proxies", "source1", source);
  source->Delete();
  // Set the value of the Resolution property.  It just so happens that
  // both ConeSource and CylinderSource have a Resolution property.
  vtkSMIntVectorProperty::SafeDownCast(source->GetProperty("Resolution"))->SetElement(0, 64);
  source->UpdateVTKObjects();

  // apply a normals filter
  vtkSMProxy *normals = proxyM->NewProxy("filters", "PolyDataNormals");
  proxyM->RegisterProxy("my proxies", "normals1", normals);
  normals->Delete();
  // connect the filter
  vtkSMProxyProperty *input
    = vtkSMProxyProperty::SafeDownCast(normals->GetProperty("Input"));
  input->AddProxy(source);
  normals->UpdateVTKObjects();

  // Get a render module.  This sets up the parallel rendering process.
  // If you want to specify a render module, call SetRenderModule in
  // options to the name of the render module class minus the vtkPV prefix
  // before calling pqApplication::Initialize.
  vtkSMRenderModuleProxy *rm = this->Server->GetRenderModule();

  // Turn on compositing for all data for demonstration.  This is controlled
  // by a CompositeThreshold property on compositing render modules.  If the
  // property is not there, it must not be a compositing render module.
  vtkSMDoubleVectorProperty *ctprop
    = vtkSMDoubleVectorProperty
      ::SafeDownCast(rm->GetProperty("CompositeThreshold"));
  if (ctprop)
    {
    ctprop->SetElement(0, 0.0);
    }
  // Also bump up the reduction factor on compositing modules.
  vtkSMIntVectorProperty *rfprop
    = vtkSMIntVectorProperty::SafeDownCast(rm->GetProperty("ReductionFactor"));
  if (rfprop)
    {
    rfprop->SetElement(0, 4);
    }

  // Add the end of our pipeline as a part in the display.
  pqAddPart(Server, vtkSMSourceProxy::SafeDownCast(normals));

  // Create a render window ...  
  vtkRenderWindow* const render_window = this->Server->GetRenderModule()->GetRenderWindow();
  render_window->SetWindowName("ParaQ");
  render_window->SetPosition(500, 500);
  render_window->Render();  

  pqResetCamera(*Server);
}

void pqMainWindow::onDebugHierarchy()
{
  pqDumpHierarchy(cerr, *this);
}

void pqMainWindow::onTestsRun()
{
  pqRunRegressionTests();
  pqRunRegressionTests(*this);
}

