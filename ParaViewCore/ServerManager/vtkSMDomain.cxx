/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDomain.h"

#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h"
#include "vtkSMSession.h"

#include <map>
#include "vtkStdString.h"
#include <assert.h>

struct vtkSMDomainInternals
{
  // This used to be a vtkSmartPointer. Converting this to vtkWeakPointer.
  // There's no reason why a domain should have a hard reference to the required
  // property since both the domain and the required property belong to the same
  // proxy, so they will be deleted only when the proxy disappears.
  typedef 
  std::map<vtkStdString, vtkWeakPointer<vtkSMProperty> > PropertyMap;
  PropertyMap RequiredProperties;
};

//---------------------------------------------------------------------------
vtkSMDomain::vtkSMDomain()
{
  this->XMLName = 0;
  this->Internals = new vtkSMDomainInternals;
  this->IsOptional = 0;
}

//---------------------------------------------------------------------------
vtkSMDomain::~vtkSMDomain()
{
  this->SetXMLName(0);
  delete this->Internals;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMDomain::GetRequiredProperty(const char* function)
{
  vtkSMDomainInternals::PropertyMap::iterator iter =
    this->Internals->RequiredProperties.find(function);
  if (iter != this->Internals->RequiredProperties.end())
    {
    return iter->second.GetPointer();
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMDomain::RemoveRequiredProperty(vtkSMProperty* prop)
{
  vtkSMDomainInternals::PropertyMap::iterator iter = 
    this->Internals->RequiredProperties.begin();

  for(; iter != this->Internals->RequiredProperties.end(); iter++)
    {
    if ( iter->second.GetPointer() == prop )
      {
      this->Internals->RequiredProperties.erase(iter);
      break;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  assert("Property and proxy should be properly set" && prop && prop->GetParent());
  this->SetSession(prop->GetParent()->GetSession());

  int isOptional;
  int retVal = element->GetScalarAttribute("optional", &isOptional);
  if(retVal) 
    { 
    this->SetIsOptional(isOptional); 
    }

  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* domainEl = element->GetNestedElement(i);
    if ( strcmp(domainEl->GetName(), "RequiredProperties" ) == 0 )
      {
      for(unsigned int j=0; j < domainEl->GetNumberOfNestedElements(); ++j)
        {
        vtkPVXMLElement* reqEl = domainEl->GetNestedElement(j);
        const char* name = reqEl->GetAttribute("name");
        if (name)
          {
          if ( prop->GetXMLName() && strcmp(name, prop->GetXMLName()) == 0 )
            {
            vtkErrorMacro("A domain can not depend on it's property");
            }
          else
            {
            const char* function = reqEl->GetAttribute("function");
            if (!function)
              {
              vtkErrorMacro("Missing required attribute: function");
              }
            else
              {
              vtkSMProperty* req = prop->NewProperty(name);
              if (req)
                {
                this->AddRequiredProperty(req, function);
                }
              else
                {
                vtkWarningMacro(
                  "You have added a domain dependency to a property named '"
                  << name << "' which does not exist.");
                }
              }
            }
          }
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDomain::AddRequiredProperty(vtkSMProperty *prop,
                                      const char *function)
{
  if (!prop)
    {
    return;
    }
  
  if (!function)
    {
    vtkErrorMacro("Missing name of function for new required property.");
    return;
    }
  
  prop->AddDependent(this);
  this->Internals->RequiredProperties[function] = prop;
}

//---------------------------------------------------------------------------
unsigned int vtkSMDomain::GetNumberOfRequiredProperties()
{
  return this->Internals->RequiredProperties.size();
}

//---------------------------------------------------------------------------
void vtkSMDomain::InvokeModified()
{
  this->InvokeEvent(vtkCommand::DomainModifiedEvent, 0);
}

//---------------------------------------------------------------------------
void vtkSMDomain::ChildSaveState(vtkPVXMLElement* /*domainElement*/)
{
}

//---------------------------------------------------------------------------
void vtkSMDomain::SaveState(vtkPVXMLElement* parent, const char* uid)
{
  vtkPVXMLElement* domainElement = vtkPVXMLElement::New();
  domainElement->SetName("Domain");
  domainElement->AddAttribute("name", this->XMLName);
  domainElement->AddAttribute("id", uid);

  this->ChildSaveState(domainElement);

  parent->AddNestedElement(domainElement);
  domainElement->Delete();
}

//---------------------------------------------------------------------------
void vtkSMDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XMLName: " << (this->XMLName ? this->XMLName : "(null)") 
     << endl;
  os << indent << "IsOptional: " << this->IsOptional << endl;
}
