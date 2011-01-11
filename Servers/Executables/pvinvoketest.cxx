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

  //session = vtkSMSessionClient::New();
  //vtkSMSessionClient::SafeDownCast(session)->Connect("cs://localhost:11111");
  session = vtkSMSession::New();

  cout << "Starting..." << endl;

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);

  //================= Invoke test =======================

  cout << "+++++++++++++++++++" << endl;
  int result = -1;

  proxy = pxm->NewProxy("sources","XMLStructuredGridReader");
  proxy->UpdateVTKObjects();

  vtkSMMessage msg;
  msg.set_global_id(proxy->GetGlobalID());
  msg.set_location(proxy->GetLocation());
  msg << pvstream::InvokeRequestNoWarning() << "CanReadFile" << "/tmp";
  session->Invoke(&msg);

  if(msg.GetExtension(InvokeResponse::error))
    {
    cout << "Error: no response" << endl;
    cout << "ERROR Exiting..." << endl;
    return EXIT_FAILURE;
    }
  else
    {
    result = msg.GetExtension(InvokeResponse::arguments).variant(0).integer(0);
    cout << "Reply: " << result << endl;
    if(result != 1)
      {
      cout << "ERROR Exiting..." << endl;
      return EXIT_FAILURE;
      }
    }
  proxy->Delete();

  cout << "+++++++++++++++++++" << endl;

  proxy = pxm->NewProxy("sources","XMLStructuredGridReader");
  proxy->UpdateVTKObjects();

  msg.Clear();
  msg.set_global_id(proxy->GetGlobalID());
  msg.set_location(proxy->GetLocation());
  msg << pvstream::InvokeRequestNoWarning() << "WRONG-Method-Name" << "/tmp";
  session->Invoke(&msg);

  if(msg.GetExtension(InvokeResponse::error))
    {
    cout << "Error: no response" << endl;
    }
  else
    {
    result = msg.GetExtension(InvokeResponse::arguments).variant(0).integer(0);
    cout << "Reply: " << result << endl;
    cout << "ERROR Exiting..." << endl;
    return EXIT_FAILURE;
    }

  proxy->Delete();

  cout << "+++++++++++++++++++" << endl;
  result = -1;

  proxy = pxm->NewProxy("sources","TIFFReader");
  proxy->UpdateVTKObjects();

  msg.Clear();
  msg.set_global_id(proxy->GetGlobalID());
  msg.set_location(proxy->GetLocation());
  msg << pvstream::InvokeRequestNoWarning() << "CanReadFile" << "/tmp/fake.tiff";
  session->Invoke(&msg);

  if(msg.GetExtension(InvokeResponse::error))
    {
    cout << "Error: no response" << endl;
    cout << "ERROR Exiting..." << endl;
    return EXIT_FAILURE;
    }
  else
    {
    result = msg.GetExtension(InvokeResponse::arguments).variant(0).integer(0);
    cout << "Reply: " << result << endl;
    if(result != 0)
      {
      cout << "ERROR Exiting..." << endl;
      return EXIT_FAILURE;
      }
    }

  proxy->Delete();

  cout << "+++++++++++++++++++" << endl;

  cout << "SUCCESS Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
