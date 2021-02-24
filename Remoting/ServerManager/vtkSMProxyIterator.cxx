/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSessionProxyManagerInternals.h"

vtkStandardNewMacro(vtkSMProxyIterator);

struct vtkSMProxyIteratorInternals
{
  vtkSMProxyManagerProxyListType::iterator ProxyIterator;
  vtkSMProxyManagerProxyMapType::iterator ProxyListIterator;
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator GroupIterator;
  vtkWeakPointer<vtkSMSessionProxyManager> ProxyManager;
};

//---------------------------------------------------------------------------
vtkSMProxyIterator::vtkSMProxyIterator()
{
  this->Internals = new vtkSMProxyIteratorInternals;
  this->Mode = vtkSMProxyIterator::ALL;
  this->SkipPrototypes = true;
}

//---------------------------------------------------------------------------
vtkSMProxyIterator::~vtkSMProxyIterator()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::SetSession(vtkSMSession* session)
{
  this->SetSessionProxyManager(session ? session->GetSessionProxyManager() : nullptr);
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::SetSessionProxyManager(vtkSMSessionProxyManager* pxm)
{
  this->Internals->ProxyManager = pxm;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Begin(const char* groupName)
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;

  if (!pm)
  {
    vtkWarningMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
  }
  this->SetModeToOneGroup();
  this->Internals->GroupIterator = pm->Internals->RegisteredProxyMap.find(groupName);
  if (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
  {
    this->Internals->ProxyListIterator = this->Internals->GroupIterator->second.begin();
    if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
    {
      this->Internals->ProxyIterator = this->Internals->ProxyListIterator->second.begin();
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Begin()
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;

  if (!pm)
  {
    vtkWarningMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
  }
  this->Internals->GroupIterator = pm->Internals->RegisteredProxyMap.begin();
  while (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
  {
    this->Internals->ProxyListIterator = this->Internals->GroupIterator->second.begin();
    while (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
    {
      this->Internals->ProxyIterator = this->Internals->ProxyListIterator->second.begin();
      if (this->Internals->ProxyIterator != this->Internals->ProxyListIterator->second.end())
      {
        break;
      }
      this->Internals->ProxyListIterator++;
    }
    if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
    {
      break;
    }
    this->Internals->GroupIterator++;
  }
  if (this->SkipPrototypes)
  {
    auto proxy = this->GetProxy();
    if (proxy && (proxy->GetSession() == nullptr || proxy->IsPrototype()))
    {
      this->Next();
    }
  }
}

//---------------------------------------------------------------------------
int vtkSMProxyIterator::IsAtEnd()
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;

  if (pm == nullptr || this->Internals->GroupIterator == pm->Internals->RegisteredProxyMap.end())
  {
    return 1;
  }
  if (this->Mode == vtkSMProxyIterator::ONE_GROUP &&
    this->Internals->ProxyListIterator == this->Internals->GroupIterator->second.end())
  {
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Next()
{
  this->NextInternal();
  if (this->SkipPrototypes && !this->IsAtEnd())
  {
    auto proxy = this->GetProxy();
    if (proxy && (proxy->GetSession() == nullptr || proxy->IsPrototype()))
    {
      this->Next();
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::NextInternal()
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;
  assert(pm != nullptr);

  if (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
  {
    if (this->Mode == vtkSMProxyIterator::GROUPS_ONLY)
    {
      this->Internals->GroupIterator++;
      if (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
      {
        this->Internals->ProxyListIterator = this->Internals->GroupIterator->second.begin();
        if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
        {
          this->Internals->ProxyIterator = this->Internals->ProxyListIterator->second.begin();
        }
      }
    }
    else
    {
      if (this->Internals->ProxyIterator != this->Internals->ProxyListIterator->second.end())
      {
        this->Internals->ProxyIterator++;
      }

      if (this->Internals->ProxyIterator == this->Internals->ProxyListIterator->second.end())
      {
        if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
        {
          // Advance the proxy list iterator till
          // we reach a non-empty proxy list. The proxy iterator
          // must also be moved to the start of this new list.
          this->Internals->ProxyListIterator++;
          while (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
          {
            this->Internals->ProxyIterator = this->Internals->ProxyListIterator->second.begin();
            if (this->Internals->ProxyIterator != this->Internals->ProxyListIterator->second.end())
            {
              break;
            }
            this->Internals->ProxyListIterator++;
          }
        }
      }

      if (this->Mode != vtkSMProxyIterator::ONE_GROUP)
      {
        if (this->Internals->ProxyListIterator == this->Internals->GroupIterator->second.end())
        {
          // Advance the group iter till we reach a non-empty group.
          // The proxt list iter and the proxy iter also need to be
          // updated accordingly.
          this->Internals->GroupIterator++;
          while (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
          {
            this->Internals->ProxyListIterator = this->Internals->GroupIterator->second.begin();

            while (
              this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
            {
              this->Internals->ProxyIterator = this->Internals->ProxyListIterator->second.begin();
              if (this->Internals->ProxyIterator !=
                this->Internals->ProxyListIterator->second.end())
              {
                break;
              }
              this->Internals->ProxyListIterator++;
            }

            if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
            {
              break;
            }
            this->Internals->GroupIterator++;
          }
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
const char* vtkSMProxyIterator::GetGroup()
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;
  assert(pm != nullptr);

  if (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
  {
    return this->Internals->GroupIterator->first.c_str();
  }
  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyIterator::GetKey()
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;
  assert(pm != nullptr);

  if (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
  {
    if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
    {
      return this->Internals->ProxyListIterator->first.c_str();
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyIterator::GetProxy()
{
  vtkSMSessionProxyManager* pm = this->Internals->ProxyManager;
  assert(pm != nullptr);

  if (this->Internals->GroupIterator != pm->Internals->RegisteredProxyMap.end())
  {
    if (this->Internals->ProxyListIterator != this->Internals->GroupIterator->second.end())
    {
      if (this->Internals->ProxyIterator != this->Internals->ProxyListIterator->second.end())
      {
        return this->Internals->ProxyIterator->GetPointer()->Proxy.GetPointer();
      }
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SkipPrototypes: " << this->SkipPrototypes << endl;
  os << indent << "Mode: " << this->Mode << endl;
}
