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
#include "vtkSMInformationHelper.h"
#include "vtkSMInstantiator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSubPropertyIterator.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"
#include <vtkstd/vector>

#include "vtkSMPropertyInternals.h"

vtkStandardNewMacro(vtkSMProperty);
vtkCxxRevisionMacro(vtkSMProperty, "1.27");

vtkCxxSetObjectMacro(vtkSMProperty, Proxy, vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMProperty, InformationHelper, vtkSMInformationHelper);
vtkCxxSetObjectMacro(vtkSMProperty, InformationProperty, vtkSMProperty);
vtkCxxSetObjectMacro(vtkSMProperty, ControllerProperty, vtkSMProperty);

int vtkSMProperty::CheckDomains = 1;
int vtkSMProperty::ModifiedAtCreation = 1;

//---------------------------------------------------------------------------
vtkSMProperty::vtkSMProperty()
{
  this->Command = 0;
  this->ImmediateUpdate = 0;
  this->Animateable = 0;
  this->UpdateSelf = 0;
  this->PInternals = new vtkSMPropertyInternals;
  this->XMLName = 0;
  this->DomainIterator = vtkSMDomainIterator::New();
  this->DomainIterator->SetProperty(this);
  this->Proxy = 0;
  this->InformationOnly = 0;
  this->InformationHelper = 0;
  this->InformationProperty = 0;
  this->ControllerProxy = 0;
  this->ControllerProperty = 0;
}

