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

#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"

void vtkSMStringVectorPropertyTest::SetNumberOfElements()
{
  vtkSMStringVectorProperty* smproperty = vtkSMStringVectorProperty::New();
  QCOMPARE(smproperty->GetNumberOfElements(), 0U);

  smproperty->SetNumberOfElements(5);
  QCOMPARE(smproperty->GetNumberOfElements(), 5U);

  smproperty->SetNumberOfElements(2);
  QCOMPARE(smproperty->GetNumberOfElements(), 2U);

  smproperty->Delete();
}

void vtkSMStringVectorPropertyTest::SetElement()
{
  vtkSMStringVectorProperty* smproperty = vtkSMStringVectorProperty::New();

  smproperty->SetElement(2, "Hello");
  QCOMPARE(smproperty->GetNumberOfElements(), 3U);
  QCOMPARE(smproperty->GetElement(0), "");
  QCOMPARE(smproperty->GetElement(1), "");
  QCOMPARE(smproperty->GetElement(2), "Hello");

  smproperty->Delete();
}

void vtkSMStringVectorPropertyTest::SetElements()
{
  vtkSMStringVectorProperty* smproperty = vtkSMStringVectorProperty::New();

  const char* values[] = { "Para", "View" };
  smproperty->SetElements(values, 2);
  QCOMPARE(smproperty->GetNumberOfElements(), 2U);
  QCOMPARE(smproperty->GetElement(0), "Para");
  QCOMPARE(smproperty->GetElement(1), "View");

  smproperty->Delete();
}

void vtkSMStringVectorPropertyTest::Copy()
{
  vtkSMStringVectorProperty* property1 = vtkSMStringVectorProperty::New();
  vtkSMStringVectorProperty* property2 = vtkSMStringVectorProperty::New();

  property1->SetElement(0, "Test");
  property1->SetElement(1, "tseT");
  property2->Copy(property1);
  QCOMPARE(property2->GetNumberOfElements(), 2U);
  QCOMPARE(property2->GetElement(0), "Test");
  QCOMPARE(property2->GetElement(1), "tseT");

  property1->Delete();
  property2->Delete();
}
