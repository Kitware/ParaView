/*=========================================================================

Program:   ParaView
Module:    TestCatalystServerManager.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Initialization
  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_BATCH, options);
  vtkSMSession* session = vtkSMSession::New();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  //---------------------------------------------------------------------------

  if (!pxm)
  {
    cout << "Null proxy manager" << endl;
    session->Delete();
    return EXIT_FAILURE;
  }

  // Create proxy and change main radius value
  vtkSMProxy* proxy = pxm->NewProxy("testcase", "None");

  if (!proxy)
  {
    cout << "Null proxy" << endl;
    session->Delete();
    return EXIT_FAILURE;
  }
  proxy->UpdateVTKObjects();

  // *******************************************************************

  proxy->Delete();
  session->Delete();
  cout << "Exiting..." << endl;

  vtkInitializationHelper::Finalize();
  options->Delete();
  return EXIT_SUCCESS;
}
