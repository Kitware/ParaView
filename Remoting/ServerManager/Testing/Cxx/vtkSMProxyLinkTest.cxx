// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMProxyLinkTest.h"

#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

void vtkSMProxyLinkTest::LinkProxies()
{
  vtkNew<vtkSMSession> session;
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> sphere1 =
    vtk::TakeSmartPointer(pxm->NewProxy("sources", "SphereSource"));
  vtkSmartPointer<vtkSMProxy> sphere2 =
    vtk::TakeSmartPointer(pxm->NewProxy("sources", "SphereSource"));

  vtkNew<vtkSMProxyLink> link;
  // this should create the 4 combinations of 2 proxies in 2 direction.
  link->LinkProxies(sphere1, sphere2);
  QCOMPARE(link->GetNumberOfLinkedObjects(), 4U);
  QVERIFY(link->GetLinkedProxy(0) == sphere1);
  QVERIFY(link->GetLinkedProxy(1) == sphere2);
  QVERIFY(link->GetLinkedProxy(2) == sphere2);
  QVERIFY(link->GetLinkedProxy(3) == sphere1);
  QVERIFY(link->GetLinkedObjectDirection(0) == vtkSMLink::INPUT);
  QVERIFY(link->GetLinkedObjectDirection(1) == vtkSMLink::OUTPUT);
  QVERIFY(link->GetLinkedObjectDirection(2) == vtkSMLink::INPUT);
  QVERIFY(link->GetLinkedObjectDirection(3) == vtkSMLink::OUTPUT);
}

void vtkSMProxyLinkTest::AddLinkedProxy()
{
  vtkNew<vtkSMSession> session;
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> sphere1 =
    vtk::TakeSmartPointer(pxm->NewProxy("sources", "SphereSource"));
  QVERIFY(sphere1 != nullptr);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 0.5);

  vtkSmartPointer<vtkSMProxy> sphere2 =
    vtk::TakeSmartPointer(pxm->NewProxy("sources", "SphereSource"));
  QVERIFY(sphere1 != nullptr);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 0.5);

  vtkNew<vtkSMProxyLink> link;
  QCOMPARE(link->GetNumberOfLinkedObjects(), 0U);

  link->AddLinkedProxy(sphere1, vtkSMLink::INPUT);
  QCOMPARE(link->GetNumberOfLinkedObjects(), 1U);
  QVERIFY(link->GetLinkedProxy(0) == sphere1);
  QVERIFY(link->GetLinkedObjectDirection(0) == vtkSMLink::INPUT);

  link->AddLinkedProxy(sphere2, vtkSMLink::OUTPUT);
  QCOMPARE(link->GetNumberOfLinkedObjects(), 2U);
  QVERIFY(link->GetLinkedProxy(1) == sphere2);
  QVERIFY(link->GetLinkedObjectDirection(1) == vtkSMLink::OUTPUT);

  vtkSMPropertyHelper(sphere1, "Radius").Set(1.3);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 1.3);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 1.3);

  link->RemoveLinkedProxy(sphere2);
  vtkSMPropertyHelper(sphere1, "Radius").Set(2.6);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 2.6);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 1.3);
}

void vtkSMProxyLinkTest::AddException()
{
  vtkNew<vtkSMSession> session;
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> sphere1 =
    vtk::TakeSmartPointer(pxm->NewProxy("sources", "SphereSource"));
  vtkSmartPointer<vtkSMProxy> sphere2 =
    vtk::TakeSmartPointer(pxm->NewProxy("sources", "SphereSource"));

  vtkNew<vtkSMProxyLink> link;
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
}
