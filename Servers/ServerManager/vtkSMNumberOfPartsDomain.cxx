/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfPartsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNumberOfPartsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMNumberOfPartsDomain);
vtkCxxRevisionMacro(vtkSMNumberOfPartsDomain, "1.7");

//---------------------------------------------------------------------------
vtkSMNumberOfPartsDomain::vtkSMNumberOfPartsDomain()
{
  this->OutputPortMultiplicity = vtkSMNumberOfPartsDomain::SINGLE;
}

//---------------------------------------------------------------------------
vtkSMNumberOfPartsDomain::~vtkSMNumberOfPartsDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMNumberOfPartsDomain::IsInDomain(vtkSMProperty* property)
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
      vtkSMProxy* proxy = pp->GetUncheckedProxy(i);
      if (!this->IsInDomain( 
            vtkSMSourceProxy::SafeDownCast(proxy) ) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMNumberOfPartsDomain::IsInDomain(vtkSMSourceProxy* proxy)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!proxy)
    {
    return 0;
    }

  // Make sure the outputs are created.
  proxy->CreateOutputPorts();

  if (proxy->GetNumberOfOutputPorts() > 1 && 
      this->OutputPortMultiplicity == vtkSMNumberOfPartsDomain::MULTIPLE)
    {
    return 1;
    }

  if (proxy->GetNumberOfOutputPorts() == 1 && 
      this->OutputPortMultiplicity == vtkSMNumberOfPartsDomain::SINGLE)
    {
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMNumberOfPartsDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  vtkPVXMLElement* multiplicityElem = vtkPVXMLElement::New();
  multiplicityElem->SetName("Multiplicity");
  switch (this->OutputPortMultiplicity)
    {
    case vtkSMNumberOfPartsDomain::SINGLE:
      multiplicityElem->AddAttribute("value", "single");
      break;
    case vtkSMNumberOfPartsDomain::MULTIPLE:
      multiplicityElem->AddAttribute("value", "multiple");
      break;
    }
  domainElement->AddNestedElement(multiplicityElem);
  multiplicityElem->Delete();
}

//---------------------------------------------------------------------------
int vtkSMNumberOfPartsDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* multiplicity = element->GetAttribute("multiplicity");
  if (multiplicity)
    {
    if (strcmp(multiplicity, "single") == 0)
      {
      this->SetOutputPortMultiplicity(vtkSMNumberOfPartsDomain::SINGLE);
      }
    else if (strcmp(multiplicity, "multiple") == 0)
      {
      this->SetOutputPortMultiplicity(vtkSMNumberOfPartsDomain::MULTIPLE);
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
void vtkSMNumberOfPartsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputPortMultiplicity: " << this->OutputPortMultiplicity << endl;
}
