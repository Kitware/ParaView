/*=========================================================================

Program:   ParaView
Module:    pvlocationtest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionClient.h"

#include "paraview.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize( argc, argv,
                                       vtkProcessModule::PROCESS_BATCH,
                                       options );

  //---------------------------------------------------------------------------
  int return_value = EXIT_SUCCESS;
  //---------------------------------------------------------------------------
  vtkSMSession* session = NULL;
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

  cout << "==== Starting ====" << endl;
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  // location 1
  vtkSMProxy* client = pxm->NewProxy("utils", "clientPID");
  vtkSMProxy* dataServer = pxm->NewProxy("utils", "dataserverPID");
  vtkSMProxy* renderServer = pxm->NewProxy("utils", "renderserverPID");

  client->UpdateVTKObjects();
  client->UpdatePropertyInformation();

  dataServer->UpdateVTKObjects();
  dataServer->UpdatePropertyInformation();

  renderServer->UpdateVTKObjects();
  renderServer->UpdatePropertyInformation();

  int clientPid, dsPid, rsPid;
  vtkSMPropertyHelper(client, "ProcessId").Get(&clientPid);
  vtkSMPropertyHelper(dataServer, "ProcessId").Get(&dsPid);
  vtkSMPropertyHelper(renderServer, "ProcessId").Get(&rsPid);

  cout << "Client PID: " << clientPid << endl;
  cout << "DataServer PID: " << dsPid << endl;
  cout << "RenderServer PID: " << rsPid << endl;

  client->Delete();
  dataServer->Delete();
  renderServer->Delete();

  // Remarque: Dynamic reloaction is not allowed, it has to be specified in
  //           the proxy XML definition.

//  cout << "==== Dynamic relocation part ====" << endl;

//  // Test with dynamic relocation
//  client = pxm->NewProxy("utils", "pid");
//  dataServer = pxm->NewProxy("utils", "pid");
//  renderServer = pxm->NewProxy("utils", "pid");

//  client->SetLocation(vtkProcessModule::PROCESS_BATCH);
//  dataServer->SetLocation(vtkProcessModule::PROCESS_DATA_SERVER);
//  renderServer->SetLocation(vtkProcessModule::PROCESS_RENDER_SERVER);

//  client->UpdateVTKObjects();
//  client->UpdatePropertyInformation();

//  dataServer->UpdateVTKObjects();
//  dataServer->UpdatePropertyInformation();

//  renderServer->UpdateVTKObjects();
//  renderServer->UpdatePropertyInformation();

//  vtkSMPropertyHelper(client, "ProcessId").Get(&clientPid);
//  vtkSMPropertyHelper(dataServer, "ProcessId").Get(&dsPid);
//  vtkSMPropertyHelper(renderServer, "ProcessId").Get(&rsPid);

//  cout << "Client PID: " << clientPid << endl;
//  cout << "DataServer PID: " << dsPid << endl;
//  cout << "RenderServer PID: " << rsPid << endl;

//  client->Delete();
//  dataServer->Delete();
//  renderServer->Delete();

  cout << "==== Exiting ====" << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
