/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomainIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDomainIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyInternals.h"

vtkStandardNewMacro(vtkSMDomainIterator);

struct vtkSMDomainIteratorInternals
{
  vtkSMPropertyInternals::DomainMap::iterator DomainIterator;
};

//---------------------------------------------------------------------------
vtkSMDomainIterator::vtkSMDomainIterator()
{
  this->Property = nullptr;
  this->Internals = new vtkSMDomainIteratorInternals;
}

//---------------------------------------------------------------------------
vtkSMDomainIterator::~vtkSMDomainIterator()
{
  if (this->Property)
  {
    this->Property->Delete();
  }
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMDomainIterator::SetProperty(vtkSMProperty* proxy)
{
  if (this->Property != proxy)
  {
    if (this->Property != nullptr)
    {
      this->Property->UnRegister(this);
    }
    this->Property = proxy;
    if (this->Property != nullptr)
    {
      this->Property->Register(this);
      this->Begin();
    }
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMDomainIterator::Begin()
{
  if (!this->Property)
  {
    vtkErrorMacro("Property is not set. Can not perform operation: Begin()");
    return;
  }

  this->Internals->DomainIterator = this->Property->PInternals->Domains.begin();
}

//---------------------------------------------------------------------------
int vtkSMDomainIterator::IsAtEnd()
{
  if (!this->Property)
  {
    vtkErrorMacro("Property is not set. Can not perform operation: IsAtEnd()");
    return 1;
  }
  if (this->Internals->DomainIterator == this->Property->PInternals->Domains.end())
  {
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMDomainIterator::Next()
{
  if (!this->Property)
  {
    vtkErrorMacro("Property is not set. Can not perform operation: Next()");
    return;
  }

  if (this->Internals->DomainIterator != this->Property->PInternals->Domains.end())
  {
    this->Internals->DomainIterator++;
    return;
  }
}

//---------------------------------------------------------------------------
const char* vtkSMDomainIterator::GetKey()
{
  if (!this->Property)
  {
    vtkErrorMacro("Property is not set. Can not perform operation: GetKey()");
    return nullptr;
  }

  if (this->Internals->DomainIterator != this->Property->PInternals->Domains.end())
  {
    return this->Internals->DomainIterator->first.c_str();
  }

  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMDomain* vtkSMDomainIterator::GetDomain()
{
  if (!this->Property)
  {
    vtkErrorMacro("Property is not set. Can not perform operation: GetProperty()");
    return nullptr;
  }

  if (this->Internals->DomainIterator != this->Property->PInternals->Domains.end())
  {
    return this->Internals->DomainIterator->second.GetPointer();
  }

  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMDomainIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Property: " << this->Property << endl;
}
