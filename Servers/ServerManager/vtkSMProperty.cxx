/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProperty.h"

#include "vtkPVXMLElement.h"

vtkCxxRevisionMacro(vtkSMProperty, "1.1");

//---------------------------------------------------------------------------
vtkSMProperty::vtkSMProperty()
{
  this->Command = 0;
}

//---------------------------------------------------------------------------
vtkSMProperty::~vtkSMProperty()
{
  this->SetCommand(0);
}

//---------------------------------------------------------------------------
int vtkSMProperty::ReadXMLAttributes(vtkPVXMLElement* element)
{
  const char* command = element->GetAttribute("command");
  if(command) 
    { 
    this->SetCommand(command); 
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Command: " 
     << (this->Command ? this->Command : "(null)") << endl;
}
