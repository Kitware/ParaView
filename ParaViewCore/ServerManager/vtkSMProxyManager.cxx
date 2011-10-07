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
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkPVXMLElement.h"

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

  // Data structure for storing GlobalPropertiesManagers.
  typedef vtkstd::map<vtkstd::string,
          vtkSmartPointer<vtkSMGlobalPropertiesManager> >
            GlobalPropertiesManagersType;
  typedef vtkstd::map<vtkstd::string,
          unsigned long >
            GlobalPropertiesManagersCallBackIDType;
  GlobalPropertiesManagersType GlobalPropertiesManagers;
  GlobalPropertiesManagersCallBackIDType GlobalPropertiesManagersCallBackID;

  // GlobalPropertiesManagerObserver
  void GlobalPropertyEvent(vtkObject* src, unsigned long event, void* data)
    {
    vtkSMGlobalPropertiesManager* globalPropertiesManager =
        vtkSMGlobalPropertiesManager::SafeDownCast(src);

    // We are only managing UndoElements on GlobalPropertyManager when only one
    // server is involved !!!
    if(globalPropertiesManager && this->SessionProxyManagerMap.size() == 1)
      {
      vtkSMSession* session = this->SessionProxyManagerMap.begin()->first;
      vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();
      const char* globalPropertiesManagerName =
          pxm->GetGlobalPropertiesManagerName(globalPropertiesManager);
      if( globalPropertiesManagerName &&
          this->UndoStackBuilder &&
          event == vtkSMGlobalPropertiesManager::GlobalPropertyLinkModified)
        {
        vtkSMGlobalPropertiesManager::ModifiedInfo* modifiedInfo;
        modifiedInfo = reinterpret_cast<vtkSMGlobalPropertiesManager::ModifiedInfo*>(data);

        vtkSMGlobalPropertiesLinkUndoElement* undoElem = vtkSMGlobalPropertiesLinkUndoElement::New();
        undoElem->SetSession(session);
        undoElem->SetLinkState( globalPropertiesManagerName,
                                modifiedInfo->GlobalPropertyName,
                                modifiedInfo->Proxy,
                                modifiedInfo->PropertyName,
                                modifiedInfo->AddLink);
        this->UndoStackBuilder->Add(undoElem);
        undoElem->Delete();
        }
      }
    }
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
//---------------------------------------------------------------------------
void vtkSMProxyManager::SetGlobalPropertiesManager(const char* name,
    vtkSMGlobalPropertiesManager* mgr)
{
  vtkSMGlobalPropertiesManager* old_mgr = this->GetGlobalPropertiesManager(name);
  if (old_mgr == mgr)
    {
    return;
    }
  this->RemoveGlobalPropertiesManager(name);
  this->PXMStorage->GlobalPropertiesManagers[name] = mgr;
  this->PXMStorage->GlobalPropertiesManagersCallBackID[name] =
      mgr->AddObserver(vtkSMGlobalPropertiesManager::GlobalPropertyLinkModified,
                       this->PXMStorage, &vtkSMProxyManager::vtkPXMInternal::GlobalPropertyEvent);

  vtkSMProxyManager::RegisteredProxyInformation info;
  info.Proxy = mgr;
  info.GroupName = NULL;
  info.ProxyName = name;
  info.Type = vtkSMProxyManager::RegisteredProxyInformation::GLOBAL_PROPERTIES_MANAGER;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetGlobalPropertiesManagerName(
  vtkSMGlobalPropertiesManager* mgr)
{
  vtkSMProxyManager::vtkPXMInternal::GlobalPropertiesManagersType::iterator iter;
  for (iter = this->PXMStorage->GlobalPropertiesManagers.begin();
    iter != this->PXMStorage->GlobalPropertiesManagers.end(); ++iter)
    {
    if (iter->second == mgr)
      {
      return iter->first.c_str();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* vtkSMProxyManager::GetGlobalPropertiesManager(
  const char* name)
{
  return this->PXMStorage->GlobalPropertiesManagers[name].GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RemoveGlobalPropertiesManager(const char* name)
{
  vtkSMGlobalPropertiesManager* gm = this->GetGlobalPropertiesManager(name);
  if (gm)
    {
    gm->RemoveObserver(this->PXMStorage->GlobalPropertiesManagersCallBackID[name]);
    vtkSMProxyManager::RegisteredProxyInformation info;
    info.Proxy = gm;
    info.GroupName = NULL;
    info.ProxyName = name;
    info.Type = vtkSMProxyManager::RegisteredProxyInformation::GLOBAL_PROPERTIES_MANAGER;
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    }
  this->PXMStorage->GlobalPropertiesManagers.erase(name);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyManager::GetNumberOfGlobalPropertiesManagers()
{
  return static_cast<unsigned int>(
    this->PXMStorage->GlobalPropertiesManagers.size());
}

//---------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* vtkSMProxyManager::GetGlobalPropertiesManager(
  unsigned int index)
{
  unsigned int cur =0;
  vtkSMProxyManager::vtkPXMInternal::GlobalPropertiesManagersType::iterator iter;
  for (iter = this->PXMStorage->GlobalPropertiesManagers.begin();
    iter != this->PXMStorage->GlobalPropertiesManagers.end(); ++iter, ++cur)
    {
    if (cur == index)
      {
      return iter->second;
      }
    }

  return NULL;
}
//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveGlobalPropertiesManagers(vtkPVXMLElement* root)
{
  vtkPXMInternal::GlobalPropertiesManagersType::iterator iter;
  for (iter = this->PXMStorage->GlobalPropertiesManagers.begin();
    iter != this->PXMStorage->GlobalPropertiesManagers.end(); ++iter)
    {
    vtkPVXMLElement* elem = iter->second->SaveLinkState(root);
    if (elem)
      {
      elem->AddAttribute("name", iter->first.c_str());
      }
    }
}
