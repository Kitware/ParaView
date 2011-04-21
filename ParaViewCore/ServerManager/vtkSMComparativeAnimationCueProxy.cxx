/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMComparativeAnimationCueProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMSession.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMComparativeAnimationCueUndoElement.h"

//****************************************************************************
//                         Internal classes
//****************************************************************************
class vtkSMComparativeAnimationCueProxy::vtkInternal
{
public:
  vtkInternal(vtkSMComparativeAnimationCueProxy* parent)
    {
    this->Parent = parent;
    this->UndoStackBuilder = NULL;
    }

  ~vtkInternal()
    {
    this->Parent = NULL;
    this->UndoStackBuilder = NULL;
    if(this->Observable)
      {
      this->Observable->RemoveObserver(this->CallbackID);
      }
    }

  void CreateUndoElement(vtkObject *vtkNotUsed(caller),
                         unsigned long vtkNotUsed(eventId),
                         void *vtkNotUsed(callData))
    {
    // Make sure an UndoStackBuilder is available
    if(this->UndoStackBuilder == NULL)
      {
      this->UndoStackBuilder = this->Parent->GetSession()->GetUndoStackBuilder();
      if(this->UndoStackBuilder == NULL)
        {
        return;
        }
      }

    if(!this->Parent || !this->Parent->GetComparativeAnimationCue())
      {
      return; // This shouldn't be called with that state
      }

    // Create custom undoElement
    vtkSMComparativeAnimationCueUndoElement* elem = vtkSMComparativeAnimationCueUndoElement::New();
    vtkSmartPointer<vtkPVXMLElement> newState = vtkSmartPointer<vtkPVXMLElement>::New();
    this->Parent->SaveXMLState(newState);
    elem->SetXMLStates(this->Parent->GetGlobalID(), this->LastKnownState, newState);
    elem->SetSession(this->Parent->GetSession());
    if(this->UndoStackBuilder->Add(elem))
      {
      this->LastKnownState = vtkSmartPointer<vtkPVXMLElement>::New();
      newState->CopyTo(this->LastKnownState);
      this->UndoStackBuilder->PushToStack();
      }
    elem->Delete();
    }

  void AttachObserver(vtkObject* obj)
    {
    this->Observable = obj;
    this->CallbackID = obj->AddObserver( vtkCommand::StateChangedEvent, this,
                                         &vtkSMComparativeAnimationCueProxy::vtkInternal::CreateUndoElement);
    }

  vtkSMUndoStackBuilder* UndoStackBuilder;
  vtkSMComparativeAnimationCueProxy* Parent;
  vtkWeakPointer<vtkObject> Observable;
  vtkSmartPointer<vtkPVXMLElement> LastKnownState;
  unsigned long CallbackID;
};
//****************************************************************************
vtkStandardNewMacro(vtkSMComparativeAnimationCueProxy);
//----------------------------------------------------------------------------
vtkSMComparativeAnimationCueProxy::vtkSMComparativeAnimationCueProxy()
{
  this->Internals = new vtkInternal(this);
  this->SetLocation(vtkPVSession::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMComparativeAnimationCueProxy::~vtkSMComparativeAnimationCueProxy()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::CreateVTKObjects()
{
  bool needToAttachObserver = !this->ObjectsCreated;
  this->Superclass::CreateVTKObjects();
  if(needToAttachObserver && this->GetClientSideObject())
    {
    this->Internals->AttachObserver(
        vtkObject::SafeDownCast(this->GetClientSideObject()));
    }
}

//----------------------------------------------------------------------------
vtkPVComparativeAnimationCue* vtkSMComparativeAnimationCueProxy::GetCue()
{
  this->CreateVTKObjects();
  return vtkPVComparativeAnimationCue::SafeDownCast(
    this->GetClientSideObject());
}

//----------------------------------------------------------------------------
vtkPVComparativeAnimationCue* vtkSMComparativeAnimationCueProxy::GetComparativeAnimationCue()
{
  return vtkPVComparativeAnimationCue::SafeDownCast(this->GetClientSideObject());
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMComparativeAnimationCueProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator *piter)
{
  return this->GetComparativeAnimationCue()->AppendCommandInfo(
      this->Superclass::SaveXMLState(root, piter));
}

//----------------------------------------------------------------------------
int vtkSMComparativeAnimationCueProxy::LoadXMLState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::LoadXMLState(proxyElement, locator))
    {
    return 0;
    }
  this->GetComparativeAnimationCue()->LoadCommandInfo(proxyElement);
  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
#ifdef FIXME_COLLABORATION

int vtkSMComparativeAnimationCueProxy::RevertState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* vtkNotUsed(locator))
{
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    const char* name =  currentElement->GetName();
    if (name && strcmp(name, "CueCommand") == 0)
      {
      vtkInternals::vtkCueCommand cmd;
      if (cmd.FromXML(currentElement) == false)
        {
        vtkErrorMacro("Error when loading CueCommand.");
        return 0;
        }

      int remove = 0;
      int position = -1;
      currentElement->GetScalarAttribute("remove", &remove);
      currentElement->GetScalarAttribute("position", &position);
      if (remove)
        {
        this->Internals->InsertCommand(cmd, position);
        }
      else
        {
        this->Internals->RemoveCommand(cmd);
        }
      }
    }
  this->Modified();
  return 1;
}
#endif

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
