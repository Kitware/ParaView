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
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMNumberOfGroupsDomain);

//---------------------------------------------------------------------------
vtkSMNumberOfGroupsDomain::vtkSMNumberOfGroupsDomain()
{
  this->GroupMultiplicity = vtkSMNumberOfGroupsDomain::NOT_SET;
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

  if (this->GroupMultiplicity == vtkSMNumberOfGroupsDomain::NOT_SET)
    {
    return this->Superclass::IsInDomain(property);
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  if (pp)
    {
    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
    unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
    for (unsigned int i=0; i<numProxs; i++)
      {
      if (!this->IsInDomain( 
            vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i)),
            (ip? ip->GetUncheckedOutputPortForConnection(i):0)) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMNumberOfGroupsDomain::IsInDomain(vtkSMSourceProxy* proxy,
  int outputport /*=0*/)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!proxy)
    {
    return 0;
    }

  vtkPVDataInformation* di = proxy->GetDataInformation(outputport);
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

  if (!cdi->GetDataIsComposite())
    {
    // This domain isn't applicable if the data is not composite, so don't
    // even consider whether the number of groups is right.
    // This case happens when the filter accepts input that may or may not be
    // vtkMultiGroupDataSet.
    return 1;
    }

  // FIXME: THIS IS TOTALLY BOGUS DOMAIN
  if (cdi->GetNumberOfChildren() > 1 && 
      this->GroupMultiplicity == vtkSMNumberOfGroupsDomain::MULTIPLE)
    {
    return 1;
    }

  if (cdi->GetNumberOfChildren() == 1 && 
      this->GroupMultiplicity == vtkSMNumberOfGroupsDomain::SINGLE)
    {
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMNumberOfGroupsDomain::Update(vtkSMProperty*)
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  
  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    this->InvokeModified();
    }
}

//---------------------------------------------------------------------------
void vtkSMNumberOfGroupsDomain::Update(vtkSMProxyProperty *pp)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
    {
    vtkSMSourceProxy* sp =
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        (ip? ip->GetUncheckedOutputPortForConnection(i) : 0));
      if (!info)
        {
        continue;
        }
      vtkPVCompositeDataInformation* cInfo = 
        info->GetCompositeDataInformation();
      this->AddMinimum(0, 0);
      if (cInfo)
        {
        this->AddMaximum(0, cInfo->GetNumberOfChildren()-1);
        }
      else
        {
        this->AddMaximum(0, -1);
        }
      this->InvokeModified();
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = pp->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        (ip? ip->GetOutputPortForConnection(i): 0));
      if (!info)
        {
        continue;
        }
      vtkPVCompositeDataInformation* cInfo = 
        info->GetCompositeDataInformation();
      this->AddMinimum(0, 0);
      if (cInfo)
        {
        this->AddMaximum(0, cInfo->GetNumberOfChildren()-1);
        }
      else
        {
        this->AddMaximum(0, -1);
        }
      this->InvokeModified();
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMNumberOfGroupsDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  vtkPVXMLElement* multiplicityElem = vtkPVXMLElement::New();
  multiplicityElem->SetName("Multiplicity");
  switch (this->GroupMultiplicity)
    {
    case vtkSMNumberOfGroupsDomain::SINGLE:
      multiplicityElem->AddAttribute("value", "single");
      break;
    case vtkSMNumberOfGroupsDomain::MULTIPLE:
      multiplicityElem->AddAttribute("value", "multiple");
      break;
    }
  domainElement->AddNestedElement(multiplicityElem);
  multiplicityElem->Delete();
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
