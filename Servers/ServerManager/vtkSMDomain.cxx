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

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include "vtkStdString.h"

vtkCxxRevisionMacro(vtkSMDomain, "1.7");

struct vtkSMDomainInternals
{
  typedef 
  vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMProperty> > PropertyMap;
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
                req->AddDependent(this);
                this->Internals->RequiredProperties[function] = req;
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
void vtkSMDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XMLName: " << this->XMLName << endl;
}
