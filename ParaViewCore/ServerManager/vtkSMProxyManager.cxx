/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyManager.h"

#include "vtkPVConfig.h" // for PARAVIEW_VERSION_*
#include "vtkObjectFactory.h"
#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkEventForwarderCommand.h"

#include <vtksys/DateStamp.h> // For date stamp
#include <vtkstd/map>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#define PARAVIEW_SOURCE_VERSION "paraview version " PARAVIEW_VERSION_FULL ", Date: " vtksys_DATE_STAMP_STRING
//***************************************************************************
class vtkSMProxyManager::vtkPXMInternal
{
public:
  // Data structure for selection models.
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkSMProxySelectionModel> >
    SelectionModelsType;
  SelectionModelsType SelectionModels;

  vtkWeakPointer<vtkSMSession> ActiveSession;
  vtkSmartPointer<vtkSMUndoStackBuilder> UndoStackBuilder;
  vtkstd::map<vtkSMSession*, vtkSmartPointer<vtkEventForwarderCommand> > EventForwarderMap;
  vtkstd::map<vtkSMSession*, vtkSmartPointer<vtkSMSessionProxyManager> > SessionProxyManagerMap;
};
//***************************************************************************
// Statics...
vtkSmartPointer<vtkSMProxyManager> vtkSMProxyManager::Singleton;
//***************************************************************************
vtkStandardNewMacro(vtkSMProxyManager);
//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->PXMStorage = new vtkPXMInternal();
}
//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMProxyManager::GetProxyManager()
{
  if(!vtkSMProxyManager::Singleton)
    {
    vtkSMProxyManager::Singleton =
        vtkSmartPointer<vtkSMProxyManager>::New();
    }
  return vtkSMProxyManager::Singleton.GetPointer();
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::Finalize()
{
  vtkSMProxyManager::Singleton = NULL;
}

//---------------------------------------------------------------------------
bool vtkSMProxyManager::IsInitialized()
{
  return (vtkSMProxyManager::Singleton != NULL);
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  delete this->PXMStorage;
  this->PXMStorage = NULL;
}

//----------------------------------------------------------------------------
const char* vtkSMProxyManager::GetParaViewSourceVersion()
{
  return PARAVIEW_SOURCE_VERSION;
}

//----------------------------------------------------------------------------
int vtkSMProxyManager::GetVersionMajor()
{
  return PARAVIEW_VERSION_MAJOR;
}

//----------------------------------------------------------------------------
int vtkSMProxyManager::GetVersionMinor()
{
  return PARAVIEW_VERSION_MINOR;
}

//----------------------------------------------------------------------------
int vtkSMProxyManager::GetVersionPatch()
{
  return PARAVIEW_VERSION_PATCH;
}

//----------------------------------------------------------------------------
vtkSMSession* vtkSMProxyManager::GetActiveSession()
{
  return this->PXMStorage->ActiveSession;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::SetActiveSession(vtkSMSession* session)
{
  this->PXMStorage->ActiveSession = session;
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMProxyManager::GetActiveSessionProxyManager()
{
  // If no active session, find the first available session and set it to active
  if(!this->PXMStorage->ActiveSession)
    {
    vtkstd::map<vtkSMSession*, vtkSmartPointer<vtkSMSessionProxyManager> >::iterator iter;
    iter = this->PXMStorage->SessionProxyManagerMap.begin();

    // Loop over all Session
    while(iter != this->PXMStorage->SessionProxyManagerMap.end())
      {
      if(vtkSMSession* session = iter->first)
        {
        this->SetActiveSession(session);
        return this->GetSessionProxyManager(this->GetActiveSession());
        }

      // Go to the next one
      iter++;
      }
    }
  return this->GetSessionProxyManager(this->GetActiveSession());
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMProxyManager::GetSessionProxyManager(vtkSMSession* session)
{
  vtkSMSessionProxyManager* pxm = NULL;
  if(session)
    {
    if(this->PXMStorage->ActiveSession == NULL)
      {
      this->SetActiveSession(session);
      }

    pxm = this->PXMStorage->SessionProxyManagerMap[session];
    if(!pxm)
      {
      this->PXMStorage->SessionProxyManagerMap[session].TakeReference(vtkSMSessionProxyManager::New());
      this->PXMStorage->EventForwarderMap[session].TakeReference(vtkEventForwarderCommand::New());
      pxm = this->PXMStorage->SessionProxyManagerMap[session];
      pxm->SetSession(session);
      session->SetUndoStackBuilder(this->PXMStorage->UndoStackBuilder);

      // Init event forwarder
      vtkEventForwarderCommand* forwarder = this->PXMStorage->EventForwarderMap[session];
      forwarder->SetTarget(this);
      pxm->AddObserver(vtkCommand::AnyEvent, forwarder);
      }
    }
  return pxm;
}
//----------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterSession(vtkSMSession* sessionToRemove)
{
  this->PXMStorage->EventForwarderMap.erase(sessionToRemove);
  this->PXMStorage->SessionProxyManagerMap.erase(sessionToRemove);
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::UpdateAllRegisteredProxiesInOrder()
{
  // Consts
  const char* groupsOrder[4] =
    {"sources", "lookup_tables", "representations", "scalar_bars"};

  // Vars
  vtkstd::map<vtkSMSession*, vtkSmartPointer<vtkSMSessionProxyManager> >::iterator iter;
  iter = this->PXMStorage->SessionProxyManagerMap.begin();

  // Loop over all SessionProxyManager
  while(iter != this->PXMStorage->SessionProxyManagerMap.end())
    {
    if(vtkSMSessionProxyManager* pxm = iter->second.GetPointer())
      {
      for(int i=0; i < 4; i++)
        {
        pxm->UpdateRegisteredProxies(groupsOrder[i], 1);
        }
      pxm->UpdateRegisteredProxies(1);
      }

    // Go to the next one
    iter++;
    }
}
//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterSelectionModel(
  const char* name, vtkSMProxySelectionModel* model)
{
  if (!model)
    {
    vtkErrorMacro("Cannot register a null model.");
    return;
    }
  if (!name)
    {
    vtkErrorMacro("Cannot register model with no name.");
    return;
    }

  vtkSMProxySelectionModel* curmodel = this->GetSelectionModel(name);
  if (curmodel && curmodel == model)
    {
    // already registered.
    return;
    }

  if (curmodel)
    {
    vtkWarningMacro("Replacing existing selection model: " << name);
    }
  this->PXMStorage->SelectionModels[name] = model;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterSelectionModel( const char* name)
{
  this->PXMStorage->SelectionModels.erase(name);
}

//---------------------------------------------------------------------------
vtkSMProxySelectionModel* vtkSMProxyManager::GetSelectionModel(
  const char* name)
{
  vtkSMProxyManager::vtkPXMInternal::SelectionModelsType::iterator iter =
    this->PXMStorage->SelectionModels.find(name);
  if (iter == this->PXMStorage->SelectionModels.end())
    {
    return 0;
    }

  return iter->second;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::AttachUndoStackBuilder(vtkSMUndoStackBuilder* undoBuilder)
{
  this->PXMStorage->UndoStackBuilder = undoBuilder;
  vtkstd::map<vtkSMSession*, vtkSmartPointer<vtkSMSessionProxyManager> >::iterator iter;
  iter = this->PXMStorage->SessionProxyManagerMap.begin();

  // Loop over all Session
  while(iter != this->PXMStorage->SessionProxyManagerMap.end())
    {
    if(vtkSMSession* session = iter->first)
      {
      session->SetUndoStackBuilder(this->PXMStorage->UndoStackBuilder);
      }

    // Go to the next one
    iter++;
    }
}

//---------------------------------------------------------------------------
bool vtkSMProxyManager::HasSessionProxyManager()
{
  return (this->PXMStorage->SessionProxyManagerMap.size() > 0);
}
