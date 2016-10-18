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

#include "vtkSMProxyTest.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

void vtkSMProxyTest::SetAnnotation()
{
  vtkSMProxy* proxy = vtkSMProxy::New();

  // set annotations
  proxy->SetAnnotation("Color", "#FFAA00");
  proxy->SetAnnotation("Tooltip", "Just a sphere");
  proxy->SetAnnotation("Owner", "Seb");
  QCOMPARE(proxy->GetNumberOfAnnotations(), 3);
  QVERIFY(proxy->HasAnnotation("Color"));
  QVERIFY(proxy->HasAnnotation("Tooltip"));
  QVERIFY(proxy->HasAnnotation("Owner"));

  // remove owner annotation
  proxy->SetAnnotation("Owner", NULL);
  QCOMPARE(proxy->GetNumberOfAnnotations(), 2);
  QVERIFY(proxy->HasAnnotation("Color"));
  QVERIFY(proxy->HasAnnotation("Tooltip"));

  // remove tooltip annotation
  proxy->RemoveAnnotation("Tooltip");
  QCOMPARE(proxy->GetNumberOfAnnotations(), 1);
  QVERIFY(proxy->HasAnnotation("Color"));

  // remove all annotations
  proxy->RemoveAllAnnotations();
  QCOMPARE(proxy->GetNumberOfAnnotations(), 0);

  // add annotations
  proxy->SetAnnotation("Color", "#FFAA00");
  proxy->SetAnnotation("Tooltip", "Just a sphere");
  proxy->SetAnnotation("Owner", "Seb");
  QCOMPARE(proxy->GetNumberOfAnnotations(), 3);
  QVERIFY(proxy->HasAnnotation("Color"));
  QVERIFY(proxy->HasAnnotation("Tooltip"));
  QVERIFY(proxy->HasAnnotation("Owner"));

  // remove all annotations
  proxy->RemoveAllAnnotations();
  QCOMPARE(proxy->GetNumberOfAnnotations(), 0);

  proxy->Delete();
}

void vtkSMProxyTest::GetProperty()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
  QVERIFY(proxy != NULL);

  // get 'Center' smproperty
  vtkSMProperty* smproperty = proxy->GetProperty("Center");
  QVERIFY(smproperty != NULL);
  QCOMPARE(proxy->GetPropertyName(smproperty), "Center");

  // try to get an invalid smproperty
  smproperty = proxy->GetProperty("NonexistantSphereProperty");
  QVERIFY(smproperty == NULL);
  QVERIFY(proxy->GetPropertyName(smproperty) == NULL);

  proxy->Delete();
  session->Delete();
}

void vtkSMProxyTest::GetVTKClassName()
{
  vtkSMProxy* proxy = vtkSMProxy::New();
  QVERIFY(proxy->GetVTKClassName() == NULL);

  proxy->SetVTKClassName("vtkSphereSource");
  QCOMPARE(proxy->GetVTKClassName(), "vtkSphereSource");

  proxy->Delete();
}
