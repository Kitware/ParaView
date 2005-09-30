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
#include "pqTesting.h"

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
#include <vtkSMStringVectorProperty.h>

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
  // Create a source ... see ParaView/Servers/ServerManager/Resources/sources.xml
  vtkSMProxy* const source = this->Server->GetProxyManager()->NewProxy("sources", "ExodusReader");
  this->Server->GetProxyManager()->RegisterProxy("paraq", "source1", source);
  source->Delete();
  vtkSMStringVectorProperty::SafeDownCast(source->GetProperty("FileName"))->SetElement(0, File.ascii());
  vtkSMStringVectorProperty::SafeDownCast(source->GetProperty("FilePrefix"))->SetElement(0, File.ascii());
  vtkSMStringVectorProperty::SafeDownCast(source->GetProperty("FilePattern"))->SetElement(0, "%s");
  source->UpdateVTKObjects();
  
  pqAddPart(Server, vtkSMSourceProxy::SafeDownCast(source));

  // Create a render window ...  
  vtkRenderWindow* const render_window = this->Server->GetRenderModule()->GetRenderWindow();
  render_window->SetWindowName("ParaQ Client");
  render_window->SetPosition(500, 500);
  render_window->SetSize(640, 480);
  render_window->Render();  

  pqResetCamera(Server->GetRenderModule());
  pqRedrawCamera(Server->GetRenderModule());
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

