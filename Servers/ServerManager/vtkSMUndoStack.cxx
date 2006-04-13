/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStack.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoStack.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStack.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMUndoRedoStateLoader.h"

//*****************************************************************************
class vtkSMUndoStackObserver : public vtkCommand
{
public:
  static vtkSMUndoStackObserver* New()
    {
    return new vtkSMUndoStackObserver;
    }
  void SetTarget(vtkSMUndoStack* t)
    {
    this->Target = t;
    }
  virtual void Execute(vtkObject* caller, unsigned long eventid, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventid, data);
      }
    }
protected:
  vtkSMUndoStackObserver()
    {
    this->Target = 0;
    }
  ~vtkSMUndoStackObserver()
    {
    this->Target = 0;
    }
  vtkSMUndoStack* Target;
};
//*****************************************************************************
class vtkSMUndoStackUndoSet : public vtkUndoSet
{
public:
  static vtkSMUndoStackUndoSet* New();
  vtkTypeRevisionMacro(vtkSMUndoStackUndoSet, vtkUndoSet);
 
  virtual int Undo() 
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVXMLElement* state = pm->NewNextUndo(this->ConnectionID);
    if (state)
      {
      return this->UndoRedoManager->ProcessUndo(this->ConnectionID, state);    
      }
    return 0;
    }
  
  virtual int Redo() 
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVXMLElement* state = pm->NewNextRedo(this->ConnectionID);
    if (state)
      {
      return this->UndoRedoManager->ProcessRedo(this->ConnectionID, state);    
      }
    return 0;
    }

  void SetConnectionID(vtkConnectionID id)
    {
    this->ConnectionID = id;
    }
  
  vtkConnectionID GetConnectionID() 
    { 
    return this->ConnectionID; 
    }

  void SetUndoRedoManager(vtkSMUndoStack* r)
    {
    this->UndoRedoManager = r;
    }

protected:
  vtkSMUndoStackUndoSet() 
    {
    this->ConnectionID.ID = 0;
    this->UndoRedoManager = 0;
    };
  ~vtkSMUndoStackUndoSet(){ };

  vtkConnectionID ConnectionID;
  vtkSMUndoStack* UndoRedoManager;
private:
  vtkSMUndoStackUndoSet(const vtkSMUndoStackUndoSet&);
  void operator=(const vtkSMUndoStackUndoSet&);
};

vtkStandardNewMacro(vtkSMUndoStackUndoSet);
vtkCxxRevisionMacro(vtkSMUndoStackUndoSet, "1.1");
//*****************************************************************************

vtkStandardNewMacro(vtkSMUndoStack);
vtkCxxRevisionMacro(vtkSMUndoStack, "1.1");
vtkCxxSetObjectMacro(vtkSMUndoStack, StateLoader, vtkSMUndoRedoStateLoader);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
  this->Observer = vtkSMUndoStackObserver::New();
  this->Observer->SetTarget(this);
  this->ActiveUndoSet = NULL;
  this->ActiveConnectionID.ID = 0;
  this->Label = NULL;
  this->StateLoader = NULL;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("vtkSMUndoStack must be created only"
       << " after the ProxyManager has been created.");
    }
  else
    {
    pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer);
    pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer);
    pxm->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
