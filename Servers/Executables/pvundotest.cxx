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
#include "vtkProcessModule2.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUndoStack.h"

#include "paraview.h"

#include "vtkSMSessionClient.h"
#include <vtkstd/vector>
//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = false;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule2::PROCESS_BATCH, options);
  //---------------------------------------------------------------------------

  vtkSMSession* session = NULL;
  vtkSMProxy* proxy = NULL;
  if(options->GetUnknownArgument())
    {
    // We have a remote URL to use
    session = vtkSMSessionClient::New();
    vtkSMSessionClient::SafeDownCast(session)->Connect(options->GetUnknownArgument());
    }
  else
    {
    // We are in built-in mode
    session = vtkSMSession::New();
    }

  // Attach Undo/Redo stack
  vtkSMUndoStack *undoStack = vtkSMUndoStack::New();
  vtkSMUndoStackBuilder *undoStackBuilder = vtkSMUndoStackBuilder::New();
  undoStackBuilder->SetUndoStack(undoStack);
  session->SetUndoStackBuilder(undoStackBuilder);

  cout << "Starting..." << endl;

  vtkSMProxyManager* pxm = session->GetProxyManager();
  vtkstd::vector<vtkTypeUInt32> sphereIds;

  for(int i=0;i<10;i++)
    {
    // Start undo listening
    undoStackBuilder->Begin("sphere");

    cout << " Processing loop: " << i << endl;

    proxy = pxm->NewProxy("misc", "FileInformationHelper");
    vtkSMPropertyHelper(proxy, "Path").Set("/tmp");
    vtkSMPropertyHelper(proxy, "SpecialDirectories").Set(0);
    proxy->UpdateVTKObjects();

    vtkPVFileInformation* info = vtkPVFileInformation::New();
    proxy->GatherInformation(info);
    if(printObject) info->Print(cout);
    info->Delete();
    proxy->Delete();

    proxy = pxm->NewProxy("sources", "SphereSource");
    vtkSMPropertyHelper(proxy, "PhiResolution").Set(20);
    vtkSMPropertyHelper(proxy, "ThetaResolution").Set(20);
    proxy->UpdateVTKObjects();
    sphereIds.push_back(proxy->GetGlobalID());

    // Undo step
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    undoStackBuilder->Begin("shrink");


    vtkSMSourceProxy* shrink =
        vtkSMSourceProxy::SafeDownCast(
            pxm->NewProxy("filters", "ProcessIdScalars"));
    vtkSMPropertyHelper(shrink, "Input").Set(proxy);
    shrink->UpdateVTKObjects();
    shrink->UpdatePipeline();

    // Undo step
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    undoStackBuilder->Begin("writer");

    shrink->GetDataInformation(0);
    if(printObject) shrink->GetDataInformation(0)->Print(cout);

    vtkSMSourceProxy* writer =
        vtkSMSourceProxy::SafeDownCast(
            pxm->NewProxy("writers", "PDataSetWriter"));
    vtkSMPropertyHelper(writer, "Input").Set(shrink);
    vtkSMPropertyHelper(writer, "FileName").Set("/tmp/foo.vtk");
    writer->UpdateVTKObjects();
    writer->UpdatePipeline();

    // Undo step
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    undoStackBuilder->Begin("edit 1");

    vtkSMPropertyHelper(proxy, "PhiResolution").Set(10);
    vtkSMPropertyHelper(proxy, "ThetaResolution").Set(20);
    proxy->UpdateVTKObjects();

    // Undo step
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    undoStackBuilder->Begin("edit 2");

    vtkSMPropertyHelper(proxy, "PhiResolution").Set(30);
    vtkSMPropertyHelper(proxy, "ThetaResolution").Set(40);
    proxy->UpdateVTKObjects();

    // Undo step
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    undoStackBuilder->Begin("edit 3");

    vtkSMPropertyHelper(proxy, "PhiResolution").Set(50);
    vtkSMPropertyHelper(proxy, "ThetaResolution").Set(60);
    proxy->UpdateVTKObjects();

    // Undo step
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    undoStackBuilder->Begin("delete");

//    cout << "===========" << endl;
//    session->PrintSelf(cout, vtkIndent());
//    cout << "===========" << endl;

    writer->Delete();
    proxy->Delete();
    shrink->Delete();

    // Save undo listening
    undoStackBuilder->End();
    undoStackBuilder->PushToStack();
    }

  // Test stack content
  undoStackBuilder->SetIgnoreAllChanges(true);
  cout << "===== Undo previous work =====" << endl;
  for(int i=0, nb=undoStack->GetNumberOfUndoSets(); i < nb; i++)
    {
    cout << "Nb undo: " << undoStack->GetNumberOfUndoSets() << " - Can undo ? " << undoStack->CanUndo() << endl;
    cout << "Nb redo: " << undoStack->GetNumberOfRedoSets() << " - Can redo ? " << undoStack->CanRedo() << endl;
    cout << "==" << endl;
    cout << "undo " << i << ": " << undoStack->Undo() << endl;

    vtkstd::vector<vtkTypeUInt32>::iterator iter = sphereIds.begin();
    double phi,theta;
    while(iter != sphereIds.end())
      {
      vtkSMProxy *sphere = vtkSMProxy::SafeDownCast(session->GetRemoteObject(*iter));
      if(sphere)
        {
        vtkSMPropertyHelper(sphere, "PhiResolution").Get(&phi);
        vtkSMPropertyHelper(sphere, "ThetaResolution").Get(&theta);
        cout << "Sphere " << *iter << " PhiResolution: " << phi
             <<  " ThetaResolution: " << theta << endl;
        }
      iter++;
      }


    //session->PrintSelf(cout, vtkIndent());
    }
  cout << "Nb undo: " << undoStack->GetNumberOfUndoSets() << " - Can undo ? " << undoStack->CanUndo() << endl;
  cout << "Nb redo: " << undoStack->GetNumberOfRedoSets() << " - Can redo ? " << undoStack->CanRedo() << endl;



  cout << "Exiting..." << endl;
  undoStack->Delete();
  undoStackBuilder->Delete();
  session->Delete();


  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
