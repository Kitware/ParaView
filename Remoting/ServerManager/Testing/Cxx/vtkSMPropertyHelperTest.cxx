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
