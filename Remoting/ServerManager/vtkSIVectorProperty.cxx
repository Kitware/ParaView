/*=========================================================================

  Program:   ParaView
  Module:    vtkSIVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIVectorProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkSIVectorProperty::vtkSIVectorProperty()
{
  this->NumberOfElementsPerCommand = 1;
  this->UseIndex = false;
  this->CleanCommand = NULL;
  this->SetNumberCommand = NULL;
  this->InitialString = NULL;
}

//----------------------------------------------------------------------------
vtkSIVectorProperty::~vtkSIVectorProperty()
{
  this->SetCleanCommand(0);
  this->SetSetNumberCommand(0);
  this->SetInitialString(0);
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
