/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyInternals.h"

vtkStandardNewMacro(vtkSMPropertyIterator);
vtkCxxRevisionMacro(vtkSMPropertyIterator, "1.2");

struct vtkSMPropertyIteratorInternals
{
  vtkSMProxyInternals::PropertyInfoMap::iterator PropertyIterator;
  vtkSMProxyInternals::ProxyMap::iterator ProxyIterator;
  vtkSMProxyInternals::PropertyInfoMap::iterator SubPropertyIterator;
};

//---------------------------------------------------------------------------
vtkSMPropertyIterator::vtkSMPropertyIterator()
{
  this->Proxy = 0;
  this->Internals = new vtkSMPropertyIteratorInternals;
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator::~vtkSMPropertyIterator()
{
  if (this->Proxy)
    {
    this->Proxy->Delete();
    }
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::SetProxy(vtkSMProxy* proxy)
{
  if (this->Proxy != proxy)
    {
    if (this->Proxy != NULL) { this->Proxy->UnRegister(this); }
    this->Proxy = proxy;
    if (this->Proxy != NULL) 
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

  this->Internals->PropertyIterator = 
    this->Proxy->Internals->Properties.begin(); 
  this->Internals->ProxyIterator = 
    this->Proxy->Internals->SubProxies.begin(); 

  while (this->Internals->ProxyIterator != 
         this->Proxy->Internals->SubProxies.end())
    {
    this->Internals->SubPropertyIterator = 
      this->Internals->ProxyIterator->second->Internals->Properties.begin();
    if ( this->Internals->SubPropertyIterator !=
         this->Internals->ProxyIterator->second->Internals->Properties.end())
      {
      break;
      }
    this->Internals->ProxyIterator++;
    }
}

//---------------------------------------------------------------------------
int vtkSMPropertyIterator::IsAtEnd()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: IsAtEnd()");
    return 1;
    }
  if ( (this->Internals->PropertyIterator == this->Proxy->Internals->Properties.end()) && (this->Internals->ProxyIterator == this->Proxy->Internals->SubProxies.end()) )
    {
    return 1;
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
  if (this->Internals->PropertyIterator != 
      this->Proxy->Internals->Properties.end())
    {
    this->Internals->PropertyIterator++;
    return;
    }

  if (this->Internals->ProxyIterator != 
      this->Proxy->Internals->SubProxies.end())
    {
    if (this->Internals->SubPropertyIterator != 
        this->Internals->ProxyIterator->second->Internals->Properties.end())
      {
      this->Internals->SubPropertyIterator++;
      }

    if (this->Internals->SubPropertyIterator == 
        this->Internals->ProxyIterator->second->Internals->Properties.end())
      {
      this->Internals->ProxyIterator++;
      while (this->Internals->ProxyIterator != 
             this->Proxy->Internals->SubProxies.end())
        {
        this->Internals->SubPropertyIterator = 
          this->Internals->ProxyIterator->second->Internals->Properties.begin();
        if ( this->Internals->SubPropertyIterator !=
             this->Internals->ProxyIterator->second->Internals->Properties.end())
          {
          break;
          }
        this->Internals->ProxyIterator++;
        }
      }
    }
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyIterator::GetKey()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetKey()");
    return 0;
    }

  if (this->Internals->PropertyIterator != 
      this->Proxy->Internals->Properties.end())
    {
    return this->Internals->PropertyIterator->first.c_str();
    }

  if (this->Internals->ProxyIterator != 
      this->Proxy->Internals->SubProxies.end())
    {
    return this->Internals->SubPropertyIterator->first.c_str();
    }

  return 0;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyIterator::GetProperty()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetProperty()");
    return 0;
    }
  if (this->Internals->PropertyIterator != 
      this->Proxy->Internals->Properties.end())
    {
    return this->Internals->PropertyIterator->second.Property;
    }

  if (this->Internals->ProxyIterator != 
      this->Proxy->Internals->SubProxies.end())
    {
    return this->Internals->SubPropertyIterator->second.Property;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Proxy: " << this->Proxy << endl;
}
