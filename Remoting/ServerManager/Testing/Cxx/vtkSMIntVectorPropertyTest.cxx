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

#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"

void vtkSMIntVectorPropertyTest::SetNumberOfElements()
{
  vtkSMIntVectorProperty* smproperty = vtkSMIntVectorProperty::New();
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

void vtkSMIntVectorPropertyTest::SetElement()
{
  vtkSMIntVectorProperty* smproperty = vtkSMIntVectorProperty::New();
  smproperty->SetNumberOfElements(2);
  QCOMPARE(smproperty->GetNumberOfElements(), 2U);
  QCOMPARE(smproperty->GetElement(0), 0);
  QCOMPARE(smproperty->GetElement(1), 0);

  smproperty->SetElement(0, 2);
  smproperty->SetElement(1, 4);
  QCOMPARE(smproperty->GetElement(0), 2);
  QCOMPARE(smproperty->GetElement(1), 4);

  smproperty->SetElement(2, 8);
  QCOMPARE(smproperty->GetNumberOfElements(), 3U);
  QCOMPARE(smproperty->GetElement(0), 2);
  QCOMPARE(smproperty->GetElement(1), 4);
  QCOMPARE(smproperty->GetElement(2), 8);

  smproperty->Delete();
}

void vtkSMIntVectorPropertyTest::SetElements()
{
  vtkSMIntVectorProperty* smproperty = vtkSMIntVectorProperty::New();

  int values[] = { 9, 18, 27 };
  smproperty->SetElements(values, 3);
  QCOMPARE(smproperty->GetNumberOfElements(), 3U);
  QCOMPARE(smproperty->GetElement(0), 9);
  QCOMPARE(smproperty->GetElement(1), 18);
  QCOMPARE(smproperty->GetElement(2), 27);

  smproperty->Delete();
}

void vtkSMIntVectorPropertyTest::Copy()
{
  vtkSMIntVectorProperty* property1 = vtkSMIntVectorProperty::New();
  vtkSMIntVectorProperty* property2 = vtkSMIntVectorProperty::New();

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
