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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION_*
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSIProxyDefinitionManager.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMWriterFactory.h"
#include "vtkSessionIterator.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>
#include <set>
#include <sstream>
#include <vector>

#define PARAVIEW_SOURCE_VERSION "paraview version " PARAVIEW_VERSION_FULL
//***************************************************************************
class vtkSMProxyManager::vtkPXMInternal
{
public:
  vtkPXMInternal()
    : ActiveSessionID(0)
  {
  }
  ~vtkPXMInternal() {}
  vtkIdType ActiveSessionID;
};

//***************************************************************************
// Statics...
vtkSmartPointer<vtkSMProxyManager> vtkSMProxyManager::Singleton;

vtkCxxSetObjectMacro(vtkSMProxyManager, UndoStackBuilder, vtkSMUndoStackBuilder);
//***************************************************************************
vtkSMProxyManager* vtkSMProxyManager::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSMProxyManager");
  if (ret)
  {
    return static_cast<vtkSMProxyManager*>(ret);
  }
  vtkSMProxyManager* o = new vtkSMProxyManager;
  o->InitializeObjectBase();
  return o;
}

//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->PXMStorage = new vtkPXMInternal();
  this->PluginManager = vtkSMPluginManager::New();
  this->UndoStackBuilder = NULL;

  this->ReaderFactory = vtkSMReaderFactory::New();
  this->WriterFactory = vtkSMWriterFactory::New();

  // Keep track of when proxy definitions change and then if it's a new
  // reader or writer we add it to ReaderFactory or WriterFactory.
  this->BlockProxyDefinitionUpdates = false;
  this->AddObserver(vtkSIProxyDefinitionManager::ProxyDefinitionsUpdated, this,
    &vtkSMProxyManager::UpdateProxyDefinitions);
  this->AddObserver(vtkSIProxyDefinitionManager::CompoundProxyDefinitionsUpdated, this,
    &vtkSMProxyManager::UpdateProxyDefinitions);

  // Monitor session creations. If a new session is created and we don't have an
  // active one, we make that new session active.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm)
  {
    pm->AddObserver(
      vtkCommand::ConnectionCreatedEvent, this, &vtkSMProxyManager::ConnectionsUpdated);
    pm->AddObserver(
      vtkCommand::ConnectionClosedEvent, this, &vtkSMProxyManager::ConnectionsUpdated);
  }
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  this->SetUndoStackBuilder(NULL);

  this->PluginManager->Delete();
  this->PluginManager = NULL;

  this->ReaderFactory->Delete();
  this->ReaderFactory = 0;

  this->WriterFactory->Delete();
  this->WriterFactory = 0;

  delete this->PXMStorage;
  this->PXMStorage = NULL;
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMProxyManager::GetProxyManager()
{
  if (!vtkSMProxyManager::Singleton)
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
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm ? vtkSMSession::SafeDownCast(pm->GetSession(this->PXMStorage->ActiveSessionID)) : NULL;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::ConnectionsUpdated(vtkObject*, unsigned long eventid, void* calldata)
{
  // Callback called when a new session is registered. Update active session
  // accordingly.
  if (eventid == vtkCommand::ConnectionCreatedEvent)
  {
    // A new session always becomes active.
    vtkIdType sid = *(reinterpret_cast<vtkIdType*>(calldata));
    this->SetActiveSession(sid);
  }
  else if (eventid == vtkCommand::ConnectionClosedEvent)
  {
    vtkIdType sid = *(reinterpret_cast<vtkIdType*>(calldata));
    if (this->PXMStorage->ActiveSessionID == sid)
    {
      vtkIdType newSID = 0;

      // Find another session, if available, and make that active.
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      vtkSessionIterator* iter = pm->NewSessionIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (iter->GetCurrentSession() != NULL && iter->GetCurrentSessionId() != sid)
        {
          newSID = iter->GetCurrentSessionId();
          break;
        }
      }
      iter->Delete();

      this->SetActiveSession(newSID);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::SetActiveSession(vtkIdType sid)
{
  if (this->PXMStorage->ActiveSessionID != sid)
  {
    this->PXMStorage->ActiveSessionID = sid;
    vtkSMSession* session = this->GetActiveSession();
    this->InvokeEvent(vtkSMProxyManager::ActiveSessionChanged, session);
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::SetActiveSession(vtkSMSession* session)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType sid = session ? pm->GetSessionID(session) : 0;
  this->SetActiveSession(sid);
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMProxyManager::GetActiveSessionProxyManager()
{
  return this->GetSessionProxyManager(this->GetActiveSession());
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMProxyManager::GetSessionProxyManager(vtkSMSession* session)
{
  return session ? session->GetSessionProxyManager() : NULL;
}

//---------------------------------------------------------------------------

void vtkSMProxyManager::UpdateProxyDefinitions()
{
  if (!this->BlockProxyDefinitionUpdates)
  {
    this->ReaderFactory->UpdateAvailableReaders();
    this->WriterFactory->UpdateAvailableWriters();
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UndoStackBuilder: " << this->UndoStackBuilder << endl;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(
  const char* groupName, const char* proxyName, const char* subProxyName)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
  {
    return pxm->NewProxy(groupName, proxyName, subProxyName);
  }
  vtkErrorMacro("No active session found.");
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterProxy(const char* groupname, const char* name, vtkSMProxy* proxy)
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
void vtkSMProxyManager::UnRegisterProxy(const char* groupname, const char* name, vtkSMProxy* proxy)
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

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetProxyName(const char* groupname, unsigned int idx)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
  {
    return pxm->GetProxyName(groupname, idx);
  }
  else
  {
    vtkErrorMacro("No active session found.");
  }
  return NULL;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetProxyName(const char* groupname, vtkSMProxy* pxy)
{
  if (vtkSMSessionProxyManager* pxm = this->GetActiveSessionProxyManager())
  {
    return pxm->GetProxyName(groupname, pxy);
  }
  else
  {
    vtkErrorMacro("No active session found.");
  }
  return NULL;
}

//---------------------------------------------------------------------------
std::string vtkSMProxyManager::GetUniqueProxyName(
  const char* groupname, const char* prefix, bool alwaysAppend)
{
  std::vector<vtkSMSessionProxyManager*> pxms;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSessionIterator* iter = pm->NewSessionIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (auto session = vtkSMSession::SafeDownCast(iter->GetCurrentSession()))
    {
      pxms.push_back(session->GetSessionProxyManager());
    }
  }
  iter->Delete();

  if (!groupname || !prefix || pxms.size() == 0)
  {
    return std::string();
  }

  if (pxms.size() == 1)
  {
    return pxms[0]->GetUniqueProxyName(groupname, prefix, alwaysAppend);
  }

  // find unique name across all sessions.
  std::set<std::string> existingNames;

  vtkNew<vtkSMProxyIterator> piter;
  for (auto pxm : pxms)
  {
    piter->SetSessionProxyManager(pxm);
    for (piter->Begin(groupname); !piter->IsAtEnd(); piter->Next())
    {
      existingNames.insert(piter->GetKey());
    }
  }

  if (!alwaysAppend)
  {
    if (existingNames.find(prefix) == existingNames.end())
    {
      return prefix;
    }
  }

  for (int suffix = 1; suffix < VTK_INT_MAX; ++suffix)
  {
    std::ostringstream name_stream;
    name_stream << prefix << suffix;
    if (existingNames.find(name_stream.str()) == existingNames.end())
    {
      return name_stream.str();
    }
  }

  vtkErrorMacro("Failed to come up with a unique name!");
  abort();
}