vtkSMUndoStack::~vtkSMUndoStack()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    pxm->RemoveObserver(this->Observer);
    }
  this->Observer->SetTarget(NULL);
  this->Observer->Delete();
  if (this->ActiveUndoSet)
    {
    this->ActiveUndoSet->Delete();
    this->ActiveUndoSet = NULL;
    }
  this->SetLabel(NULL);
  this->SetStateLoader(NULL);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::BeginUndoSet(vtkConnectionID cid, const char* label)
{
  if (this->ActiveUndoSet)
    {
    vtkErrorMacro("BeginUndoSet cannot be nested. EndUndoSet must be called "
      << "before calling BeginUndoSet again.");
    return;
    }
  this->ActiveUndoSet = vtkUndoSet::New();
  this->SetLabel(label);
  this->ActiveConnectionID = cid;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::EndUndoSet()
{
  if (!this->ActiveUndoSet)
    {
    vtkErrorMacro("BeginUndoSet must be called before calling EndUndoSet.");
    return;
    }

  this->Push(this->ActiveConnectionID, this->Label, this->ActiveUndoSet);
  
  this->ActiveUndoSet->Delete();
  this->ActiveUndoSet = NULL;
  this->SetLabel(NULL);
  this->ActiveConnectionID.ID = 0;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::Push(vtkConnectionID cid, const char* label, vtkUndoSet* set)
{
  if (!set)
    {
    vtkErrorMacro("No set sepecified to Push.");
    return;
    }
  
  if (!label)
    {
    vtkErrorMacro("Label is required.");
    return;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVXMLElement* state = set->SaveState(NULL);
  pm->PushUndo(cid, label, state);
  state->Delete();

  // For now, call this method direcly, eventually, PM may fire and event or something
  // when the undo stack on any connection is updated.
  this->PushUndoConnection(label, cid); 
}

//-----------------------------------------------------------------------------
// TODO: Eventually this method will be called as an effect of the PM telling the client
// that something has been pushed on the server side undo stack.
// As a consequence each client will update their undo stack status. Note,
// only the status is updated, the actual undo state is not sent to the client
// until it requests it. Ofcourse, this part is still not implemnted. For now,
// multiple clients are not supported.
void vtkSMUndoStack::PushUndoConnection(const char* label, 
  vtkConnectionID id)
{
  vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
  elem->SetConnectionID(id);
  elem->SetUndoRedoManager(this);
  this->Superclass::Push(label, elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid,  void* data)
{
  if (!this->ActiveUndoSet)
    {
    return;
    }

  switch (eventid)
    {
  case vtkCommand::RegisterEvent:
    this->OnRegisterProxy(data);
    break;

  case vtkCommand::UnRegisterEvent:
    this->OnUnRegisterProxy(data);
    break;

  case vtkCommand::PropertyModifiedEvent:
    this->OnPropertyModified(data);
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnRegisterProxy(void* data)
{
  vtkSMProxyManager::RegisteredProxyInformation &info =*(static_cast<
    vtkSMProxyManager::RegisteredProxyInformation*>(data));

  vtkSMProxyRegisterUndoElement* elem = vtkSMProxyRegisterUndoElement::New();
  elem->SetConnectionID(this->ActiveConnectionID);
  elem->ProxyToRegister(info.GroupName, info.ProxyName, info.Proxy);
  this->ActiveUndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnUnRegisterProxy(void* data)
{
  vtkSMProxyManager::RegisteredProxyInformation &info =*(static_cast<
    vtkSMProxyManager::RegisteredProxyInformation*>(data));

  vtkSMProxyUnRegisterUndoElement* elem = 
    vtkSMProxyUnRegisterUndoElement::New();
  elem->SetConnectionID(this->ActiveConnectionID);
  elem->ProxyToUnRegister(info.GroupName, info.ProxyName, info.Proxy);
  this->ActiveUndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnPropertyModified(void* data)
{
  vtkSMProxyManager::ModifiedPropertyInformation &info =*(static_cast<
    vtkSMProxyManager::ModifiedPropertyInformation*>(data)); 
  
  vtkSMPropertyModificationUndoElement* elem = 
    vtkSMPropertyModificationUndoElement::New();
  elem->ModifiedProperty(info.Proxy, info.PropertyName);
  this->ActiveUndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::ProcessUndo(vtkConnectionID id, vtkPVXMLElement* root)
{
  if (!this->StateLoader)
    {
    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
    this->SetStateLoader(sl);
    sl->Delete();
    }
  this->StateLoader->SetConnectionID(id); 
  vtkUndoSet* undo = this->StateLoader->LoadUndoRedoSet(root);
  int status = 0;
  if (undo)
    {
    status = undo->Undo();
    undo->Delete();
    // Update modified proxies.
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    }
  return status;
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::ProcessRedo(vtkConnectionID id, vtkPVXMLElement* root)
{
  if (!this->StateLoader)
    {
    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
    this->SetStateLoader(sl);
    sl->Delete();
    }
  this->StateLoader->SetConnectionID(id); 
  vtkUndoSet* redo = this->StateLoader->LoadUndoRedoSet(root);
  int status = 0;
  if (redo)
    {
    status = redo->Redo();
    redo->Delete();
    // Update modified proxies.
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    }
  return status;
}


//-----------------------------------------------------------------------------
int vtkSMUndoStack::Undo()
{
  if (!this->CanUndo())
    {
    vtkErrorMacro("Cannot undo. Nothing on undo stack.");
    return 0;
    }
  return this->Superclass::Undo();
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::Redo()
{
  if (!this->CanRedo())
    {
    vtkErrorMacro("Cannot redo. Nothing on redo stack.");
    return 0;
    }
  return this->Superclass::Redo();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent <<"StateLoader: " << this->StateLoader << endl;
}

