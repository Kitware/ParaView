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

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"

#include <vtksys/RegularExpression.hxx>
#include <vtkstd/set>

//*****************************************************************************
//class vtkSMUndoStackObserver : public vtkCommand
//{
//public:
//  static vtkSMUndoStackObserver* New()
//    {
//    return new vtkSMUndoStackObserver;
//    }
  
//  void SetTarget(vtkSMUndoStack* t)
//    {
//    this->Target = t;
//    }

//  virtual void Execute(vtkObject* caller, unsigned long eventid, void* data)
//    {
//    if (this->Target)
//      {
//      this->Target->ExecuteEvent(caller, eventid, data);
//      }
//    }

//private:
//  vtkSMUndoStackObserver()
//    {
//    this->Target = 0;
//    }

//  vtkSMUndoStack* Target;
//};

//*****************************************************************************
// FIXME I broke VisTrail !!!
//*****************************************************************************

vtkStandardNewMacro(vtkSMUndoStack);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
//  this->ClientOnly = 0;

//  this->Observer = vtkSMUndoStackObserver::New();
//  this->Observer->SetTarget(this);

//  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
//  if (pm)
//    {
//    pm->AddObserver(vtkCommand::ConnectionClosedEvent, this->Observer);
//    }
}

//-----------------------------------------------------------------------------
vtkSMUndoStack::~vtkSMUndoStack()
{
//  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
//  if (pm)
//    {
//    pm->RemoveObserver(this->Observer);
//    }

//  this->Observer->SetTarget(NULL);
//  this->Observer->Delete();
}

//-----------------------------------------------------------------------------
//void vtkSMUndoStack::Push(const char* label, vtkUndoSet* set)
//{
  /*
   * FIXME: Until we start supporting multiple client, it really unnecessary to push
   * the elements on the server side.
   */

//  if (!set)
//    {
//    vtkErrorMacro("No set sepecified to Push.");
//    return;
//    }

//  if (!label)
//    {
//    vtkErrorMacro("Label is required.");
//    return;
//    }

//  vtkPVXMLElement* state = set->SaveState(NULL);
//  //if (!this->ClientOnly)
//  //  state->PrintXML();

//  if (true || this->ClientOnly)
//    {
//    vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
//    elem->SetConnectionID(cid);
//    elem->SetUndoRedoManager(this);
//    elem->SetState(state);
//    this->Superclass::Push(label, elem);
//    elem->Delete();
//    }
//  else
//    {
//    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
//    pm->PushUndo(cid, label, state);

//    // For now, call this method direcly, eventually, PM may fire and event or something
//    // when the undo stack on any connection is updated.
//    this->PushUndoConnection(label, cid);
//    }

//  state->Delete();
//}

//-----------------------------------------------------------------------------
// TODO: Eventually this method will be called as an effect of the PM telling the client
// that something has been pushed on the server side undo stack.
// As a consequence each client will update their undo stack status. Note,
// only the status is updated, the actual undo state is not sent to the client
// until it requests it. Ofcourse, this part is still not implemnted. For now,
// multiple clients are not supported.
//void vtkSMUndoStack::PushUndoConnection(const char* label,
//  vtkIdType id)
//{
//  vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
//  elem->SetConnectionID(id);
//  elem->SetUndoRedoManager(this);
//  this->Superclass::Push(label, elem);
//  elem->Delete();
//}

//-----------------------------------------------------------------------------
//int vtkSMUndoStack::ProcessUndo(vtkIdType id, vtkPVXMLElement* root)
//{
//  if (!this->StateLoader)
//    {
//    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
//    this->SetStateLoader(sl);
//    sl->Delete();
//    }

//  vtkSMIdBasedProxyLocator* locator = vtkSMIdBasedProxyLocator::New();
//  locator->SetConnectionID(id);
//  locator->SetDeserializer(this->StateLoader);
//  vtkUndoSet* undo = this->StateLoader->LoadUndoRedoSet(root, locator);
//  int status = 0;
//  if (undo)
//    {
//    status = undo->Undo();
//    undo->Delete();
//    // Update modified proxies.
//    // vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
//    }
//  locator->Delete();
//  return status;
//}

//-----------------------------------------------------------------------------
//int vtkSMUndoStack::ProcessRedo(vtkIdType id, vtkPVXMLElement* root)
//{
//  if (!this->StateLoader)
//    {
//    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
//    this->SetStateLoader(sl);
//    sl->Delete();
//    }

