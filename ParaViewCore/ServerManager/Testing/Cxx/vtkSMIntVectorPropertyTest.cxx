/*=========================================================================

Program:   ParaView
Module:    vtkSMIntVectorPropertyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMIntVectorPropertyTest.h"

#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"

void vtkSMIntVectorPropertyTest::SetNumberOfElements()
{
  vtkSMIntVectorProperty *property = vtkSMIntVectorProperty::New();
  QCOMPARE(property->GetNumberOfElements(), 0U);

  property->SetNumberOfElements(4);
  QCOMPARE(property->GetNumberOfElements(), 4U);

  property->SetNumberOfElements(14);
  QCOMPARE(property->GetNumberOfElements(), 14U);

  property->SetNumberOfElements(6);
  QCOMPARE(property->GetNumberOfElements(), 6U);

  property->SetNumberOfElements(0);
  QCOMPARE(property->GetNumberOfElements(), 0U);

  property->Delete();
}

void vtkSMIntVectorPropertyTest::SetElement()
{
  vtkSMIntVectorProperty *property = vtkSMIntVectorProperty::New();
  property->SetNumberOfElements(2);
  QCOMPARE(property->GetNumberOfElements(), 2U);
  QCOMPARE(property->GetElement(0), 0);
  QCOMPARE(property->GetElement(1), 0);

  property->SetElement(0, 2);
  property->SetElement(1, 4);
  QCOMPARE(property->GetElement(0), 2);
  QCOMPARE(property->GetElement(1), 4);

  property->SetElement(2, 8);
  QCOMPARE(property->GetNumberOfElements(), 3U);
  QCOMPARE(property->GetElement(0), 2);
  QCOMPARE(property->GetElement(1), 4);
  QCOMPARE(property->GetElement(2), 8);

  property->Delete();
}

void vtkSMIntVectorPropertyTest::SetElements()
{
  vtkSMIntVectorProperty *property = vtkSMIntVectorProperty::New();

  int values[] = {9, 18, 27};
  property->SetElements(values, 3);
  QCOMPARE(property->GetNumberOfElements(), 3U);
  QCOMPARE(property->GetElement(0), 9);
  QCOMPARE(property->GetElement(1), 18);
  QCOMPARE(property->GetElement(2), 27);

  property->Delete();
}

void vtkSMIntVectorPropertyTest::Copy()
{
  vtkSMIntVectorProperty *property1 = vtkSMIntVectorProperty::New();
  vtkSMIntVectorProperty *property2 = vtkSMIntVectorProperty::New();

  property1->SetElement(0, 5);
  property1->SetElement(1, 10);
  property1->SetElement(2, 15);
  QCOMPARE(property1->GetNumberOfElements(), 3U);

  property2->Copy(property1);
  QCOMPARE(property2->GetNumberOfElements(), 3U);
  QCOMPARE(property2->GetElement(0), 5);
  QCOMPARE(property2->GetElement(1), 10);
  QCOMPARE(property2->GetElement(2), 15);

  property1->Delete();
  property2->Delete();
}

int main(int argc, char *argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);

  vtkSMIntVectorPropertyTest test;
  int ret = QTest::qExec(&test, argc, argv);

  vtkInitializationHelper::Finalize();
  options->Delete();

  return ret;
}
