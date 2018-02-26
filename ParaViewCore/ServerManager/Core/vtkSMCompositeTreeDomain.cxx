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
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
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
{
  this->Information = 0;
  this->LastInformation = 0;
  this->Mode = ALL;
  this->DefaultMode = DEFAULT;
  this->Source = 0;
  this->SourcePort = 0;
}

//----------------------------------------------------------------------------
vtkSMCompositeTreeDomain::~vtkSMCompositeTreeDomain()
{
  this->Source = 0;
  this->SourcePort = 0;
  this->SetInformation(0);
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::Update(vtkSMProperty*)
{
  this->Source = 0;
  this->SourcePort = 0;
  this->SetInformation(0);

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(this->GetRequiredProperty("Input"));
  if (pp)
  {
    this->Update(pp);
  }
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::InvokeModifiedIfChanged()
{
  if (this->Information != this->LastInformation ||
    (this->Information && this->UpdateTime < this->Information->GetMTime()))
  {
    this->LastInformation = this->Information;
    this->UpdateTime.Modified();
    this->DomainModified();
  }
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMCompositeTreeDomain::GetSource()
{
  return this->Source.GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::Update(vtkSMInputProperty* ip)
{
  unsigned int i;
  unsigned int numProxs = ip->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(ip->GetUncheckedProxy(i));
    if (sp)
    {
      vtkPVDataInformation* info =
        sp->GetDataInformation(ip->GetUncheckedOutputPortForConnection(i));
      if (!info)
      {
        continue;
      }
      this->Source = sp;
      this->SourcePort = ip->GetUncheckedOutputPortForConnection(i);
      this->SetInformation(info);
      this->InvokeModifiedIfChanged();
      return;
    }
  }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = ip->GetNumberOfProxies();
  for (i = 0; i < numProxs; i++)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(i));
    if (sp)
    {
      vtkPVDataInformation* info = sp->GetDataInformation(ip->GetOutputPortForConnection(i));
      if (!info)
      {
        continue;
      }
      this->Source = sp;
      this->SourcePort = ip->GetOutputPortForConnection(i);
      this->SetInformation(info);
      this->InvokeModifiedIfChanged();
      return;
    }
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
      vtkNew<vtkPVCompositeDataInformationIterator> iter;
      iter->SetDataInformation(this->Information);
      int index = -1;
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        index++;
        if (vtkPVDataInformation* info = iter->GetCurrentDataInformation())
        {
          vtkPVCompositeDataInformation* cinfo = info->GetCompositeDataInformation();
          if (!cinfo->GetDataIsComposite() || cinfo->GetDataIsMultiPiece())
          {
            break;
          }
        }
      }
      if (index != -1)
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
  os << indent << "SourcePort: " << this->SourcePort << endl;
}
