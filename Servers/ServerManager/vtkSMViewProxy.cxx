/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxy.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
class vtkSMViewProxy::Command : public vtkCommand
{
public:
  static Command* New() {  return new Command(); }
  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ProcessEvents(caller, eventId, callData);
      }
    }
  void SetTarget(vtkSMViewProxy* t)
    {
    this->Target = t;
    }
private:
  Command() { this->Target = 0; }
  vtkSMViewProxy* Target;
};

vtkStandardNewMacro(vtkSMViewProxy);
vtkCxxRevisionMacro(vtkSMViewProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
{
  this->Representations = vtkCollection::New();
  this->Observer = vtkSMViewProxy::Command::New();
  this->Observer->SetTarget(this);
}

//----------------------------------------------------------------------------
vtkSMViewProxy::~vtkSMViewProxy()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->RemoveAllRepresentations();
  this->Representations->Delete();
}

//----------------------------------------------------------------------------
vtkCommand* vtkSMViewProxy::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::AddRepresentation(vtkSMRepresentationProxy* repr)
{
  if (repr && !this->Representations->IsItemPresent(repr))
    {
    if (repr->AddToView(this))
      {
      this->AddRepresentationInternal(repr);
      }
    else
      {
      vtkErrorMacro(<< repr->GetClassName() << " cannot be added to view "
        << "of type " << this->GetClassName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::AddRepresentationInternal(vtkSMRepresentationProxy* repr)
{
  this->Representations->AddItem(repr);
  // repr->AddObserver(vtkCommand::SelectionChanged, this->Observer);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* repr)
{
  if (repr)
    {
    repr->RemoveFromView(this);
    this->RemoveRepresentationInternal(repr);
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RemoveRepresentationInternal(vtkSMRepresentationProxy* repr)
{
  this->Representations->RemoveItem(repr);
  // repr->RemoveObserver(this->Observer);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RemoveAllRepresentations()
{
  while (this->Representations->GetNumberOfItems() > 0)
    {
    this->RemoveRepresentation(
      vtkSMRepresentationProxy::SafeDownCast(
        this->Representations->GetItemAsObject(0)));
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::BeginStillRender()
{
  int interactive = 0;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);

  vtkTimerLog::MarkStartEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::EndStillRender()
{
  vtkTimerLog::MarkEndEvent("Still Render");

  int interactive = 0;
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::BeginInteractiveRender()
{
  int interactive = 1;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);

  vtkTimerLog::MarkStartEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::EndInteractiveRender()
{
  vtkTimerLog::MarkEndEvent("Interactive Render");

  int interactive = 1;
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::StillRender()
{
  // Ensure that all representation pipelines are updated.
  this->UpdateAllRepresentations();

  this->BeginStillRender();
  this->PerformRender();
  this->EndStillRender();
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InteractiveRender()
{
  // Ensure that all representation pipelines are updated.
  this->UpdateAllRepresentations();

  this->BeginInteractiveRender();
  this->PerformRender();
  this->EndInteractiveRender();
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::UpdateAllRepresentations()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Representations->NewIterator());

  // We are calling SendPrepareProgress-SendCleanupPendingProgress
  // only if any representation is indeed going to update. 
  // I wonder of this condition is required and we can;t always
  // called these methods. But since there's minimal effort in calling 
  // these conditionally, I am letting it be (erring on the safer side).
  bool enable_progress = false;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr->GetVisibility())
      {
      // Invisible representations are not updated.
      continue;
      }

    if (!enable_progress && repr->UpdateRequired())
      {
      // If a representation required an update, than it implies that the
      // update will result in progress events. We don't to ignore those
      // progress events, hence we enable progress handling.
      pm->SendPrepareProgress(this->ConnectionID,
        vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER);
      enable_progress = true;
      }

    repr->Update(this);
    }

  if (enable_progress)
    {
    pm->SendCleanupPendingProgress(this->ConnectionID);
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData)
{
  (void)caller;
  (void)eventId;
  (void)callData;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


