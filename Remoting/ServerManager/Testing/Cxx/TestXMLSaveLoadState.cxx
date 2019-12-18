/*=========================================================================

Program:   ParaView
Module:    pvstatetest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
int TestXMLSaveLoadState(int argc, char* argv[])
{
  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT, options);

  //---------------------------------------------------------------------------
  int return_value = EXIT_SUCCESS;
  vtkSmartPointer<vtkPVXMLElement> xmlRootNodeOrigin;
  vtkSmartPointer<vtkPVXMLElement> xmlRootNodeLoaded;
  //---------------------------------------------------------------------------
  vtkSMSession* session = vtkSMSession::New();
  cout << "==== Starting ====" << endl;
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);

  vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
  vtkSMPropertyHelper(proxy, "PhiResolution").Set(20);
  vtkSMPropertyHelper(proxy, "ThetaResolution").Set(20);
  proxy->UpdateVTKObjects();

  vtkSMSourceProxy* shrink =
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ShrinkFilter"));

  vtkSMPropertyHelper(shrink, "Input").Set(proxy);
  shrink->UpdateVTKObjects();
  shrink->UpdatePipeline();

  // shrink->GetDataInformation(0)->Print(cout);

  pxm->RegisterProxy("sources", "sphere", proxy);
  pxm->RegisterProxy("filters", "shrink", shrink);

  // Try to build XML state
  xmlRootNodeOrigin.TakeReference(pxm->SaveXMLState());
  xmlRootNodeOrigin->PrintXML();

  cout << "==== End of State creation ===" << endl;

  cout << "==== Clear proxyManager state ===" << endl;
  pxm->UnRegisterProxies();
  ;
  proxy->Delete();
  shrink->Delete();

  cout << "==== Make sure that the state is empty ===" << endl;
  xmlRootNodeLoaded.TakeReference(pxm->SaveXMLState());
  // xmlRootNodeLoaded->PrintXML();
  if (pxm->GetProxy("sources", "sphere") && pxm->GetProxy("filters", "shrink"))
  {
    cout << " - Error in clearing" << endl;
    return_value = EXIT_FAILURE;
  }
  else
  {
    cout << " - Clearing done" << endl;
  }

  cout << "==== Loading previous state ====" << endl;
  pxm->LoadXMLState(xmlRootNodeOrigin);
  xmlRootNodeLoaded.TakeReference(pxm->SaveXMLState());
  xmlRootNodeLoaded->PrintXML();
  cout << "==== End of state loading ====" << endl;

  //---------------------------------------------------------------------------
  if (pxm->GetProxy("sources", "sphere") && pxm->GetProxy("filters", "shrink") &&
    return_value == EXIT_SUCCESS)
  {
    cout << endl << " ### SUCCESS: States are equals ###" << endl;
  }
  else
  {
    cout << endl << " ### FAILED: States are NOT equals ###" << endl;
    return_value = EXIT_FAILURE;
  }
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
