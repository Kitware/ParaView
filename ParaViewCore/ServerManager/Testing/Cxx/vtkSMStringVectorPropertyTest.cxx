/*=========================================================================

Program:   ParaView
Module:    vtkSMStringVectorPropertyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMStringVectorPropertyTest.h"

#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkSMStringVectorProperty.h"

void vtkSMStringVectorPropertyTest::SetNumberOfElements()
{
  vtkSMStringVectorProperty *property = vtkSMStringVectorProperty::New();
  QCOMPARE(property->GetNumberOfElements(), 0U);

  property->SetNumberOfElements(5);
  QCOMPARE(property->GetNumberOfElements(), 5U);

  property->SetNumberOfElements(2);
  QCOMPARE(property->GetNumberOfElements(), 2U);

  property->Delete();
}

void vtkSMStringVectorPropertyTest::SetElement()
{
  vtkSMStringVectorProperty *property = vtkSMStringVectorProperty::New();

  property->SetElement(2, "Hello");
  QCOMPARE(property->GetNumberOfElements(), 3U);
  QCOMPARE(property->GetElement(0), "");
  QCOMPARE(property->GetElement(1), "");
  QCOMPARE(property->GetElement(2), "Hello");

  property->Delete();
}

void vtkSMStringVectorPropertyTest::SetElements()
{
  vtkSMStringVectorProperty *property = vtkSMStringVectorProperty::New();

  const char *values[] = {"Para", "View"};
  property->SetElements(values, 2);
  QCOMPARE(property->GetNumberOfElements(), 2U);
  QCOMPARE(property->GetElement(0), "Para");
  QCOMPARE(property->GetElement(1), "View");

  property->Delete();
}

void vtkSMStringVectorPropertyTest::Copy()
{
  vtkSMStringVectorProperty *property1 = vtkSMStringVectorProperty::New();
  vtkSMStringVectorProperty *property2 = vtkSMStringVectorProperty::New();

  property1->SetElement(0, "Test");
  property1->SetElement(1, "tseT");
  property2->Copy(property1);
  QCOMPARE(property2->GetNumberOfElements(), 2U);
  QCOMPARE(property2->GetElement(0), "Test");
  QCOMPARE(property2->GetElement(1), "tseT");

  property1->Delete();
  property2->Delete();
}

int main(int argc, char *argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);

  vtkSMStringVectorPropertyTest test;
  int ret = QTest::qExec(&test, argc, argv);

  vtkInitializationHelper::Finalize();
  options->Delete();

  return ret;
}
