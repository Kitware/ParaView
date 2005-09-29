/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfGroupsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNumberOfGroupsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMNumberOfGroupsDomain);
vtkCxxRevisionMacro(vtkSMNumberOfGroupsDomain, "1.1");

//---------------------------------------------------------------------------
vtkSMNumberOfGroupsDomain::vtkSMNumberOfGroupsDomain()
{
  this->GroupMultiplicity = vtkSMNumberOfGroupsDomain::SINGLE;
}

//---------------------------------------------------------------------------
vtkSMNumberOfGroupsDomain::~vtkSMNumberOfGroupsDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMNumberOfGroupsDomain::IsInDomain(vtkSMProperty* property)
{
  if (!property)
    {
    return 0;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  if (pp)
    {
    unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
    for (unsigned int i=0; i<numProxs; i++)
      {
      if (!this->IsInDomain( 
            vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i)) ) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMNumberOfGroupsDomain::IsInDomain(vtkSMSourceProxy* proxy)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!proxy)
    {
    return 0;
    }

  vtkPVDataInformation* di = proxy->GetDataInformation();
  if (!di)
    {
    vtkErrorMacro("Input does not have associated data information. "
                  "Cannot verify domain.");
    return 0;
    }
  vtkPVCompositeDataInformation* cdi = di->GetCompositeDataInformation();
  if (!cdi)
    {
    vtkErrorMacro("Input does not have associated composite data "
                  "information. Cannot verify domain.");
    return 0;
    }

  if (cdi->GetNumberOfGroups() > 1 && 
      this->GroupMultiplicity == vtkSMNumberOfGroupsDomain::MULTIPLE)
    {
    return 1;
    }

  if (cdi->GetNumberOfGroups() == 1 && 
      this->GroupMultiplicity == vtkSMNumberOfGroupsDomain::SINGLE)
    {
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMNumberOfGroupsDomain::SaveState(
  const char* name, ostream* file, vtkIndent indent)
{
  *file << indent 
        << "<Domain name=\"" << this->XMLName << "\" id=\"" << name << "\">"
        << endl;
  *file << indent.GetNextIndent() 
        << "<Multiplicity value=\"";
  switch (this->GroupMultiplicity)
    {
    case vtkSMNumberOfGroupsDomain::SINGLE:
      *file << "single";
      break;
    case vtkSMNumberOfGroupsDomain::MULTIPLE:
      *file << "multiple";
      break;
    }
  *file << "\"/>" << endl;
  *file << indent
        << "</Domain>" << endl;
}

//---------------------------------------------------------------------------
int vtkSMNumberOfGroupsDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* multiplicity = element->GetAttribute("multiplicity");
  if (multiplicity)
    {
    if (strcmp(multiplicity, "single") == 0)
      {
      this->SetGroupMultiplicity(vtkSMNumberOfGroupsDomain::SINGLE);
      }
    else if (strcmp(multiplicity, "multiple") == 0)
      {
      this->SetGroupMultiplicity(vtkSMNumberOfGroupsDomain::MULTIPLE);
      }
    else
      {
      vtkErrorMacro("Unrecognized multiplicity.");
      return 0;
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMNumberOfGroupsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "GroupMultiplicity: " << this->GroupMultiplicity << endl;
}
