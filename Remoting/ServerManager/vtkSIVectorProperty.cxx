// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIVectorProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkSIVectorProperty::vtkSIVectorProperty()
{
  this->NumberOfElementsPerCommand = 1;
  this->UseIndex = false;
  this->CleanCommand = nullptr;
  this->SetNumberCommand = nullptr;
  this->InitialString = nullptr;
}

//----------------------------------------------------------------------------
vtkSIVectorProperty::~vtkSIVectorProperty()
{
  this->SetCleanCommand(nullptr);
  this->SetSetNumberCommand(nullptr);
  this->SetInitialString(nullptr);
}

//---------------------------------------------------------------------------
bool vtkSIVectorProperty::ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
  {
    return false;
    ;
  }

  const char* numCommand = element->GetAttribute("set_number_command");
  if (numCommand)
  {
    this->SetSetNumberCommand(numCommand);
  }

  int use_index;
  if (element->GetScalarAttribute("use_index", &use_index))
  {
    this->UseIndex = (use_index != 0);
  }

  int numElsPerCommand;
  if (element->GetScalarAttribute("number_of_elements_per_command", &numElsPerCommand))
  {
    this->NumberOfElementsPerCommand = numElsPerCommand;
  }

  const char* clean_command = element->GetAttribute("clean_command");
  if (clean_command)
  {
    this->SetCleanCommand(clean_command);
  }

  const char* initial_string = element->GetAttribute("initial_string");
  if (initial_string)
  {
    this->SetInitialString(initial_string);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSIVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
