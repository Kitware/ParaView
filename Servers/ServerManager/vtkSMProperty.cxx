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
#include "vtkSMDomainIterator.h"
#include "vtkSMInstantiator.h"
#include "vtkSMSubPropertyIterator.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

#include "vtkSMPropertyInternals.h"

vtkStandardNewMacro(vtkSMProperty);
vtkCxxRevisionMacro(vtkSMProperty, "1.10");

//---------------------------------------------------------------------------
vtkSMProperty::vtkSMProperty()
{
  this->Command = 0;
  this->ImmediateUpdate = 0;
  this->UpdateSelf = 0;
  this->PInternals = new vtkSMPropertyInternals;
  this->XMLName = 0;
  this->IsReadOnly = 0;
  this->DomainIterator = vtkSMDomainIterator::New();
  this->DomainIterator->SetProperty(this);
}

//---------------------------------------------------------------------------
vtkSMProperty::~vtkSMProperty()
{
  this->SetCommand(0);
  delete this->PInternals;
  this->SetXMLName(0);
  this->DomainIterator->Delete();
}

//-----------------------------------------------------------------------------
// UnRegister is overloaded because the object has a reference to itself
// through the domain iterator.
void vtkSMProperty::UnRegister(vtkObjectBase* obj)
{
  if (this->ReferenceCount == 2)
    {
    this->Superclass::UnRegister(obj);

    vtkSMDomainIterator *tmp = this->DomainIterator;
    tmp->Register(0);
    tmp->SetProperty(0);
    tmp->UnRegister(0);
    return;
    }
  this->Superclass::UnRegister(obj);

}

//---------------------------------------------------------------------------
int vtkSMProperty::IsInDomains()
{
  this->DomainIterator->Begin();
  while(!this->DomainIterator->IsAtEnd())
    {
    if (!this->DomainIterator->GetDomain()->IsInDomain(this))
      {
      return 0;
      }
    this->DomainIterator->Next();
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProperty::AddDomain(const char* name, vtkSMDomain* domain)
{
  // Check if the proxy already exists. If it does, we will
  // replace it
  vtkSMPropertyInternals::DomainMap::iterator it =
    this->PInternals->Domains.find(name);

  if (it != this->PInternals->Domains.end())
    {
    vtkWarningMacro("Domain " << name  << " already exists. Replacing");
    }

  this->PInternals->Domains[name] = domain;
}

//---------------------------------------------------------------------------
vtkSMDomain* vtkSMProperty::GetDomain(const char* name)
{
  vtkSMPropertyInternals::DomainMap::iterator it =
    this->PInternals->Domains.find(name);

  if (it == this->PInternals->Domains.end())
    {
    return 0;
    }

  return it->second.GetPointer();
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
  vtkSMProxy*, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->IsReadOnly)
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
      if (domain->ReadXMLAttributes(domainEl))
        {
        const char* dname = domainEl->GetAttribute("name");
        if (dname)
          {
          domain->SetXMLName(dname);
          this->AddDomain(dname, domain);
          }
        }
      domain->Delete();
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProperty::SaveState(const char* name, ofstream* file, vtkIndent indent)
{
  this->DomainIterator->Begin();
  while(!this->DomainIterator->IsAtEnd())
    {
    ostrstream dname;
    dname << name << "." << this->DomainIterator->GetKey() << ends;
    this->DomainIterator->GetDomain()->SaveState(
      dname.str(), file, indent.GetNextIndent());
    delete[] dname.str();
    this->DomainIterator->Next();
    }
}

//---------------------------------------------------------------------------
void vtkSMProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Command: " 
     << (this->Command ? this->Command : "(null)") << endl;
  os << indent << "ImmediateUpdate:" << this->ImmediateUpdate << endl;
  os << indent << "UpdateSelf:" << this->UpdateSelf << endl;
  os << indent << "IsReadOnly:" << this->IsReadOnly << endl;

  vtkSMSubPropertyIterator* iter = vtkSMSubPropertyIterator::New();
  iter->SetProperty(this);
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMProperty* property = iter->GetSubProperty();
    if (property)
      {
      os << indent << "Sub-property " << iter->GetKey() << ": " << endl;
      property->PrintSelf(os, indent.GetNextIndent());
      }
    iter->Next();
    }
  iter->Delete();
}