//  vtkSMIdBasedProxyLocator* locator = vtkSMIdBasedProxyLocator::New();
//  locator->SetConnectionID(id);
//  locator->SetDeserializer(this->StateLoader);
//  vtkUndoSet* redo = this->StateLoader->LoadUndoRedoSet(root, locator);
//  int status = 0;
//  if (redo)
//    {
//    status = redo->Redo();
//    redo->Delete();
//    // Update modified proxies.
//    // vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
//    }
//  locator->Delete();
//  return status;
//}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::Undo()
{
  if (!this->CanUndo())
    {
    vtkErrorMacro("Cannot undo. Nothing on undo stack.");
    return 0;
    }

  // Hold remote objects refs while the UndoSet is processing
  vtkSmartPointer<vtkCollection> remoteObjectsCollection;
  remoteObjectsCollection = vtkSmartPointer<vtkCollection>::New();
  this->FillWithRemoteObjects(this->GetNextUndoSet(), remoteObjectsCollection.GetPointer());

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

  // Hold remote objects refs while the UndoSet is processing
  vtkSmartPointer<vtkCollection> remoteObjectsCollection;
  remoteObjectsCollection = vtkSmartPointer<vtkCollection>::New();
  this->FillWithRemoteObjects(this->GetNextRedoSet(), remoteObjectsCollection.GetPointer());

  return this->Superclass::Redo();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::FillWithRemoteObjects( vtkUndoSet *undoSet,
                                            vtkCollection *collection)
{
  if(!undoSet || !collection)
    return;

  int max = undoSet->GetNumberOfElements();
  vtkstd::set<vtkSmartPointer<vtkSMSession> > sessions;
  for (int cc=0; cc < max; ++cc)
    {
    vtkSMUndoElement* elem =
        vtkSMUndoElement::SafeDownCast(undoSet->GetElement(cc));
    sessions.insert(elem->GetSession());
    }
  vtkstd::set<vtkSmartPointer<vtkSMSession> >::iterator iter = sessions.begin();
  while(iter != sessions.end())
    {
    iter->GetPointer()->GetAllRemoteObjects(collection);
    iter++;
    }

//  cout << "List of availabe remote objects: ";
//  for(int i=0;i<collection->GetNumberOfItems();i++)
//    {
//    vtkSMRemoteObject *obj = vtkSMRemoteObject::SafeDownCast(collection->GetItemAsObject(i));
//    cout << " " << obj->GetGlobalID();
//    }
//  cout << endl;
}

//-----------------------------------------------------------------------------
//void vtkSMUndoStack::ExecuteEvent(vtkObject* vtkNotUsed(caller),
//  unsigned long eventid, void* data)
//{
//  if (eventid == vtkCommand::ConnectionClosedEvent)
//    {
//    //this->OnConnectionClosed(*reinterpret_cast<vtkIdType*>(data));
//    }
//}

//-----------------------------------------------------------------------------
//void vtkSMUndoStack::OnConnectionClosed(vtkIdType cid)
//{
//  // Connection closed, remove undo/redo elements belonging to the connection.
//  vtkUndoStackInternal::VectorOfElements::iterator iter;
//  vtkUndoStackInternal::VectorOfElements tempStack;
  
//  for (iter = this->Internal->UndoStack.begin();
//    iter != this->Internal->UndoStack.end(); ++iter)
//    {
//    vtkSMUndoStackUndoSet* set = vtkSMUndoStackUndoSet::SafeDownCast(
//      iter->UndoSet);
//    if (!set || set->GetConnectionID() != cid)
//      {
//      tempStack.push_back(*iter);
//      }
//    }
//  this->Internal->UndoStack.clear();
//  this->Internal->UndoStack.insert(this->Internal->UndoStack.begin(),
//    tempStack.begin(), tempStack.end());

//  tempStack.clear();
//  for (iter = this->Internal->RedoStack.begin();
//    iter != this->Internal->RedoStack.end(); ++iter)
//    {
//     vtkSMUndoStackUndoSet* set = vtkSMUndoStackUndoSet::SafeDownCast(
//      iter->UndoSet);
//    if (!set || set->GetConnectionID() != cid)
//      {
//      tempStack.push_back(*iter);
//      }
//    }
//  this->Internal->RedoStack.clear();
//  this->Internal->RedoStack.insert(this->Internal->RedoStack.begin(),
//    tempStack.begin(), tempStack.end());
//  this->Modified();
//}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //os << indent << "ClientOnly: " << this->ClientOnly << endl;
}

//-----------------------------------------------------------------------------
//begin vistrails
vtkUndoSet* vtkSMUndoStack::getLastUndoSet() {
//  if (this->Internal->UndoStack.empty())
//    {
//    return NULL;
//    }
//  vtkSMUndoStackUndoSet *us =
//    vtkSMUndoStackUndoSet::SafeDownCast(
//      this->Internal->UndoStack.back().UndoSet.GetPointer());
//  return us->getLastUndoSet();
  return NULL;
}
//end vistrails
