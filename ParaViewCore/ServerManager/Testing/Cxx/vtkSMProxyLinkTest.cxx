/*=========================================================================

Program:   ParaView
Module:    vtkSMProxyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMProxyLinkTest.h"

#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMProxyLink.h"
#include "vtkSMPropertyHelper.h"

void vtkSMProxyLinkTest::AddLinkedProxy()
{
  vtkSMSession *session = vtkSMSession::New();
  vtkSMSessionProxyManager *pxm = session->GetSessionProxyManager();

  vtkSMProxy *sphere1 = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(sphere1 != NULL);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 0.5);

  vtkSMProxy *sphere2 = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(sphere1 != NULL);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 0.5);

  vtkSMProxyLink *link = vtkSMProxyLink::New();
  QCOMPARE(link->GetNumberOfLinkedProxies(), 0U);

  link->AddLinkedProxy(sphere1, vtkSMLink::INPUT);
  QCOMPARE(link->GetNumberOfLinkedProxies(), 1U);
  QVERIFY(link->GetLinkedProxy(0) == sphere1);
  QVERIFY(link->GetLinkedProxyDirection(0) == vtkSMLink::INPUT);

  link->AddLinkedProxy(sphere2, vtkSMLink::OUTPUT);
  QCOMPARE(link->GetNumberOfLinkedProxies(), 2U);
  QVERIFY(link->GetLinkedProxy(1) == sphere2);
  QVERIFY(link->GetLinkedProxyDirection(1) == vtkSMLink::OUTPUT);

  vtkSMPropertyHelper(sphere1, "Radius").Set(1.3);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 1.3);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 1.3);

  link->RemoveLinkedProxy(sphere2);
  vtkSMPropertyHelper(sphere1, "Radius").Set(2.6);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 2.6);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 1.3);

  link->Delete();
  sphere1->Delete();
  sphere2->Delete();
  session->Delete();
}

void vtkSMProxyLinkTest::AddException()
{
  vtkSMSession *session = vtkSMSession::New();
  vtkSMSessionProxyManager *pxm = session->GetSessionProxyManager();

  vtkSMProxy *sphere1 = pxm->NewProxy("sources", "SphereSource");
  vtkSMProxy *sphere2 = pxm->NewProxy("sources", "SphereSource");

  vtkSMProxyLink *link = vtkSMProxyLink::New();
  link->AddLinkedProxy(sphere1, vtkSMLink::INPUT);
  link->AddLinkedProxy(sphere2, vtkSMLink::OUTPUT);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "ThetaResolution").GetAsInt(), 8);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "ThetaResolution").GetAsInt(), 8);

  link->AddException("ThetaResolution");
  vtkSMPropertyHelper(sphere1, "ThetaResolution").Set(10);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "ThetaResolution").GetAsInt(), 10);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "ThetaResolution").GetAsInt(), 8);

  vtkSMPropertyHelper(sphere1, "PhiResolution").Set(12);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "PhiResolution").GetAsInt(), 12);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "PhiResolution").GetAsInt(), 12);

  link->RemoveException("ThetaResolution");
  vtkSMPropertyHelper(sphere1, "ThetaResolution").Set(14);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "ThetaResolution").GetAsInt(), 14);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "ThetaResolution").GetAsInt(), 14);

  link->Delete();
  sphere1->Delete();
  sphere2->Delete();
  session->Delete();
}

int main(int argc, char *argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);

  vtkSMProxyLinkTest test;
  int ret = QTest::qExec(&test, argc, argv);

  vtkInitializationHelper::Finalize();
  options->Delete();

  return ret;
}
