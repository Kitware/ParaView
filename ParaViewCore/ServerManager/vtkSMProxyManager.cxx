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

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION_*
#include "vtkPVXMLElement.h"
#include "vtkSessionIterator.h"
#include "vtkSmartPointer.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkWeakPointer.h"

#include <vtksys/DateStamp.h> // For date stamp
#include <vtkstd/map>

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
    // FIXME: FIXME_MULTI_SERVER
    //vtkSMGlobalPropertiesManager* globalPropertiesManager =
    //    vtkSMGlobalPropertiesManager::SafeDownCast(src);

    //// We are only managing UndoElements on GlobalPropertyManager when only one
    //// server is involved !!!
    //if(globalPropertiesManager && this->SessionProxyManagerMap.size() == 1)
    //  {
    //  vtkSMSession* session = this->SessionProxyManagerMap.begin()->first;
    //  vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();
    //  const char* globalPropertiesManagerName =
    //      pxm->GetGlobalPropertiesManagerName(globalPropertiesManager);
    //  if( globalPropertiesManagerName &&
    //      this->UndoStackBuilder &&
    //      event == vtkSMGlobalPropertiesManager::GlobalPropertyLinkModified)
    //    {
    //    vtkSMGlobalPropertiesManager::ModifiedInfo* modifiedInfo;
    //    modifiedInfo = reinterpret_cast<vtkSMGlobalPropertiesManager::ModifiedInfo*>(data);

    //    vtkSMGlobalPropertiesLinkUndoElement* undoElem = vtkSMGlobalPropertiesLinkUndoElement::New();
    //    undoElem->SetSession(session);
    //    undoElem->SetLinkState( globalPropertiesManagerName,
    //                            modifiedInfo->GlobalPropertyName,
    //                            modifiedInfo->Proxy,
    //                            modifiedInfo->PropertyName,
    //                            modifiedInfo->AddLink);
    //    this->UndoStackBuilder->Add(undoElem);
    //    undoElem->Delete();
    //    }
    //  }
    }
};
//***************************************************************************
// Statics...
vtkSmartPointer<vtkSMProxyManager> vtkSMProxyManager::Singleton;

vtkCxxSetObjectMacro(vtkSMProxyManager, UndoStackBuilder,
  vtkSMUndoStackBuilder);
//***************************************************************************
vtkSMProxyManager* vtkSMProxyManager::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSMProxyManager");
  if (ret)
    {
    return static_cast<vtkSMProxyManager*>(ret);
    }
  return new vtkSMProxyManager;
}

//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->PXMStorage = new vtkPXMInternal();
  this->PluginManager = vtkSMPluginManager::New();
  this->UndoStackBuilder = NULL;
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  this->SetUndoStackBuilder(NULL);

  this->PluginManager->Delete();
  this->PluginManager = NULL;

  delete this->PXMStorage;
  this->PXMStorage = NULL;
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMProxyManager::GetProxyManager()
{
  if(!vtkSMProxyManager::Singleton)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::New();
    vtkSMProxyManager::Singleton.TakeReference(pxm);
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
  // If no active session, find the first available session and set it to active
  if (this->PXMStorage->ActiveSession == NULL)
    {
    vtkSMSession* session = 0;
    vtkSmartPointer<vtkSessionIterator> iter;
    iter.TakeReference(
      vtkProcessModule::GetProcessModule()->NewSessionIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
      {
      vtkSMSession* temp = vtkSMSession::SafeDownCast(
        iter->GetCurrentSession());
      if (temp && session)
        {
        // more than 1 vtkSMSession is present. In that case, user has to
        // explicitly set the active session.
        return NULL;
        }
      session = temp;
      }
    this->PXMStorage->ActiveSession = session;
    }

  return this->PXMStorage->ActiveSession;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::SetActiveSession(vtkIdType sid)
{
  this->SetActiveSession(
    vtkSMSession::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetSession(sid)));
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::SetActiveSession(vtkSMSession* session)
{
  this->PXMStorage->ActiveSession = session;

  // ensures that the active session is updated if session == NULL.
  this->GetActiveSession();
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMProxyManager::GetActiveSessionProxyManager()
{
  return this->GetSessionProxyManager(this->GetActiveSession());
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMProxyManager::GetSessionProxyManager(vtkSMSession* session)
{
  return session? session->GetSessionProxyManager() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UndoStackBuilder: " << this->UndoStackBuilder << endl;
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

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(const char* groupName,
  const char* proxyName, const char* subProxyName)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
    {
    return pxm->NewProxy(groupName, proxyName, subProxyName);
    }
  vtkErrorMacro("No active session found.");
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterProxy(
  const char* groupname, const char* name, vtkSMProxy* proxy)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
    {
    pxm->RegisterProxy(groupname, name, proxy);
    }
  else
    {
    vtkErrorMacro("No active session found.");
    }
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(const char* groupname, const char* name)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
    {
    return pxm->GetProxy(groupname, name);
    }
  vtkErrorMacro("No active session found.");
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(
  const char* groupname, const char* name, vtkSMProxy* proxy)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
    {
    pxm->UnRegisterProxy(groupname, name, proxy);
    }
  else
    {
    vtkErrorMacro("No active session found.");
    }
}
