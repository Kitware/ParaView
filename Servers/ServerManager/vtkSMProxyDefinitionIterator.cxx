/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyDefinitionIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyDefinitionIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyManagerInternals.h"

vtkStandardNewMacro(vtkSMProxyDefinitionIterator);

class vtkSMProxyDefinitionIteratorInternals
{
public:
  vtkSMProxyManagerElementMapType::iterator ProxyIterator;
  vtkSMProxyManagerInternals::GroupMapType::iterator GroupIterator;
};

//-----------------------------------------------------------------------------
vtkSMProxyDefinitionIterator::vtkSMProxyDefinitionIterator()
{
  this->Internals = new vtkSMProxyDefinitionIteratorInternals;

  this->Mode = vtkSMProxyDefinitionIterator::ALL;
  this->Begin();
}

//-----------------------------------------------------------------------------
vtkSMProxyDefinitionIterator::~vtkSMProxyDefinitionIterator()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::Begin()
{
  vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operatrion: Begin();");
    return;
    }

  this->Internals->GroupIterator = pm->Internals->GroupMap.begin();
  if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
    {
    this->Internals->ProxyIterator =
      this->Internals->GroupIterator->second.begin();
    }

  if (this->Mode == CUSTOM_ONLY)
    {
    this->MoveTillCustom();
    }
}

//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::Begin(const char* groupName)
{
  vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operatrion: Begin();");
    return;
    }

  this->Internals->GroupIterator = pm->Internals->GroupMap.find(groupName);
  if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
    {
    this->Internals->ProxyIterator =
      this->Internals->GroupIterator->second.begin();
    }

  if (this->Mode == CUSTOM_ONLY)
    {
    this->MoveTillCustom();
    }
}

//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::Next()
{
  this->NextInternal();
  if (this->Mode == CUSTOM_ONLY)
    {
    this->MoveTillCustom();
    }
}

//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::MoveTillCustom()
{
  while (!this->IsAtEnd() && !this->IsCustom())
    {
    this->NextInternal();
    }
}

//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::NextInternal()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: Next()");
    return;
    }

  if (this->Internals->GroupIterator == 
    pm->Internals->GroupMap.end())
    {
    // reached end.
    return;
    }

  if (this->Mode == vtkSMProxyDefinitionIterator::GROUPS_ONLY)
    {
    // Iterating over groups alone, Next() should take us to the next group.
    this->Internals->GroupIterator++;
    if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
      {
      this->Internals->ProxyIterator = 
        this->Internals->GroupIterator->second.begin();
      }
    return;
    }

  if (this->Internals->ProxyIterator != 
    this->Internals->GroupIterator->second.end())
    {
    this->Internals->ProxyIterator++;
    }

  if (this->Mode != vtkSMProxyDefinitionIterator::ONE_GROUP)
    {
    // If not iterating over One group only and we've reached the
    // end of the current group, we must go to the start of the next
    // non-empty group.
    if (this->Internals->ProxyIterator == 
      this->Internals->GroupIterator->second.end())
      {
      this->Internals->GroupIterator++;
      while (this->Internals->GroupIterator !=
        pm->Internals->GroupMap.end())
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


//-----------------------------------------------------------------------------
int vtkSMProxyDefinitionIterator::IsAtEnd()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: IsAtEnd()");
    return 1;
    }

  if (this->Internals->GroupIterator == 
    pm->Internals->GroupMap.end())
    {
    return 1;
    }

  if ( this->Mode == vtkSMProxyDefinitionIterator::ONE_GROUP &&
    this->Internals->ProxyIterator == 
    this->Internals->GroupIterator->second.end() )
    {
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyDefinitionIterator::GetGroup()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetGroup()");
    return 0;
    }

  if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
    {
    return this->Internals->GroupIterator->first.c_str();
    }
  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyDefinitionIterator::GetKey()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetKey()");
    return 0;
    }

  if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
    {
    if (this->Internals->ProxyIterator != 
        this->Internals->GroupIterator->second.end())
      {
      return this->Internals->ProxyIterator->first.c_str();
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyDefinitionIterator::GetDefinition()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetKey()");
    return 0;
    }

  if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
    {
    if (this->Internals->ProxyIterator != 
        this->Internals->GroupIterator->second.end())
      {
      return this->Internals->ProxyIterator->second.GetPointer();
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
bool vtkSMProxyDefinitionIterator::IsCustom()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: GetKey()");
    return false;
    }

  if (this->Internals->GroupIterator != pm->Internals->GroupMap.end())
    {
    if (this->Internals->ProxyIterator != 
        this->Internals->GroupIterator->second.end())
      {
      return this->Internals->ProxyIterator->second.Custom;
      }
    }

  return false;
}


//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
}
