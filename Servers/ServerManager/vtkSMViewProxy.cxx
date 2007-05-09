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
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMRepresentationStrategy.h"
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
vtkCxxRevisionMacro(vtkSMViewProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
{
  this->Representations = vtkCollection::New();
  this->Observer = vtkSMViewProxy::Command::New();
  this->Observer->SetTarget(this);
  this->ViewHelper = 0;

  this->GUISize[0] = this->GUISize[1] = 300;
  this->WindowPosition[0] = this->WindowPosition[1] = 0;
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
void vtkSMViewProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  if (!this->ViewHelper)
    {
    this->ViewHelper = this->GetSubProxy("ViewHelper");
    }

  if (this->ViewHelper)
    {
    this->ViewHelper->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    }

  this->Superclass::CreateVTKObjects(numObjects);
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
  if (this->ViewHelper && repr->GetProperty("ViewHelper"))
    {
    // Provide th representation with the helper if it needs it.
    this->Connect(this->ViewHelper, repr, "ViewHelper");
    }

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
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    repr->GetProperty("ViewHelper"));
  if (pp)
    {
    pp->RemoveAllProxies();
    repr->UpdateProperty("ViewHelper");
    }

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

//-----------------------------------------------------------------------------
void vtkSMViewProxy::Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname/*="Input"*/)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty(propertyname));
  if (!pp)
    {
    vtkErrorMacro("Failed to locate property " << propertyname
      << " on " << consumer->GetXMLName());
    return;
    }

  pp->AddProxy(producer);
  consumer->UpdateProperty(propertyname);
}

//----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMViewProxy::NewStrategy(int dataType, int type)
{
  vtkSMRepresentationStrategy* strategy = 
    this->NewStrategyInternal(dataType, type);
  if (strategy && this->ViewHelper)
    {
    // Deliberately not going the proxy property route here since otherwise the
    // strategy becomes a consumer of the ViewHelper and whenever the ViewHelper
    // properties are modified, the startegy will mark it's data invalid etc
    // etc.
    strategy->SetViewHelperProxy(this->ViewHelper);
    }
  return strategy;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GUISize: " 
    << this->GUISize[0] << ", " << this->GUISize[1] << endl;
  os << indent << "WindowPosition: " 
    << this->WindowPosition[0] << ", " << this->WindowPosition[1] << endl;
}


