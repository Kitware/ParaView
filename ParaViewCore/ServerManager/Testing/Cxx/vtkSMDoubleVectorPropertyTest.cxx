/*=========================================================================

Program:   ParaView
Module:    vtkSMDoubleVectorPropertyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMDoubleVectorPropertyTest.h"

#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

void vtkSMDoubleVectorPropertyTest::SetNumberOfElements()
{
  vtkSMDoubleVectorProperty *property = vtkSMDoubleVectorProperty::New();
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

void vtkSMDoubleVectorPropertyTest::SetElement()
{
  vtkSMDoubleVectorProperty *property = vtkSMDoubleVectorProperty::New();
  property->SetNumberOfElements(2);
  QCOMPARE(property->GetNumberOfElements(), 2U);
  QCOMPARE(property->GetElement(0), 0.0);
  QCOMPARE(property->GetElement(1), 0.0);

  property->SetElement(0, 2.5);
  property->SetElement(1, 4.1);
  QCOMPARE(property->GetElement(0), 2.5);
  QCOMPARE(property->GetElement(1), 4.1);

  property->SetElement(2, 0.8);
  QCOMPARE(property->GetNumberOfElements(), 3U);
  QCOMPARE(property->GetElement(0), 2.5);
  QCOMPARE(property->GetElement(1), 4.1);
  QCOMPARE(property->GetElement(2), 0.8);

  property->Delete();
}

void vtkSMDoubleVectorPropertyTest::SetElements()
{
  vtkSMDoubleVectorProperty *property = vtkSMDoubleVectorProperty::New();

  double values[] = {9.5, 18.7, 27.9};
  property->SetElements(values, 3);
  QCOMPARE(property->GetNumberOfElements(), 3U);
  QCOMPARE(property->GetElement(0), 9.5);
  QCOMPARE(property->GetElement(1), 18.7);
  QCOMPARE(property->GetElement(2), 27.9);

  property->Delete();
}

void vtkSMDoubleVectorPropertyTest::Copy()
{
  vtkSMDoubleVectorProperty *property1 = vtkSMDoubleVectorProperty::New();
  vtkSMDoubleVectorProperty *property2 = vtkSMDoubleVectorProperty::New();

  property1->SetElement(0, 5.0);
  property1->SetElement(1, 10.1);
  property1->SetElement(2, 15.2);
  QCOMPARE(property1->GetNumberOfElements(), 3U);

  property2->Copy(property1);
  QCOMPARE(property2->GetNumberOfElements(), 3U);
  QCOMPARE(property2->GetElement(0), 5.0);
  QCOMPARE(property2->GetElement(1), 10.1);
  QCOMPARE(property2->GetElement(2), 15.2);

  property1->Delete();
  property2->Delete();
}

int main(int argc, char *argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);

  vtkSMDoubleVectorPropertyTest test;
  int ret = QTest::qExec(&test, argc, argv);

  vtkInitializationHelper::Finalize();
  options->Delete();

  return ret;
}
