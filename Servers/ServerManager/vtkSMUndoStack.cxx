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
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIdBasedProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMUndoRedoStateLoader.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"

#include <vtksys/RegularExpression.hxx>

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

private:
  vtkSMUndoStackObserver()
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
    int status=0;
    vtkPVXMLElement* state;
    if (this->State)
      {
      state = this->State;
      state->Register(this);
      }
    else
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      state = pm->NewNextUndo(this->ConnectionID);
      }
    if (state)
      {
      status = this->UndoRedoManager->ProcessUndo(this->ConnectionID, state);    
      state->Delete();
      }
    return status;
    }
  
  virtual int Redo() 
    {
    int status = 0;
    vtkPVXMLElement* state;
    if (this->State)
      {
      state = this->State;
      state->Register(this);
      }
    else
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      state = pm->NewNextRedo(this->ConnectionID);
      }
    if (state)
      {
      status = this->UndoRedoManager->ProcessRedo(this->ConnectionID, state);    
      state->Delete();
      }
    return status;
    }

  //begin vistrails
  vtkUndoSet* getLastUndoSet()
  {
    vtkPVXMLElement* state;
    if (this->State)
      {
      state = this->State;
      state->Register(this);
      }
    else
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      state = pm->NewNextUndo(this->ConnectionID);
      }
    if (state) {
      if (!this->UndoRedoManager->GetStateLoader())
      {
        vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
        this->UndoRedoManager->SetStateLoader(sl);
        sl->Delete();
      }


    vtkSMIdBasedProxyLocator* locator = vtkSMIdBasedProxyLocator::New();
    locator->SetConnectionID(this->ConnectionID);
    locator->SetDeserializer(this->UndoRedoManager->StateLoader);
    vtkUndoSet* undo = this->UndoRedoManager->StateLoader->LoadUndoRedoSet(state, locator);
    locator->Delete();

      //this->UndoRedoManager->GetStateLoader()->ClearCreatedProxies();
    state->Delete();

      return undo;
    }
    return NULL;

  }
//end vistrails

  void SetConnectionID(vtkIdType id)
    {
    this->ConnectionID = id;
    }
  
  vtkIdType GetConnectionID() 
    { 
    return this->ConnectionID; 
    }

  void SetUndoRedoManager(vtkSMUndoStack* r)
    {
    this->UndoRedoManager = r;
    }

  void SetState(vtkPVXMLElement* elem)
    {
    this->State = elem;
    }

protected:
  vtkSMUndoStackUndoSet() 
    {
    this->ConnectionID = vtkProcessModuleConnectionManager::GetNullConnectionID();
    this->UndoRedoManager = 0;
    };
  ~vtkSMUndoStackUndoSet(){ };

  vtkIdType ConnectionID;
  vtkSMUndoStack* UndoRedoManager;

  // State is set for Client side only elements. If state is NULL, then and then 
  // alone an attempt is made to obtain the state from the server.
  vtkSmartPointer<vtkPVXMLElement> State;
private:
  vtkSMUndoStackUndoSet(const vtkSMUndoStackUndoSet&);
  void operator=(const vtkSMUndoStackUndoSet&);
};

vtkStandardNewMacro(vtkSMUndoStackUndoSet);
vtkCxxRevisionMacro(vtkSMUndoStackUndoSet, "1.18");
//*****************************************************************************

vtkStandardNewMacro(vtkSMUndoStack);
vtkCxxRevisionMacro(vtkSMUndoStack, "1.18");
vtkCxxSetObjectMacro(vtkSMUndoStack, StateLoader, vtkSMUndoRedoStateLoader);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
  this->ClientOnly = 0;
  this->StateLoader = NULL;

  this->Observer = vtkSMUndoStackObserver::New();
  this->Observer->SetTarget(this);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm)
    {
    pm->AddObserver(vtkCommand::ConnectionClosedEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
vtkSMUndoStack::~vtkSMUndoStack()
{
  this->SetStateLoader(NULL);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm)
    {
    pm->RemoveObserver(this->Observer);
    }

  this->Observer->SetTarget(NULL);
  this->Observer->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::Push(vtkIdType cid, const char* label, vtkUndoSet* set)
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
  
  vtkPVXMLElement* state = set->SaveState(NULL);
  //if (!this->ClientOnly)
  //  state->PrintXML();

  /*
   * FIXME: Until we start supporting multiple client, it really unnecessary to push
   * the elements on the server side. 
   */
  if (true || this->ClientOnly)
    {
    vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
    elem->SetConnectionID(cid);
    elem->SetUndoRedoManager(this);
    elem->SetState(state);
    this->Superclass::Push(label, elem);
    elem->Delete();
    }
  else
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->PushUndo(cid, label, state);

    // For now, call this method direcly, eventually, PM may fire and event or something
    // when the undo stack on any connection is updated.
    this->PushUndoConnection(label, cid); 
    }

  state->Delete();
}

