/*=========================================================================

  Program:   ParaView
  Module:    vtkSMVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMVectorProperty.h"

#include "vtkPVXMLElement.h"

//---------------------------------------------------------------------------
vtkSMVectorProperty::vtkSMVectorProperty()
{
  this->RepeatCommand = 0;
  this->NumberOfElementsPerCommand = 1;
  this->UseIndex = 0;
  this->CleanCommand = 0;
  this->IsInternal = 0;
  this->SetNumberCommand = 0;
  this->InitialString = 0; // TODO(jpocom) check
}

//---------------------------------------------------------------------------
vtkSMVectorProperty::~vtkSMVectorProperty()
{
  this->SetCleanCommand(0);
  this->SetSetNumberCommand(0);
  this->SetInitialString(0); // TODO(jpocom) check
}

//---------------------------------------------------------------------------
int vtkSMVectorProperty::ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(parent, element);
  if (!retVal)
  {
    return retVal;
  }

  const char* numCommand = element->GetAttribute("set_number_command");
  if (numCommand)
  {
    this->SetSetNumberCommand(numCommand);
  }

  int use_index;
  retVal = element->GetScalarAttribute("use_index", &use_index);
  if (retVal)
  {
    this->SetUseIndex(use_index);
  }
  int repeat_command;
  retVal = element->GetScalarAttribute("repeat_command", &repeat_command);
  if (retVal)
  {
    this->SetRepeatCommand(repeat_command);
    this->Repeatable = repeat_command;
  }
  int numElsPerCommand;
  retVal = element->GetScalarAttribute("number_of_elements_per_command", &numElsPerCommand);
  if (retVal)
  {
    this->SetNumberOfElementsPerCommand(numElsPerCommand);
  }
  int numEls;
  retVal = element->GetScalarAttribute("number_of_elements", &numEls);
  if (retVal)
  {
    this->SetNumberOfElements(numEls);
  }

  const char* clean_command = element->GetAttribute("clean_command");
  if (clean_command)
  {
    this->SetCleanCommand(clean_command);
  }

  // TODO(jpocom) check
  const char* initial_string = element->GetAttribute("initial_string");
  if (initial_string)
  {
    this->SetInitialString(initial_string);
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);
}

//---------------------------------------------------------------------------
bool vtkSMVectorProperty::ResetToDomainDefaults(bool use_unchecked_values)
{
  if (this->Superclass::ResetToDomainDefaults(use_unchecked_values))
  {
    return true;
  }

  // If none of the domains picked a default, maybe there's an information
  // property that wants to provide use with a default.
  if (vtkSMVectorProperty* infoProp =
        vtkSMVectorProperty::SafeDownCast(this->GetInformationProperty()))
  {
    if (infoProp->GetNumberOfElements() > 0)
    {
      this->Copy(infoProp);
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSMVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfElements: " << this->GetNumberOfElements() << endl;
  os << indent << "NumberOfElementsPerCommand: " << this->GetNumberOfElementsPerCommand() << endl;
  os << indent << "RepeatCommand: " << this->RepeatCommand << endl;
  os << indent << "CleanCommand: " << (this->CleanCommand ? this->CleanCommand : "(null)") << endl;
  os << indent << "UseIndex: " << this->UseIndex << endl;
  os << indent
     << "SetNumberCommand: " << (this->SetNumberCommand ? this->SetNumberCommand : "(null)")
     << endl;
}
