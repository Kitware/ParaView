/*=========================================================================

Program:   ParaView
Module:    TestSMPropertyLabel.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMProperty.h"

#include <cstring>
#include <iostream>

bool CheckStringEquivalent(const char* tested, const char* reference)
{
  int res = strcmp(reference, tested);
  delete[] tested;
  if (res)
  {
    cerr << "Expected '" << reference << "' but got '" << tested << "'" << std::endl;
    return false;
  }
  return true;
}

int TestSMPropertyLabel(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MYSpace"), "MY Space"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MYSPACE"), "MYSPACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("My Space"), "My Space"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MySPACE"), "My SPACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MySPace"), "My S Pace"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MySPACE"), "My SPACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MySpACE"), "My Sp ACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(
        vtkSMProperty::CreateNewPrettyLabel("MYSuperSpacer"), "MY Super Spacer"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MySpACe"), "My Sp A Ce"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel("MySpAcE"), "My Sp Ac E"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel(""), ""))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMProperty::CreateNewPrettyLabel(nullptr), ""))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
