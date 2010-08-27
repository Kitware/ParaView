/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMVectorProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkPMVectorProperty::vtkPMVectorProperty()
{
  this->NumberOfElementsPerCommand = 1;
  this->UseIndex = false;
  this->CleanCommand = NULL;
  this->SetNumberCommand = NULL;
}

//----------------------------------------------------------------------------
vtkPMVectorProperty::~vtkPMVectorProperty()
{
  this->SetCleanCommand(0);
  this->SetSetNumberCommand(0);
}

//---------------------------------------------------------------------------
bool vtkPMVectorProperty::ReadXMLAttributes(
  vtkPMProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
    {
    return false;
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
  if (element->GetScalarAttribute(
      "number_of_elements_per_command", &numElsPerCommand))
    {
    this->NumberOfElementsPerCommand = numElsPerCommand;
    }

  const char* clean_command = element->GetAttribute("clean_command");
  if (clean_command)
    {
    this->SetCleanCommand(clean_command);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPMVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