//-----------------------------------------------------------------------------
// TODO: Eventually this method will be called as an effect of the PM telling the client
// that something has been pushed on the server side undo stack.
// As a consequence each client will update their undo stack status. Note,
// only the status is updated, the actual undo state is not sent to the client
// until it requests it. Ofcourse, this part is still not implemnted. For now,
// multiple clients are not supported.
void vtkSMUndoStack::PushUndoConnection(const char* label, 
  vtkIdType id)
{
  vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
  elem->SetConnectionID(id);
  elem->SetUndoRedoManager(this);
  this->Superclass::Push(label, elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::ProcessUndo(vtkIdType id, vtkPVXMLElement* root)
{
  if (!this->StateLoader)
    {
    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
    this->SetStateLoader(sl);
    sl->Delete();
    }

  vtkSMIdBasedProxyLocator* locator = vtkSMIdBasedProxyLocator::New();
  locator->SetConnectionID(id);
  locator->SetDeserializer(this->StateLoader);
  vtkUndoSet* undo = this->StateLoader->LoadUndoRedoSet(root, locator);
  int status = 0;
  if (undo)
    {
    status = undo->Undo();
    undo->Delete();
    // Update modified proxies.
    // vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    }
  locator->Delete();
  return status;
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::ProcessRedo(vtkIdType id, vtkPVXMLElement* root)
{
  if (!this->StateLoader)
    {
    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
    this->SetStateLoader(sl);
    sl->Delete();
    }

  vtkSMIdBasedProxyLocator* locator = vtkSMIdBasedProxyLocator::New();
  locator->SetConnectionID(id);
  locator->SetDeserializer(this->StateLoader);
  vtkUndoSet* redo = this->StateLoader->LoadUndoRedoSet(root, locator);
  int status = 0;
  if (redo)
    {
    status = redo->Redo();
    redo->Delete();
    // Update modified proxies.
    // vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    }
  locator->Delete();
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
void vtkSMUndoStack::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid, void* data)
{
  if (eventid == vtkCommand::ConnectionClosedEvent)
    {
    this->OnConnectionClosed(*reinterpret_cast<vtkIdType*>(data));
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnConnectionClosed(vtkIdType cid)
{
  // Connection closed, remove undo/redo elements belonging to the connection.
  vtkUndoStackInternal::VectorOfElements::iterator iter;
  vtkUndoStackInternal::VectorOfElements tempStack;
  
  for (iter = this->Internal->UndoStack.begin();
    iter != this->Internal->UndoStack.end(); ++iter)
    {
    vtkSMUndoStackUndoSet* set = vtkSMUndoStackUndoSet::SafeDownCast(
      iter->UndoSet);
    if (!set || set->GetConnectionID() != cid)
      {
      tempStack.push_back(*iter);
      }
    }
  this->Internal->UndoStack.clear();
  this->Internal->UndoStack.insert(this->Internal->UndoStack.begin(),
    tempStack.begin(), tempStack.end());

  tempStack.clear();
  for (iter = this->Internal->RedoStack.begin();
    iter != this->Internal->RedoStack.end(); ++iter)
    {
     vtkSMUndoStackUndoSet* set = vtkSMUndoStackUndoSet::SafeDownCast(
      iter->UndoSet);
    if (!set || set->GetConnectionID() != cid)
      {
      tempStack.push_back(*iter);
      }
    }
  this->Internal->RedoStack.clear();
  this->Internal->RedoStack.insert(this->Internal->RedoStack.begin(),
    tempStack.begin(), tempStack.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StateLoader: " << this->StateLoader << endl;
  os << indent << "ClientOnly: " << this->ClientOnly << endl;
}

//-----------------------------------------------------------------------------
//begin vistrails
vtkUndoSet* vtkSMUndoStack::getLastUndoSet() {
  if (this->Internal->UndoStack.empty())
    {
    return NULL;
    }
  vtkSMUndoStackUndoSet *us = 
    vtkSMUndoStackUndoSet::SafeDownCast(
      this->Internal->UndoStack.back().UndoSet.GetPointer());
  return us->getLastUndoSet();
}
//end vistrails

