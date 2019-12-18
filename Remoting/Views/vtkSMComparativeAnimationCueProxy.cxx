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
#include "vtkSMComparativeAnimationCueUndoElement.h"
#include "vtkSMDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMUndoStackBuilder.h"

//****************************************************************************
//                         Internal classes
//****************************************************************************
class vtkSMComparativeAnimationCueProxy::vtkInternal
{
public:
  vtkInternal(vtkSMComparativeAnimationCueProxy* parent) { this->Parent = parent; }

  ~vtkInternal()
  {
    this->Parent = NULL;
    if (this->Observable)
    {
      this->Observable->RemoveObserver(this->CallbackID);
    }
  }

  void CreateUndoElement(
    vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId), void* vtkNotUsed(callData))
  {
    // Make sure an UndoStackBuilder is available
    vtkSMUndoStackBuilder* usb = vtkSMProxyManager::GetProxyManager()->GetUndoStackBuilder();
    if (usb == NULL)
    {
      return;
    }

    if (!this->Parent || !this->Parent->GetComparativeAnimationCue())
    {
      return; // This shouldn't be called with that state
    }

    // Create custom undoElement
    vtkSMComparativeAnimationCueUndoElement* elem = vtkSMComparativeAnimationCueUndoElement::New();
    vtkSmartPointer<vtkPVXMLElement> newState = vtkSmartPointer<vtkPVXMLElement>::New();
    this->Parent->SaveXMLState(newState);
    elem->SetXMLStates(this->Parent->GetGlobalID(), this->LastKnownState, newState);
    elem->SetSession(this->Parent->GetSession());
    if (usb->Add(elem))
    {
      this->LastKnownState = vtkSmartPointer<vtkPVXMLElement>::New();
      newState->CopyTo(this->LastKnownState);
      usb->PushToStack();
    }
    elem->Delete();
  }

  void AttachObserver(vtkObject* obj)
  {
    this->Observable = obj;
    this->CallbackID = obj->AddObserver(vtkCommand::StateChangedEvent, this,
      &vtkSMComparativeAnimationCueProxy::vtkInternal::CreateUndoElement);
  }

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
  if (needToAttachObserver && this->GetClientSideObject())
  {
    this->Internals->AttachObserver(vtkObject::SafeDownCast(this->GetClientSideObject()));
  }
}

//----------------------------------------------------------------------------
vtkPVComparativeAnimationCue* vtkSMComparativeAnimationCueProxy::GetComparativeAnimationCue()
{
  return vtkPVComparativeAnimationCue::SafeDownCast(this->GetClientSideObject());
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMComparativeAnimationCueProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator* piter)
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

#define SAFE_FORWARD(method)                                                                       \
  vtkPVComparativeAnimationCue* cue = this->GetComparativeAnimationCue();                          \
  if (cue == NULL)                                                                                 \
  {                                                                                                \
    vtkWarningMacro("Please call CreateVTKObjects() first.");                                      \
    return;                                                                                        \
  }                                                                                                \
  cue->method

#define SAFE_FORWARD_AND_RETURN(method, retval)                                                    \
  vtkPVComparativeAnimationCue* cue = this->GetComparativeAnimationCue();                          \
  if (cue == NULL)                                                                                 \
  {                                                                                                \
    vtkWarningMacro("Please call CreateVTKObjects() first.");                                      \
    return retval;                                                                                 \
  }                                                                                                \
  return cue->method

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateXRange(int y, double minx, double maxx)
{
  SAFE_FORWARD(UpdateXRange)(y, minx, maxx);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateYRange(int x, double miny, double maxy)
{
  SAFE_FORWARD(UpdateYRange)(x, miny, maxy);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateWholeRange(double mint, double maxt)
{
  SAFE_FORWARD(UpdateWholeRange)(mint, maxt);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateValue(int x, int y, double value)
{
  SAFE_FORWARD(UpdateValue)(x, y, value);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateXRange(
  int y, double* minx, double* maxx, unsigned int numvalues)
{
  SAFE_FORWARD(UpdateXRange)(y, minx, maxx, numvalues);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateYRange(
  int x, double* minx, double* maxx, unsigned int numvalues)
{
  SAFE_FORWARD(UpdateYRange)(x, minx, maxx, numvalues);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateWholeRange(
  double* mint, double* maxt, unsigned int numValues)
{
  SAFE_FORWARD(UpdateWholeRange)(mint, maxt, numValues);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateWholeRange(
  double* mint, double* maxt, unsigned int numValues, bool vertical_first)
{
  SAFE_FORWARD(UpdateWholeRange)(mint, maxt, numValues, vertical_first);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateValue(
  int x, int y, double* value, unsigned int numValues)
{
  SAFE_FORWARD(UpdateValue)(x, y, value, numValues);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::UpdateAnimatedValue(int x, int y, int dx, int dy)
{
  SAFE_FORWARD(UpdateAnimatedValue)(x, y, dx, dy);
  // NOTE: this should not call MarkModified().
}

//----------------------------------------------------------------------------
double* vtkSMComparativeAnimationCueProxy::GetValues(
  int x, int y, int dx, int dy, unsigned int& numValues)
{
  SAFE_FORWARD_AND_RETURN(GetValues, NULL)(x, y, dx, dy, numValues);
}

//----------------------------------------------------------------------------
double vtkSMComparativeAnimationCueProxy::GetValue(int x, int y, int dx, int dy)
{
  SAFE_FORWARD_AND_RETURN(GetValue, 0)(x, y, dx, dy);
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
