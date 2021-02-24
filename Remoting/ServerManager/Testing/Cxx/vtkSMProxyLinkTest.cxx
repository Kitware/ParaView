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

#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

void vtkSMProxyLinkTest::AddLinkedProxy()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSMProxy* sphere1 = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(sphere1 != nullptr);
  QCOMPARE(vtkSMPropertyHelper(sphere1, "Radius").GetAsDouble(), 0.5);

  vtkSMProxy* sphere2 = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(sphere1 != nullptr);
  QCOMPARE(vtkSMPropertyHelper(sphere2, "Radius").GetAsDouble(), 0.5);

  vtkSMProxyLink* link = vtkSMProxyLink::New();
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

  link->Delete();
  sphere1->Delete();
  sphere2->Delete();
  session->Delete();
}

void vtkSMProxyLinkTest::AddException()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSMProxy* sphere1 = pxm->NewProxy("sources", "SphereSource");
  vtkSMProxy* sphere2 = pxm->NewProxy("sources", "SphereSource");

  vtkSMProxyLink* link = vtkSMProxyLink::New();
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
