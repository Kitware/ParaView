/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDataArraySelectionProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIDataArraySelectionProperty.h"

#include "vtkClientServerStream.h"
#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"

#include <string>

vtkStandardNewMacro(vtkSIDataArraySelectionProperty);
//----------------------------------------------------------------------------
vtkSIDataArraySelectionProperty::vtkSIDataArraySelectionProperty()
{
  this->NumberOfElementsPerCommand = 2;
}

//----------------------------------------------------------------------------
vtkSIDataArraySelectionProperty::~vtkSIDataArraySelectionProperty()
{
}

//----------------------------------------------------------------------------
void vtkSIDataArraySelectionProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkSIDataArraySelectionProperty::GetSelection()
{
  if (!this->GetCommand())
  {
    return nullptr;
  }

  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke << this->GetVTKObject() << this->GetCommand()
      << vtkClientServerStream::End;
  this->ProcessMessage(str);

  vtkObjectBase* resultObj = nullptr;
  if (!this->GetLastResult().GetArgument(0, 0, &resultObj) ||
    vtkDataArraySelection::SafeDownCast(resultObj) == nullptr)
  {
    vtkErrorMacro("'" << this->GetCommand() << "' did not return a `vtkDataArraySelection`.");
    return nullptr;
  }

  return vtkDataArraySelection::SafeDownCast(resultObj);
}

//----------------------------------------------------------------------------
bool vtkSIDataArraySelectionProperty::ReadXMLAttributes(
  vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
  {
    return false;
  }

  int number_of_elements_per_command = 0;
  if (element->GetScalarAttribute(
        "number_of_elements_per_command", &number_of_elements_per_command))
  {
    if (number_of_elements_per_command != 1 && number_of_elements_per_command != 2)
    {
      vtkErrorMacro("`number_of_elements_per_command` must be 1 or 2, if specified");
      return false;
    }
    this->NumberOfElementsPerCommand = number_of_elements_per_command;
  }
  else
  {
    this->NumberOfElementsPerCommand = 2;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSIDataArraySelectionProperty::Push(vtkSMMessage* message, int offset)
{
  if (this->InformationOnly)
  {
    return this->Superclass::Pull(message);
  }

  if (vtkDataArraySelection* selection = this->GetSelection())
  {
    const ProxyState_Property* prop = &message->GetExtension(ProxyState::property, offset);
    assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

    const Variant& variant = prop->value();
    const int num_elems = variant.txt_size();

    if (this->NumberOfElementsPerCommand == 2)
    {
      for (int cc = 0; (cc + 1) < num_elems; cc += 2)
      {
        const auto name = variant.txt(cc);
        const int status = variant.txt(cc + 1) == "0" ? 0 : 1;
        selection->SetArraySetting(name.c_str(), status);
      }
    }
    else if (this->NumberOfElementsPerCommand == 1)
    {
      selection->RemoveAllArrays();
      for (int cc = 0; cc < num_elems; ++cc)
      {
        selection->EnableArray(variant.txt(cc).c_str());
      }
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSIDataArraySelectionProperty::Pull(vtkSMMessage* message)
{
  if (!this->InformationOnly)
  {
    return this->Superclass::Pull(message);
  }

  if (vtkDataArraySelection* selection = this->GetSelection())
  {
    ProxyState_Property* prop = message->AddExtension(ProxyState::property);
    prop->set_name(this->GetXMLName());
    Variant* result = prop->mutable_value();
    result->set_type(Variant::STRING);
    for (int cc = 0, max = selection->GetNumberOfArrays(); cc < max; ++cc)
    {
      result->add_txt(selection->GetArrayName(cc));
      result->add_txt((selection->GetArraySetting(cc) != 0 ? "1" : "0"));
    }
    return true;
  }
  return false;
}
