/*=========================================================================

Program:   ParaView
Module:    pvtest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "paraview.h"
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionCore.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = false;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule::PROCESS_CLIENT, options);
  //---------------------------------------------------------------------------

  vtkSMSession* session = NULL;
  vtkSMProxy* proxy = NULL;

  session = vtkSMSessionClient::New();
  session->Initialize();
  vtkSMSessionClient::SafeDownCast(session)->Connect("cs://localhost:11111");

  cout << "Starting..." << endl;

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);

  //================= Get ProxyDefinition from server =====================
  pxm->LoadXMLDefinitionFromServer();
  //-----------------------------------------------------------------------

  cout << "Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
