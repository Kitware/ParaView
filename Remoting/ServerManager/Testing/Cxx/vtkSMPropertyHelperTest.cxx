// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMPropertyHelperTest.h"

#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

void vtkSMPropertyHelperTest::Set()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(proxy != nullptr);
  QCOMPARE(vtkSMPropertyHelper(proxy, "Radius").GetAsDouble(), 0.5);

  vtkSMPropertyHelper(proxy, "Radius").Set(4.2);
  QCOMPARE(vtkSMPropertyHelper(proxy, "Radius").GetAsDouble(), 4.2);

  vtkSMPropertyHelper(proxy, "Radius").Set(8.4);
  QCOMPARE(vtkSMPropertyHelper(proxy, "Radius").GetAsDouble(), 8.4);

  proxy->Delete();
  session->Delete();
}

void vtkSMPropertyHelperTest::Contains()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSMProxy* sphere1 = pxm->NewProxy("sources", "SphereSource");
  vtkSMProxy* sphere2 = pxm->NewProxy("sources", "SphereSource");
  vtkSMProxy* group = pxm->NewProxy("filters", "GroupDataSets");

  QVERIFY(sphere1 != nullptr);
  QVERIFY(sphere2 != nullptr);
  QVERIFY(group != nullptr);

  vtkSMProperty* inputProp = group->GetProperty("Input");
  QVERIFY(inputProp != nullptr);

  vtkSMPropertyHelper helper(inputProp);

  QVERIFY(!helper.Contains(sphere1));

  helper.Add(sphere1, 0);
  QVERIFY(helper.Contains(sphere1));
  QVERIFY(!helper.Contains(sphere2));

  helper.Add(sphere2, 0);
  QVERIFY(helper.Contains(sphere1));
  QVERIFY(helper.Contains(sphere2));

  helper.Remove(sphere1);
  QVERIFY(!helper.Contains(sphere1));
  QVERIFY(helper.Contains(sphere2));

  sphere1->Delete();
  sphere2->Delete();
  group->Delete();
  session->Delete();
}
