/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMInstantiator.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

#include "vtkSMPropertyInternals.h"

vtkStandardNewMacro(vtkSMProperty);
vtkCxxRevisionMacro(vtkSMProperty, "1.5");

//---------------------------------------------------------------------------
vtkSMProperty::vtkSMProperty()
{
  this->Command = 0;
  this->ImmediateUpdate = 0;
  this->UpdateSelf = 0;
  this->PInternals = new vtkSMPropertyInternals;
  this->XMLName = 0;
}

//---------------------------------------------------------------------------
vtkSMProperty::~vtkSMProperty()
{
  this->SetCommand(0);
  delete this->PInternals;
  this->SetXMLName(0);
}

//---------------------------------------------------------------------------
void vtkSMProperty::AddDomain(vtkSMDomain* domain)
{
  this->PInternals->Domains.push_back(domain);
}

//---------------------------------------------------------------------------
vtkSMDomain* vtkSMProperty::GetDomain(unsigned int idx)
{
  return this->PInternals->Domains[idx];
}

//---------------------------------------------------------------------------
unsigned int vtkSMProperty::GetNumberOfDomains()
{
  return this->PInternals->Domains.size();
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProperty::GetSubProperty(const char* name)
{
  vtkSMPropertyInternals::PropertyMap::iterator it =
    this->PInternals->SubProperties.find(name);

  if (it == this->PInternals->SubProperties.end())
    {
    return 0;
    }

  return it->second.GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMProperty::AddSubProperty(const char* name, vtkSMProperty* proxy)
{
  // Check if the proxy already exists. If it does, we will
  // replace it
  vtkSMPropertyInternals::PropertyMap::iterator it =
    this->PInternals->SubProperties.find(name);

  if (it != this->PInternals->SubProperties.end())
    {
    vtkWarningMacro("Property " << name  << " already exists. Replacing");
    }

  this->PInternals->SubProperties[name] = proxy;
}

//---------------------------------------------------------------------------
void vtkSMProperty::RemoveSubProperty(const char* name)
{
  vtkSMPropertyInternals::PropertyMap::iterator it =
    this->PInternals->SubProperties.find(name);

  if (it != this->PInternals->SubProperties.end())
    {
    this->PInternals->SubProperties.erase(it);
    }
}

//---------------------------------------------------------------------------
void vtkSMProperty::AppendCommandToStream(
    vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command)
    {
    return;
    }

  *str << vtkClientServerStream::Invoke 
       << objectId << this->Command
       << vtkClientServerStream::End;
}

//---------------------------------------------------------------------------
int vtkSMProperty::ReadXMLAttributes(vtkPVXMLElement* element)
{
  const char* xmlname = element->GetAttribute("name");
  if(xmlname) 
    { 
    this->SetXMLName(xmlname); 
    }

  const char* command = element->GetAttribute("command");
  if(command) 
    { 
    this->SetCommand(command); 
    }

  int immediate_update;
  int retVal = element->GetScalarAttribute("immediate_update", &immediate_update);
  if(retVal) 
    { 
    this->SetImmediateUpdate(immediate_update); 
    }

  int update_self;
  retVal = element->GetScalarAttribute("update_self", &update_self);
  if(retVal) 
    { 
    this->SetUpdateSelf(update_self); 
    }

  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* domainEl = element->GetNestedElement(i);
    vtkObject* object = 0;
    ostrstream name;
    name << "vtkSM" << domainEl->GetName() << ends;
    object = vtkInstantiator::CreateInstance(name.str());
    delete[] name.str();
    vtkSMDomain* domain = vtkSMDomain::SafeDownCast(object);
    if (domain)
      {
      domain->ReadXMLAttributes(domainEl);
      this->AddDomain(domain);
      domain->Delete();
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Command: " 
     << (this->Command ? this->Command : "(null)") << endl;
  os << indent << "ImmediateUpdate:" << this->ImmediateUpdate << endl;
  os << indent << "UpdateSelf:" << this->UpdateSelf << endl;
}
