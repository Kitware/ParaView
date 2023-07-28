// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMOrderedPropertyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyInternals.h"

vtkStandardNewMacro(vtkSMOrderedPropertyIterator);

//---------------------------------------------------------------------------
vtkSMOrderedPropertyIterator::vtkSMOrderedPropertyIterator()
{
  this->Proxy = nullptr;
  this->Index = 0;
}

//---------------------------------------------------------------------------
vtkSMOrderedPropertyIterator::~vtkSMOrderedPropertyIterator()
{
  this->SetProxy(nullptr);
}

//---------------------------------------------------------------------------
void vtkSMOrderedPropertyIterator::SetProxy(vtkSMProxy* proxy)
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
void vtkSMOrderedPropertyIterator::Begin()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: Begin()");
    return;
  }

  this->Index = 0;
}

//---------------------------------------------------------------------------
int vtkSMOrderedPropertyIterator::IsAtEnd()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: IsAtEnd()");
    return 1;
  }

  if (this->Index >= this->Proxy->Internals->PropertyNamesInOrder.size())
  {
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMOrderedPropertyIterator::Next()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: Next()");
    return;
  }

  this->Index++;
}

//---------------------------------------------------------------------------
const char* vtkSMOrderedPropertyIterator::GetKey()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetKey()");
    return nullptr;
  }

  if (!this->IsAtEnd())
  {
    return this->Proxy->Internals->PropertyNamesInOrder[this->Index].c_str();
  }

  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMOrderedPropertyIterator::GetPropertyLabel()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetPropertyLabel()");
    return nullptr;
  }

  if (!this->IsAtEnd())
  {
    const char* pname = this->Proxy->Internals->PropertyNamesInOrder[this->Index].c_str();

    if (vtkSMProperty* prop = this->Proxy->GetProperty(pname, /*selfOnly*/ 1))
    {
      // Self property
      return prop->GetXMLLabel();
    }
    else
    {
      // Property of a sub-proxy
      return this->GetKey();
    }
  }

  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMOrderedPropertyIterator::GetProperty()
{
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetProperty()");
    return nullptr;
  }

  if (!this->IsAtEnd())
  {
    return this->Proxy->GetProperty(
      this->Proxy->Internals->PropertyNamesInOrder[this->Index].c_str());
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMOrderedPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Proxy: " << this->Proxy << endl;
}
