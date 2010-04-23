/*=========================================================================

  Program:   ParaView
  Module:    vtkConnectionIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkConnectionIterator.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModuleConnectionManagerInternals.h"

//*****************************************************************************
class vtkConnectionIteratorInternals
{
public:
  vtkProcessModuleConnectionManagerInternals::MapOfIDToConnection::iterator Iter;
};

//*****************************************************************************
vtkStandardNewMacro(vtkConnectionIterator);
vtkCxxSetObjectMacro(vtkConnectionIterator, ConnectionManager,
  vtkProcessModuleConnectionManager);
//-----------------------------------------------------------------------------
vtkConnectionIterator::vtkConnectionIterator()
{
  this->MatchConnectionID = 
    vtkProcessModuleConnectionManager::GetAllConnectionsID();
  this->ConnectionManager = 0;
  this->Internals = new vtkConnectionIteratorInternals;
  this->InBegin = 0;
}

//-----------------------------------------------------------------------------
vtkConnectionIterator::~vtkConnectionIterator()
{
  this->SetConnectionManager(0);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkConnectionIterator::Begin()
{
  
  if (!this->ConnectionManager)
    {
    vtkErrorMacro("ConnectionManager must be set.");
    return;
    }
  this->InBegin = 1;
  if (this->MatchConnectionID == 
    vtkProcessModuleConnectionManager::GetAllConnectionsID())
    {
    this->Internals->Iter = 
      this->ConnectionManager->Internals->IDToConnectionMap.begin();
    }
  else if (this->MatchConnectionID ==
    vtkProcessModuleConnectionManager::GetAllServerConnectionsID() ||
    this->MatchConnectionID == 
    vtkProcessModuleConnectionManager::GetRootServerConnectionID())
    {
    this->Internals->Iter = 
      this->ConnectionManager->Internals->IDToConnectionMap.begin();
    // Go to the first server connection.
    while (!this->IsAtEnd() && 
      !this->ConnectionManager->IsServerConnection(this->GetCurrentConnectionID()))
      {
      this->Next();
      }
    }
  else
    {
    this->Internals->Iter = 
      this->ConnectionManager->Internals->IDToConnectionMap.find(
        this->MatchConnectionID);
    }
  this->InBegin = 0;
}

//-----------------------------------------------------------------------------
int vtkConnectionIterator::IsAtEnd()
{
  if (!this->ConnectionManager)
    {
    vtkErrorMacro("ConnectionManager must be set.");
    return 1;
    }

  return (this->Internals->Iter ==
    this->ConnectionManager->Internals->IDToConnectionMap.end());
}

//-----------------------------------------------------------------------------
void vtkConnectionIterator::Next()
{
  if (!this->ConnectionManager)
    {
    vtkErrorMacro("ConnectionManager must be set.");
    return;
    }

  this->Internals->Iter++;
  // If the match id was to match only a single conneciton,
  // we terminate the iteration.
  if (!this->InBegin && this->MatchConnectionID != 
    vtkProcessModuleConnectionManager::GetAllConnectionsID() &&
    this->MatchConnectionID !=
    vtkProcessModuleConnectionManager::GetAllServerConnectionsID())
    {
    // The Root server connection was already pointed to when Begin() 
    // was called. So we finish iteration.
    this->Internals->Iter = 
      this->ConnectionManager->Internals->IDToConnectionMap.end();
    }
}

//-----------------------------------------------------------------------------
vtkProcessModuleConnection* vtkConnectionIterator::GetCurrentConnection()
{
  if (!this->ConnectionManager)
    {
    vtkErrorMacro("ConnectionManager must be set.");
    return NULL;
    }

  return this->Internals->Iter->second.GetPointer();
}

//-----------------------------------------------------------------------------
vtkIdType vtkConnectionIterator::GetCurrentConnectionID()
{
  if (!this->ConnectionManager)
    {
    vtkErrorMacro("ConnectionManager must be set.");
    return vtkProcessModuleConnectionManager::GetNullConnectionID();
    }

  return this->Internals->Iter->first;
}

//-----------------------------------------------------------------------------
void vtkConnectionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MatchConnectionID: " << this->MatchConnectionID << endl;
  os << indent << "ConnectionManager: " << this->ConnectionManager << endl;
  
}
