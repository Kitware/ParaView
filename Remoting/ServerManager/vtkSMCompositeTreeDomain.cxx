/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeTreeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeTreeDomain.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMCompositeTreeDomain);
vtkCxxSetObjectMacro(vtkSMCompositeTreeDomain, Information, vtkPVDataInformation);
//----------------------------------------------------------------------------
vtkSMCompositeTreeDomain::vtkSMCompositeTreeDomain()
  : Information(nullptr)
  , Mode(vtkSMCompositeTreeDomain::ALL)
  , DefaultMode(vtkSMCompositeTreeDomain::DEFAULT)
  , DataInformationTimeStamp(0)
{
}

//----------------------------------------------------------------------------
vtkSMCompositeTreeDomain::~vtkSMCompositeTreeDomain()
{
  this->SetInformation(nullptr);
}

//---------------------------------------------------------------------------
vtkDataAssembly* vtkSMCompositeTreeDomain::GetHierarchy() const
{
  return this->Information ? this->Information->GetHierarchy() : nullptr;
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::Update(vtkSMProperty*)
{
  this->SetInformation(this->GetInputDataInformation("Input"));

  const auto stamp = this->Information ? this->Information->GetMTime() : 0;
  if (this->DataInformationTimeStamp != stamp)
  {
    this->DataInformationTimeStamp = stamp;
    this->DomainModified();
  }
}

//---------------------------------------------------------------------------
int vtkSMCompositeTreeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  this->Mode = ALL;
  const char* mode = element->GetAttribute("mode");
  if (mode)
  {
    if (strcmp(mode, "all") == 0)
    {
      this->Mode = ALL;
    }
    else if (strcmp(mode, "leaves") == 0)
    {
      this->Mode = LEAVES;
    }
    else if (strcmp(mode, "amr") == 0)
    {
      this->Mode = AMR;
    }
    else if (strcmp(mode, "non-leaves") == 0)
    {
      vtkWarningMacro("Obsolete 'non-leaves' mode detected. Using 'all' instead.");
      this->Mode = ALL;
    }
    else if (strcmp(mode, "none") == 0)
    {
      // not sure why this mode was ever added or what it stood for <|:0).
      vtkWarningMacro("Obsolete 'none' mode detected. Using 'all' instead.");
      this->Mode = ALL;
    }
    else
    {
      vtkErrorMacro("Unrecognized mode: " << mode);
      return 0;
    }
  }

  if (const char* default_mode = element->GetAttribute("default_mode"))
  {
    if (strcmp(default_mode, "nonempty-leaf") == 0)
    {
      this->DefaultMode = NONEMPTY_LEAF;
    }
    else
    {
      vtkErrorMacro("Unrecognized 'default_mode': " << mode);
      return 0;
    }
  }

  if (vtkPVXMLElement* hints = prop->GetHints())
  {
    vtkPVXMLElement* useFlatIndex = hints->FindNestedElementByName("UseFlatIndex");
    if (useFlatIndex && useFlatIndex->GetAttribute("value") &&
      strcmp(useFlatIndex->GetAttribute("value"), "0") == 0)
    {
      this->Mode = AMR;
      vtkWarningMacro("'UseFlatIndex' index hint is deprecated. You may simply want "
                      "to set the 'mode' for the domain to 'amr' in the XML configuration.");
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMCompositeTreeDomain::SetDefaultValues(vtkSMProperty* property, bool use_unchecked_values)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
  vtkSMPropertyHelper helper(property);
  helper.SetUseUnchecked(use_unchecked_values);
  if (ivp && this->Information)
  {
    if (this->Mode == LEAVES || this->DefaultMode == NONEMPTY_LEAF)
    {
      int index = static_cast<int>(this->Information->GetFirstLeafCompositeIndex());
      if (index != 0)
      {
        const bool repeatable = (ivp->GetRepeatCommand() == 1);
        const int num_elements_per_command = ivp->GetNumberOfElementsPerCommand();
        const int num_elements = ivp->GetNumberOfElements();

        // Ensure that we don't set incorrect number of elements as the default
        // for any property.
        if ((repeatable && num_elements_per_command == 1) || (!repeatable && num_elements == 1))
        {
          helper.Set(0, index);
          return 1;
        }
      }
    }
  }
  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Information: " << this->Information << endl;
  os << indent << "Mode: ";
  switch (this->Mode)
  {
    case ALL:
      os << "ALL";
      break;
    case LEAVES:
      os << "LEAVES";
      break;
    case NON_LEAVES:
      os << "NON_LEAVES";
      break;
    case AMR:
      os << "AMR";
      break;
    default:
      os << "UNKNOWN";
  }
  os << endl;
  os << indent << "DefaultMode: ";
  switch (this->DefaultMode)
  {
    case DEFAULT:
      os << "DEFAULT";
      break;
    case NONEMPTY_LEAF:
      os << "NONEMPTY_LEAF";
      break;
    default:
      os << "UNKNOWN";
  }
  os << endl;
}
