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
#include "vtkSMProxyManager.h"
#include "vtkSMProxyManagerInternals.h"

vtkStandardNewMacro(vtkSMProxyIterator);
vtkCxxRevisionMacro(vtkSMProxyIterator, "1.2");

struct vtkSMProxyIteratorInternals
{
  vtkSMProxyManagerProxyMapType::iterator ProxyIterator;
  vtkSMProxyManagerInternals::ProxyGroupType::iterator GroupIterator;
};

//---------------------------------------------------------------------------
vtkSMProxyIterator::vtkSMProxyIterator()
{
  this->Internals = new vtkSMProxyIteratorInternals;

  this->Mode = vtkSMProxyIterator::ALL;
  this->Begin();
}

//---------------------------------------------------------------------------
vtkSMProxyIterator::~vtkSMProxyIterator()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Begin(const char* groupName)
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
    }
  this->Internals->GroupIterator = 
    pm->Internals->RegisteredProxyMap.find(groupName);
  if (this->Internals->GroupIterator!=pm->Internals->RegisteredProxyMap.end())
    {
    this->Internals->ProxyIterator = 
      this->Internals->GroupIterator->second.begin();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Begin()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
    }
  this->Internals->GroupIterator = pm->Internals->RegisteredProxyMap.begin();
  if (this->Internals->GroupIterator!=pm->Internals->RegisteredProxyMap.end())
    {
    this->Internals->ProxyIterator = 
      this->Internals->GroupIterator->second.begin();
    }
}

//---------------------------------------------------------------------------
int vtkSMProxyIterator::IsAtEnd()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: IsAtEnd()");
    return 1;
    }
  if (this->Internals->GroupIterator == 
      pm->Internals->RegisteredProxyMap.end())
    {
    return 1;
    }
  if ( this->Mode == vtkSMProxyIterator::ONE_GROUP &&
       this->Internals->ProxyIterator == 
       this->Internals->GroupIterator->second.end() )
    {
    return 1;
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Next()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: Next()");
    return;
    }

  if (this->Internals->GroupIterator != 
      pm->Internals->RegisteredProxyMap.end())
    {
    if (this->Mode == vtkSMProxyIterator::GROUPS_ONLY)
      {
      this->Internals->GroupIterator++;
      if (this->Internals->GroupIterator != 
          pm->Internals->RegisteredProxyMap.end())
        {
        this->Internals->ProxyIterator = 
          this->Internals->GroupIterator->second.begin();
        }
      }
    else
      {
      if (this->Internals->ProxyIterator != 
          this->Internals->GroupIterator->second.end())
        {
        this->Internals->ProxyIterator++;
        }
      
      if (this->Mode != vtkSMProxyIterator::ONE_GROUP)
        {
        if (this->Internals->ProxyIterator == 
            this->Internals->GroupIterator->second.end())
          {
          this->Internals->GroupIterator++;
          while (this->Internals->GroupIterator !=
                 pm->Internals->RegisteredProxyMap.end())
            {
            this->Internals->ProxyIterator = 
              this->Internals->GroupIterator->second.begin();
            if ( this->Internals->ProxyIterator !=
                 this->Internals->GroupIterator->second.end() )
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
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetGroup()");
    return 0;
    }

  if (this->Internals->GroupIterator != 
      pm->Internals->RegisteredProxyMap.end())
    {
    return this->Internals->GroupIterator->first.c_str();
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyIterator::GetKey()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetKey()");
    return 0;
    }

  if (this->Internals->GroupIterator != 
      pm->Internals->RegisteredProxyMap.end())
    {
    if (this->Internals->ProxyIterator != 
        this->Internals->GroupIterator->second.end())
      {
      return this->Internals->ProxyIterator->first.c_str();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyIterator::GetProxy()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetProxy()");
    return 0;
    }

  if (this->Internals->GroupIterator != 
      pm->Internals->RegisteredProxyMap.end())
    {
    if (this->Internals->ProxyIterator != 
        this->Internals->GroupIterator->second.end())
      {
      return this->Internals->ProxyIterator->second.GetPointer();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Mode: " << this->Mode << endl;
}
