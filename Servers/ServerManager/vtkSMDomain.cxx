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

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSMDomain, "1.3");

struct vtkSMDomainInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMProperty> > RequiredProperties;
};

//---------------------------------------------------------------------------
vtkSMDomain::vtkSMDomain()
{
  this->XMLName = 0;
  this->Internals = new vtkSMDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMDomain::~vtkSMDomain()
{
  this->SetXMLName(0);
  delete this->Internals;
}

//---------------------------------------------------------------------------
unsigned int vtkSMDomain::GetNumberOfRequiredProperties()
{
  return this->Internals->RequiredProperties.size();
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMDomain::GetRequiredProperty(unsigned int idx)
{
  return this->Internals->RequiredProperties[idx];
}

//---------------------------------------------------------------------------
int vtkSMDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
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
            vtkSMProperty* req = prop->NewProperty(name);
            if (req)
              {
              req->AddDependant(this);
              this->Internals->RequiredProperties.push_back(req);
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
}