//---------------------------------------------------------------------------
vtkSMProperty::~vtkSMProperty()
{
  this->SetCommand(0);
  delete this->PInternals;
  this->SetXMLName(0);
  this->DomainIterator->Delete();
  this->SetProxy(0);
  this->SetInformationHelper(0);
  this->SetInformationProperty(0);
  this->SetControllerProperty(0);
  this->SetControllerProxy(0);
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
void vtkSMProperty::SetControllerProxy(vtkSMProxy* proxy)
{
  this->ControllerProxy = proxy;
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
unsigned int vtkSMProperty::GetNumberOfDomains()
{
  return this->PInternals->Domains.size();
}

//---------------------------------------------------------------------------
vtkSMDomainIterator* vtkSMProperty::NewDomainIterator()
{
  vtkSMDomainIterator* iter = vtkSMDomainIterator::New();
  iter->SetProperty(this);
  return iter;
}

//---------------------------------------------------------------------------
void vtkSMProperty::AddDependent(vtkSMDomain* dom)
{
  this->PInternals->Dependents.push_back(dom);
}

//---------------------------------------------------------------------------
void vtkSMProperty::RemoveAllDependents()
{
  vtkSMPropertyInternals::DependentsVector::iterator iter =
    this->PInternals->Dependents.begin();
  for (; iter != this->PInternals->Dependents.end(); iter++)
    {
    iter->GetPointer()->RemoveRequiredProperty(this);
    }
  this->PInternals->Dependents.erase(
    this->PInternals->Dependents.begin(), this->PInternals->Dependents.end());
}

//---------------------------------------------------------------------------
void vtkSMProperty::UpdateDependentDomains()
{
  // Update own domains
  this->DomainIterator->Begin();
  while(!this->DomainIterator->IsAtEnd())
    {
    this->DomainIterator->GetDomain()->Update(0);
    this->DomainIterator->Next();
    }

  // Update other dependent domains
  vtkSMPropertyInternals::DependentsVector::iterator iter =
    this->PInternals->Dependents.begin();
  for (; iter != this->PInternals->Dependents.end(); iter++)
    {
    iter->GetPointer()->Update(this);
    }
}

//---------------------------------------------------------------------------
void vtkSMProperty::UpdateInformation(int serverIds, vtkClientServerID objectId)
{
  if (!this->InformationOnly)
    {
    return;
    }

  if (this->InformationHelper)
    {
    this->InformationHelper->UpdateProperty(serverIds, objectId, this);
    }
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
void vtkSMProperty::AddSubProperty(const char* name, vtkSMProperty* property)
{
  // Check if the proxy already exists. If it does, we will
  // replace it
  vtkSMPropertyInternals::PropertyMap::iterator it =
    this->PInternals->SubProperties.find(name);

  if (it != this->PInternals->SubProperties.end())
    {
    vtkWarningMacro("Property " << name  << " already exists. Replacing");
    }

  this->PInternals->SubProperties[name] = property;
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
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  *str << vtkClientServerStream::Invoke 
       << objectId << this->Command
       << vtkClientServerStream::End;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProperty::NewProperty(const char* name)
{
  if (!this->Proxy)
    {
    return 0;
    }
  return this->Proxy->NewProperty(name);
}

//---------------------------------------------------------------------------
int vtkSMProperty::ReadXMLAttributes(vtkSMProxy* proxy,
                                     vtkPVXMLElement* element)
{
  // Set during xml parsing only. Used in NewProperty() which is
  // called by domains to get required properties.
  this->SetProxy(proxy);

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

  const char* information_property = 
    element->GetAttribute("information_property");
  if(information_property) 
    { 
    this->SetInformationProperty(this->NewProperty(information_property));
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

  int information_only;
  retVal = element->GetScalarAttribute("information_only", &information_only);
  if(retVal) 
    { 
    this->SetInformationOnly(information_only); 
    }

  int animateable;
  retVal = element->GetScalarAttribute("animateable", &animateable);
  if (retVal)
    {
    this->SetAnimateable(animateable);
    }

  // Read and create domains.
  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* domainEl = element->GetNestedElement(i);
    vtkObject* object = 0;
    ostrstream name;
    name << "vtkSM" << domainEl->GetName() << ends;
    object = vtkInstantiator::CreateInstance(name.str());
    if (object)
      {
      vtkSMDomain* domain = vtkSMDomain::SafeDownCast(object);
      vtkSMInformationHelper* ih = vtkSMInformationHelper::SafeDownCast(object);
      if (domain)
        {
        if (domain->ReadXMLAttributes(this, domainEl))
          {
          const char* dname = domainEl->GetAttribute("name");
          if (dname)
            {
            domain->SetXMLName(dname);
            this->AddDomain(dname, domain);
            }
          }
        }
      else if (ih)
        {
        if (ih->ReadXMLAttributes(this, domainEl))
          {
          this->SetInformationHelper(ih);
          }
        }
      else
        {
        vtkErrorMacro("Object created (type: " << name.str()
                      << ") is not of a recognized type.");
        }
      object->Delete();
      }
    else
      {
      vtkErrorMacro("Could not create object of type: " << name.str()
                    << ". Did you specify wrong xml element?");
      }
    delete[] name.str();
    }

  this->SetProxy(0);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProperty::SaveState(const char* name, ostream* file, vtkIndent indent)
{
  if (this->ControllerProxy && this->ControllerProperty)
    {
    vtkSMProxyManager *pxm = this->GetProxyManager();
    //NOTE: this operation is slow :(.
    const char* cname = pxm->GetProxyName(this->ControllerProxy->GetXMLGroup(),this->ControllerProxy);
    if (cname)
      {
    *file << "    <ControllerProperty name=\""
      << cname << "." << this->ControllerProperty->GetXMLName() 
      << "\" />" << endl;
      }
    }
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
void vtkSMProperty::SetCheckDomains(int check)
{
  vtkSMProperty::CheckDomains = check;
}

//---------------------------------------------------------------------------
int vtkSMProperty::GetCheckDomains()
{
  return vtkSMProperty::CheckDomains;
}

//---------------------------------------------------------------------------
void vtkSMProperty::SetModifiedAtCreation(int check)
{
  vtkSMProperty::ModifiedAtCreation = check;
}

//---------------------------------------------------------------------------
int vtkSMProperty::GetModifiedAtCreation()
{
  return vtkSMProperty::ModifiedAtCreation;
}

//---------------------------------------------------------------------------
void vtkSMProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Command: " 
     << (this->Command ? this->Command : "(null)") << endl;
  os << indent << "ImmediateUpdate:" << this->ImmediateUpdate << endl;
  os << indent << "UpdateSelf:" << this->UpdateSelf << endl;
  os << indent << "InformationOnly:" << this->InformationOnly << endl;
  os << indent << "XMLName:" 
     <<  (this->XMLName ? this->XMLName : "(null)") << endl;
  os << indent << "InformationProperty: " << this->InformationProperty << endl;
  os << indent << "Animateable: " << this->Animateable << endl;

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
