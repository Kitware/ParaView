// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInitializationHelper.h"
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "vtkPVXMLElement.h"

#include <iostream>

//----------------------------------------------------------------------------
extern int TestXMLSaveLoadState(int argc, char* argv[])
{
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);

  //---------------------------------------------------------------------------
  int return_value = EXIT_SUCCESS;
  vtkSmartPointer<vtkPVXMLElement> xmlRootNodeOrigin;
  vtkSmartPointer<vtkPVXMLElement> xmlRootNodeLoaded;
  //---------------------------------------------------------------------------
  vtkSMSession* session = vtkSMSession::New();
  std::cout << "==== Starting ====" << endl;
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

  // shrink->GetDataInformation(0)->Print(std::cout);

  pxm->RegisterProxy("sources", "sphere", proxy);
  pxm->RegisterProxy("filters", "shrink", shrink);

  // Try to build XML state
  xmlRootNodeOrigin.TakeReference(pxm->SaveXMLState());
  xmlRootNodeOrigin->PrintXML();

  std::cout << "==== End of State creation ===" << endl;

  std::cout << "==== Clear proxyManager state ===" << endl;
  pxm->UnRegisterProxies();
  ;
  proxy->Delete();
  shrink->Delete();

  std::cout << "==== Make sure that the state is empty ===" << endl;
  xmlRootNodeLoaded.TakeReference(pxm->SaveXMLState());
  // xmlRootNodeLoaded->PrintXML();
  if (pxm->GetProxy("sources", "sphere") && pxm->GetProxy("filters", "shrink"))
  {
    std::cout << " - Error in clearing" << endl;
    return_value = EXIT_FAILURE;
  }
  else
  {
    std::cout << " - Clearing done" << endl;
  }

  std::cout << "==== Loading previous state ====" << endl;
  pxm->LoadXMLState(xmlRootNodeOrigin);
  xmlRootNodeLoaded.TakeReference(pxm->SaveXMLState());
  xmlRootNodeLoaded->PrintXML();
  std::cout << "==== End of state loading ====" << endl;

  //---------------------------------------------------------------------------
  if (pxm->GetProxy("sources", "sphere") && pxm->GetProxy("filters", "shrink") &&
    return_value == EXIT_SUCCESS)
  {
    std::cout << endl << " ### SUCCESS: States are equals ###" << endl;
  }
  else
  {
    std::cout << endl << " ### FAILED: States are NOT equals ###" << endl;
    return_value = EXIT_FAILURE;
  }
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  return return_value;
}
