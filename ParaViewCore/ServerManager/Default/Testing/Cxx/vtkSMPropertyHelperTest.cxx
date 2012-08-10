/*=========================================================================

Program:   ParaView
Module:    vtkSMPropertyHelperTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPropertyHelperTest.h"

#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMPropertyHelper.h"

void vtkSMPropertyHelperTest::Set()
{
  vtkSMSession *session = vtkSMSession::New();
  vtkSMSessionProxyManager *pxm = session->GetSessionProxyManager();
  vtkSMProxy *proxy = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(proxy != NULL);
  QCOMPARE(vtkSMPropertyHelper(proxy, "Radius").GetAsDouble(), 0.5);

  vtkSMPropertyHelper(proxy, "Radius").Set(4.2);
  QCOMPARE(vtkSMPropertyHelper(proxy, "Radius").GetAsDouble(), 4.2);

  vtkSMPropertyHelper(proxy, "Radius").Set(8.4);
  QCOMPARE(vtkSMPropertyHelper(proxy, "Radius").GetAsDouble(), 8.4);

  proxy->Delete();
  session->Delete();
}

int main(int argc, char *argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);

  vtkSMPropertyHelperTest test;
  int ret = QTest::qExec(&test, argc, argv);

  vtkInitializationHelper::Finalize();
  options->Delete();

  return ret;
}
