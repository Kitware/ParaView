// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqApplication.h"
#include "pqMainWindow.h"
#include "pqOptions.h"
#include "pqProcessModuleGUIHelper.h"

#include <vtkObjectFactory.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>
#include <vtkProcessModule.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkCamera.h>

#include <QApplication>
#include <QPushButton>

vtkCxxRevisionMacro(pqProcessModuleGUIHelper, "1.1");
vtkStandardNewMacro(pqProcessModuleGUIHelper);

//-----------------------------------------------------------------------------

pqProcessModuleGUIHelper::pqProcessModuleGUIHelper()
{
}

pqProcessModuleGUIHelper::~pqProcessModuleGUIHelper()
{
}

void pqProcessModuleGUIHelper::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

int pqProcessModuleGUIHelper::OpenConnectionDialog(int *vtkNotUsed(start))
{
  // This is where you could give the user a dialog box to change the
  // options to connect to the server.  The options would be then placed in
  // the options object retrieved from this->ProcessModule->GetOptions().
  // If the user changes options, return 1 and set start to 1 (I guess).
  // Otherwise, return 0.  See vtkPVProcessModuleGUIHelper for a better
  // example.
  cout << "Wanna change how you connect to the server?  Too bad." << endl;

  return 0;
}

//-----------------------------------------------------------------------------

void pqProcessModuleGUIHelper::SendPrepareProgress()
{
}

void pqProcessModuleGUIHelper::SetLocalProgress(const char *filter,
                                                   int progress)
{
}

void pqProcessModuleGUIHelper::SendCleanupPendingProgress()
{
}

//-----------------------------------------------------------------------------

void pqProcessModuleGUIHelper::ExitApplication()
{
}

//-----------------------------------------------------------------------------

int pqProcessModuleGUIHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
/*
  // At this point, the process module and the client/server interpreter are initialized.

  pqOptions *options
    = pqOptions::SafeDownCast(this->ProcessModule->GetOptions());

  // Create a helper class and initialize it.
  pqApplication *application = pqApplication::New();
  application->Initialize(this->ProcessModule);
  application->SetupRenderModule();

  vtkSMProxyManager* proxyM = application->GetProxyManager();

  // Here we create a simple pipeline

  // vector text source (see
  // ParaView/Servers/ServerManager/Resources/sources.xml and filters.xml)
  vtkSMProxy *source = proxyM->NewProxy("sources", "ConeSource");
  proxyM->RegisterProxy("my proxies", "source1", source);
  source->Delete();
  // Set the value of the Resolution property.  It just so happens that
  // both ConeSource and CylinderSource have a Resolution property.
  vtkSMIntVectorProperty::SafeDownCast(source->GetProperty("Resolution"))
    ->SetElement(0, 64);
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
  vtkSMRenderModuleProxy *rm = application->GetRenderModuleProxy();

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

  // Here is how you access the local (client) render window and renderer.
  // Do not add renderers to the render window or actors/mappers to the
  // renderer.  Remember that there may also be coupled render
  // windows/renderers on a server cluster somewhere.
  vtkRenderWindow* renwin = rm->GetRenderWindow();
  renwin->SetWindowName("ParaQ");
  renwin->SetPosition(500, 500);
  
  vtkRenderer* renderer = rm->GetRenderer();

  // Add the end of our pipeline as a part in the display.
  application->AddPart(vtkSMSourceProxy::SafeDownCast(normals));

//   // This render is necessary to "boot" the compositing. In other words, it
//   // is to bypass a bug in the compositing code.
//  application->StillRender();

  // This will make sure the camera is reset to the actual data in
  // client/server mode.
  application->ResetCamera();

  vtkRenderWindowInteractor* const interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renwin);
  interactor->Initialize();
//  interactor->Start();
*/
  QApplication qapplication(argc, argv);
  pqMainWindow qwindow(qapplication);
  qwindow.resize(400, 400);
  qwindow.show();
  const int result = qapplication.exec();

/*  
  interactor->Delete();

  // Cleanup ParaView.
  application->Finalize();
  application->Delete();
*/

  // This value is returned by vtkProcessModule::Start.
  return result;
}
