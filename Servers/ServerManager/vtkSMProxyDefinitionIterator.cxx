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
vtkCxxRevisionMacro(vtkSMProxyDefinitionIterator, "1.2");

class vtkSMProxyDefinitionIteratorInternals
{
public:
  vtkSMProxyManagerElementMapType::iterator ProxyIterator;
  vtkSMProxyManagerInternals::GroupMapType::iterator GroupIterator;

  vtkSMProxyManagerInternals::DefinitionType::iterator CompoundProxyIterator;
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

  this->Internals->CompoundProxyIterator = 
    pm->Internals->CompoundProxyDefinitions.begin();
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
}

//-----------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::Next()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("ProxyManager is not set. Can not perform operation: Next()");
    return;
    }

  if (this->Mode == COMPOUND_PROXY_DEFINITIONS)
    {
    if (this->Internals->CompoundProxyIterator ==
      pm->Internals->CompoundProxyDefinitions.end())
      {
      return;
      }
    this->Internals->CompoundProxyIterator++;
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

  if (this->Mode == COMPOUND_PROXY_DEFINITIONS)
    {
    if (this->Internals->CompoundProxyIterator == 
      pm->Internals->CompoundProxyDefinitions.end())
      {
      return 1;
      }

    return 0;
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

  if (this->Mode == COMPOUND_PROXY_DEFINITIONS)
    {
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

  if (this->Mode == COMPOUND_PROXY_DEFINITIONS)
    {
    if (this->Internals->CompoundProxyIterator !=
      pm->Internals->CompoundProxyDefinitions.end())
      {
      this->Internals->CompoundProxyIterator->first.c_str();
      }
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
void vtkSMProxyDefinitionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
}
