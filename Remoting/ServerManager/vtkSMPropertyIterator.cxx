// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPropertyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyInternals.h"

vtkStandardNewMacro(vtkSMPropertyIterator);

struct vtkSMPropertyIteratorInternals
{
  vtkSMProxyInternals::PropertyInfoMap::iterator PropertyIterator;
  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator ExposedPropertyIterator;
};

//---------------------------------------------------------------------------
vtkSMPropertyIterator::vtkSMPropertyIterator()
{
  this->Proxy = nullptr;
  this->Internals = new vtkSMPropertyIteratorInternals;
  this->TraverseSubProxies = 1;
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator::~vtkSMPropertyIterator()
{
  this->SetProxy(nullptr);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::SetProxy(vtkSMProxy* proxy)
{
  if (this->Proxy != proxy)
  {
    if (this->Proxy != nullptr)
    {
      this->Proxy->UnRegister(this);
    }
    this->Proxy = proxy;
    if (this->Proxy != nullptr)
    {
      this->Proxy->Register(this);
      this->Begin();
    }
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::Begin()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: Begin()");
    return;
  }

  this->Internals->PropertyIterator = this->Proxy->Internals->Properties.begin();

  this->Internals->ExposedPropertyIterator = this->Proxy->Internals->ExposedProperties.begin();
}

//---------------------------------------------------------------------------
int vtkSMPropertyIterator::IsAtEnd()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: IsAtEnd()");
    return 1;
  }
  if (this->TraverseSubProxies)
  {
    if (this->Internals->PropertyIterator == this->Proxy->Internals->Properties.end() &&
      this->Internals->ExposedPropertyIterator == this->Proxy->Internals->ExposedProperties.end())
    {
      return 1;
    }
  }
  else
  {
    if (this->Internals->PropertyIterator == this->Proxy->Internals->Properties.end())
    {
      return 1;
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::Next()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: Next()");
    return;
  }

  // If we are still in the root proxy, move to the next element.
  if (this->Internals->PropertyIterator != this->Proxy->Internals->Properties.end())
  {
    this->Internals->PropertyIterator++;
    return;
    // Consider the end case when the this->Internals->PropertyIterator
    // is pointing to the last element before the above iterator increment.
    // Then the iterator is now pointing off the last element (after the
    // this->Internals->PropertyIterator++). But that's still okay,
    // since this->Internals->ExposedPropertyIterator is already
    // initialized to point to the first exposed property
    // and the hence the next GetKey()/GetProperty() call
    // will correctly return the first exposed property.
  }

  if (!this->TraverseSubProxies)
  {
    return;
  }

  if (this->Internals->ExposedPropertyIterator != this->Proxy->Internals->ExposedProperties.end())
  {
    this->Internals->ExposedPropertyIterator++;
  }
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyIterator::GetKey()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetKey()");
    return nullptr;
  }

  if (this->Internals->PropertyIterator != this->Proxy->Internals->Properties.end())
  {
    return this->Internals->PropertyIterator->first.c_str();
  }

  if (this->TraverseSubProxies)
  {
    if (this->Internals->ExposedPropertyIterator != this->Proxy->Internals->ExposedProperties.end())
    {
      // return the exposed name.
      return this->Internals->ExposedPropertyIterator->first.c_str();
    }
  }

  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyIterator::GetPropertyLabel()
{
  // Self property
  if (this->Internals->PropertyIterator != this->Proxy->Internals->Properties.end())
  {
    return this->GetProperty()->GetXMLLabel();
  }

  // Property of a sub-proxy
  if (this->TraverseSubProxies)
  {
    return this->GetKey();
  }

  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyIterator::GetProperty()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetProperty()");
    return nullptr;
  }
  if (this->Internals->PropertyIterator != this->Proxy->Internals->Properties.end())
  {
    return this->Internals->PropertyIterator->second.Property.GetPointer();
  }

  if (this->TraverseSubProxies)
  {
    if (this->Internals->ExposedPropertyIterator != this->Proxy->Internals->ExposedProperties.end())
    {
      vtkSMProxy* proxy = this->Proxy->GetSubProxy(
        this->Internals->ExposedPropertyIterator->second.SubProxyName.c_str());
      if (!proxy)
      {
        vtkErrorMacro(<< "In proxy " << this->Proxy->GetXMLName() << " cannot find sub proxy "
                      << this->Internals->ExposedPropertyIterator->second.SubProxyName.c_str()
                      << " that is supposed to contain exposed property "
                      << this->Internals->ExposedPropertyIterator->first.c_str());
        return nullptr;
      }
      vtkSMProperty* property =
        proxy->GetProperty(this->Internals->ExposedPropertyIterator->second.PropertyName.c_str());
      if (!property)
      {
        vtkErrorMacro(<< "In proxy " << this->Proxy->GetXMLName()
                      << " cannot find exposed property "
                      << this->Internals->ExposedPropertyIterator->second.PropertyName.c_str()
                      << " in sub proxy "
                      << this->Internals->ExposedPropertyIterator->second.SubProxyName.c_str());
      }
      return property;
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TraverseSubProxies: " << this->TraverseSubProxies << endl;
  os << indent << "Proxy: " << this->Proxy << endl;
}
