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

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"

void vtkSMDoubleVectorPropertyTest::SetNumberOfElements()
{
  vtkSMDoubleVectorProperty* smproperty = vtkSMDoubleVectorProperty::New();
  QCOMPARE(smproperty->GetNumberOfElements(), 0U);

  smproperty->SetNumberOfElements(4);
  QCOMPARE(smproperty->GetNumberOfElements(), 4U);

  smproperty->SetNumberOfElements(14);
  QCOMPARE(smproperty->GetNumberOfElements(), 14U);

  smproperty->SetNumberOfElements(6);
  QCOMPARE(smproperty->GetNumberOfElements(), 6U);

  smproperty->SetNumberOfElements(0);
  QCOMPARE(smproperty->GetNumberOfElements(), 0U);

  smproperty->Delete();
}

void vtkSMDoubleVectorPropertyTest::SetElement()
{
  vtkSMDoubleVectorProperty* smproperty = vtkSMDoubleVectorProperty::New();
  smproperty->SetNumberOfElements(2);
  QCOMPARE(smproperty->GetNumberOfElements(), 2U);
  QCOMPARE(smproperty->GetElement(0), 0.0);
  QCOMPARE(smproperty->GetElement(1), 0.0);

  smproperty->SetElement(0, 2.5);
  smproperty->SetElement(1, 4.1);
  QCOMPARE(smproperty->GetElement(0), 2.5);
  QCOMPARE(smproperty->GetElement(1), 4.1);

  smproperty->SetElement(2, 0.8);
  QCOMPARE(smproperty->GetNumberOfElements(), 3U);
  QCOMPARE(smproperty->GetElement(0), 2.5);
  QCOMPARE(smproperty->GetElement(1), 4.1);
  QCOMPARE(smproperty->GetElement(2), 0.8);

  smproperty->Delete();
}

void vtkSMDoubleVectorPropertyTest::SetElements()
{
  vtkSMDoubleVectorProperty* smproperty = vtkSMDoubleVectorProperty::New();

  double values[] = { 9.5, 18.7, 27.9 };
  smproperty->SetElements(values, 3);
  QCOMPARE(smproperty->GetNumberOfElements(), 3U);
  QCOMPARE(smproperty->GetElement(0), 9.5);
  QCOMPARE(smproperty->GetElement(1), 18.7);
  QCOMPARE(smproperty->GetElement(2), 27.9);

  smproperty->Delete();
}

void vtkSMDoubleVectorPropertyTest::Copy()
{
  vtkSMDoubleVectorProperty* property1 = vtkSMDoubleVectorProperty::New();
  vtkSMDoubleVectorProperty* property2 = vtkSMDoubleVectorProperty::New();

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
