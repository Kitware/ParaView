/*=========================================================================

Program:   ParaView
Module:    pvserver.cxx

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

#include "vtkSMMessage.h"
#include "paraview.h"

#include "vtkSMSessionClient.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule::PROCESS_BATCH, options);
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

  cout << "Starting..." << endl;
  vtkSMProxyManager* pxm = session->GetProxyManager();
  vtkSMMessage origin, editA, editB, editC;
  // ==========================================================================

  proxy = pxm->NewProxy("sources", "SphereSource");
  pxm->RegisterProxy("testing","sphere", proxy);
  if(proxy->GetFullState())
    {
    return_value = EXIT_FAILURE;
    cout << "Error the initial state should be NULL before any UpdateVTKObjects" << endl;
    }
  proxy->UpdateVTKObjects();

  // Initial values
  if(!proxy->GetFullState())
    {
    return_value = EXIT_FAILURE;
    cout << "Error after UpdateVTKObjects, the State shouldn't be NULL" << endl;
    }
  origin = *proxy->GetFullState();
  cout << "====== Origin ======" << endl;
  origin.PrintDebugString();

  // Edition A
  vtkSMPropertyHelper(proxy, "PhiResolution").Set(20);
  vtkSMPropertyHelper(proxy, "ThetaResolution").Set(30);
  proxy->UpdateVTKObjects();
  editA = *proxy->GetFullState();
  cout << "====== Edition A ======" << endl;
  editA.PrintDebugString();

  // Edition B
  double center[3] = {1,2,3};
  vtkSMPropertyHelper(proxy, "PhiResolution").Set(40);
  vtkSMPropertyHelper(proxy, "ThetaResolution").Set(50);
  vtkSMPropertyHelper(proxy, "Center").Set(center, 3);
  proxy->UpdateVTKObjects();
  editB = *proxy->GetFullState();
  cout << "====== Edition B ======" << endl;
  editB.PrintDebugString();

  // Load previous state
  proxy->LoadState(&origin);
  proxy->UpdateVTKObjects();
  editC = *proxy->GetFullState();
  cout << "====== Edition C (Load origin) ======" << endl;
  if(editC.SerializeAsString() != origin.SerializeAsString())
    {
    return_value = EXIT_FAILURE;
    cout << "Error origin state and state after load origin differ !!!" << endl;
    editC.PrintDebugString();
    }
  else
    {
    cout << " - OK" << endl;
    }

  // Load previous state
  proxy->LoadState(&editA);
  proxy->UpdateVTKObjects();
  editC = *proxy->GetFullState();
  cout << "====== Edition C (Load editA) ======" << endl;
  if(editC.SerializeAsString() != editA.SerializeAsString())
    {
    return_value = EXIT_FAILURE;
    cout << "Error editA state and state after load origin differ !!!" << endl;
    editC.PrintDebugString();
    }
  else
    {
    cout << " - OK" << endl;
    }

  // Load previous state
  proxy->LoadState(&editB);
  proxy->UpdateVTKObjects();
  editC = *proxy->GetFullState();
  cout << "====== Edition C (Load editB) ======" << endl;
  if(editC.SerializeAsString() != editB.SerializeAsString())
    {
    return_value = EXIT_FAILURE;
    cout << "Error editB state and state after load origin differ !!!" << endl;
    editC.PrintDebugString();
    }
  else
    {
    cout << " - OK" << endl;
    }

  cout << "====== ProxyManagerState ======" << endl;
  pxm->GetFullState()->PrintDebugString();
  cout << "====== ProxyManagerState (cleared) ======" << endl;
  pxm->UnRegisterProxies();
  pxm->GetFullState()->PrintDebugString();

  // ==========================================================================
  proxy->Delete();
  cout << "Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
