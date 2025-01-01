// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#include <string>

extern int TestExecutableRunner(int, char* argv[])
{
  vtkInitializationHelper::SetApplicationName("TestExecutableRunner");
  vtkInitializationHelper::SetOrganizationName("Humanity");
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkNew<vtkSMSession> session;

  // Register the session with the process module.
  vtkProcessModule::GetProcessModule()->RegisterSession(session.Get());

  // Initializes a session and setups all basic proxies that are needed for a
  // ParaView-like application.
  controller->InitializeSession(session.Get());

  // Setup a proxy to a command line process
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "ExecutableRunner"));
  if (!proxy)
  {
    vtkGenericWarningMacro("Failed to create proxy: `misc,ExecutableRunner`. Aborting !!!");
    abort();
  }

  // Call a command line process
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  vtkSMPropertyHelper(proxy->GetProperty("Command")).Set("cmd.exe /c echo Hello World");
#else
  vtkSMPropertyHelper(proxy->GetProperty("Command")).Set("echo Hello World");
#endif
  proxy->UpdateVTKObjects();
  proxy->InvokeCommand("Execute");
  auto* outProp = proxy->GetProperty("StdOut");
  proxy->UpdatePropertyInformation(outProp);
  std::string result = vtkSMPropertyHelper(outProp).GetAsString();

  // Check if output is good
  if (result != "Hello World")
  {
    vtkGenericWarningMacro(
      "Wrong output, command line failed. Expected: 'Hello World', received: '" << result << "'");
    abort();
  }

  // Unregistering pipeline proxies will also release any representations
  // created for these proxies.
  controller->UnRegisterProxy(proxy);

  vtkProcessModule::GetProcessModule()->UnRegisterSession(session.Get());
  vtkInitializationHelper::Finalize();
  return 0;
}
