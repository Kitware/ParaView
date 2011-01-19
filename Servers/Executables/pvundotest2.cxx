/*=========================================================================

Program:   ParaView
Module:    pvundotest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUndoStack.h"
#include "vtkSMRenderViewProxy.h"

#include "paraview.h"
#include "vtkCollection.h"

#include "vtkSMSessionClient.h"
#include <vtkstd/vector>
//----------------------------------------------------------------------------
void printSphereInfo(vtkSMProxy* sphere)
{
  double phi,theta;

  vtkSMPropertyHelper(sphere, "PhiResolution").Get(&phi);
  vtkSMPropertyHelper(sphere, "ThetaResolution").Get(&theta);

  cout << "Sphere " << " PhiResolution: " << phi
       <<  " ThetaResolution: " << theta << endl;
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = false;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_BATCH, options);
  //---------------------------------------------------------------------------

  vtkSMSession* session = vtkSMSession::New();
  session->Initialize();
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  // Attach Undo/Redo stack
  vtkSMUndoStack *undoStack = vtkSMUndoStack::New();
  undoStack->SetStackDepth(100);
  vtkSMUndoStackBuilder *undoStackBuilder = vtkSMUndoStackBuilder::New();
  undoStackBuilder->SetUndoStack(undoStack);
  session->SetUndoStackBuilder(undoStackBuilder);

  // --------------------------------------------------------------------------
  // Create proxy that are already there
  // --------------------------------------------------------------------------
  vtkSMProxy* view = pxm->NewProxy("views", "RenderView");
  view->UpdateVTKObjects();
  pxm->RegisterProxy("views", "RenderView1", view);
  view->Delete();

  // --------------------------------------------------------------------------
  undoStackBuilder->Begin("sphere");

  vtkSMProxy* sphere = pxm->NewProxy("sources", "SphereSource");
  sphere->UpdateVTKObjects();
  pxm->RegisterProxy("sources", "MySphere", sphere);
  sphere->Delete();

  undoStackBuilder->End();
  undoStackBuilder->PushToStack();

  // --------------------------------------------------------------------------

  undoStackBuilder->Begin("Apply simulation");

  vtkSMProxy* repr = pxm->NewProxy("representations", "GeometryRepresentation");
  vtkSMPropertyHelper(repr, "Input").Set(sphere);
  vtkSMPropertyHelper(repr, "Representation").Set("Wireframe");
  repr->UpdateVTKObjects();
  pxm->RegisterProxy("representations", "DataRepresentation1", repr);
  repr->Delete();


  vtkSMPropertyHelper(view, "Representations").Add(repr);
  view->UpdateVTKObjects();

  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();

  undoStackBuilder->End();
  undoStackBuilder->PushToStack();

  // --------------------------------------------------------------------------

//  undoStackBuilder->Begin("edit Resolution");

//  vtkSMPropertyHelper(sphere, "PhiResolution").Set(10);
//  vtkSMPropertyHelper(sphere, "ThetaResolution").Set(20);
//  sphere->UpdateVTKObjects();

//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
//  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();

//  undoStackBuilder->End();
//  undoStackBuilder->PushToStack();

  // --------------------------------------------------------------------------

//  undoStackBuilder->Begin("edit Resolution");

//  vtkSMPropertyHelper(sphere, "PhiResolution").Set(20);
//  vtkSMPropertyHelper(sphere, "ThetaResolution").Set(40);
//  sphere->UpdateVTKObjects();

//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
//  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();

//  undoStackBuilder->End();
//  undoStackBuilder->PushToStack();

  // --------------------------------------------------------------------------
  // Test stack content
  // --------------------------------------------------------------------------

  undoStackBuilder->SetIgnoreAllChanges(true);

  cout << "===== Undo previous work ( "<< undoStack->GetNumberOfUndoSets() <<" in stack) =====" << endl;

  cout << "Current state: " << endl;
  printSphereInfo(sphere);
  sleep(1);

//  cout << "Undo resolution change:" << endl;
//  undoStack->Undo();
//  printSphereInfo(sphere);
//  pxm->UpdateRegisteredProxies(1);
//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
//  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
//  sleep(1);

//  cout << "Undo resolution change:" << endl;
//  undoStack->Undo();
//  printSphereInfo(sphere);
//  pxm->UpdateRegisteredProxies(1);
//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
//  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
//  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
//  sleep(1);

  cout << "Undo representation add:" << endl;
  undoStack->Undo();
  printSphereInfo(sphere);
  pxm->UpdateRegisteredProxies(1);
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  sleep(1);

  cout << "Undo sphere:" << endl;
  undoStack->Undo();
  pxm->UpdateRegisteredProxies(1);
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  sleep(1);

  // --------------------------------------------------------------------------
  cout << "Exiting..." << endl;
  // --------------------------------------------------------------------------

  pxm->SetSession(NULL);
  undoStack->Delete();
  undoStackBuilder->Delete();
  session->Delete();


  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
