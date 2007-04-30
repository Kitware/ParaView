/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInteractionUndoStackBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInteractionUndoStackBuilder.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMUndoStack.h"
#include "vtkUndoSet.h"

//-----------------------------------------------------------------------------
class vtkSMInteractionUndoStackBuilderObserver : public vtkCommand
{
public:
  static vtkSMInteractionUndoStackBuilderObserver* New()
    { return new vtkSMInteractionUndoStackBuilderObserver(); }

  void SetTarget(vtkSMInteractionUndoStackBuilder* target)
    {
    this->Target = target;
    }

  virtual void Execute(vtkObject* caller, unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, event, data);
      }
    }

protected:
  vtkSMInteractionUndoStackBuilderObserver()
    {
    this->Target = 0;
    }
  vtkSMInteractionUndoStackBuilder* Target;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMInteractionUndoStackBuilder);
vtkCxxRevisionMacro(vtkSMInteractionUndoStackBuilder, "1.3");
vtkCxxSetObjectMacro(vtkSMInteractionUndoStackBuilder, UndoStack, vtkSMUndoStack);

//-----------------------------------------------------------------------------
vtkSMInteractionUndoStackBuilder::vtkSMInteractionUndoStackBuilder()
{
  this->RenderModule = 0;
  this->UndoStack = 0;

  vtkSMInteractionUndoStackBuilderObserver * observer =
    vtkSMInteractionUndoStackBuilderObserver::New();

  observer->SetTarget(this);
  this->Observer = observer;

  this->UndoSet = vtkUndoSet::New();
}

//-----------------------------------------------------------------------------
vtkSMInteractionUndoStackBuilder::~vtkSMInteractionUndoStackBuilder()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->SetRenderModule(0);
  this->SetUndoStack(0);
  this->UndoSet->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::SetRenderModule(
  vtkSMRenderModuleProxy* renModule)
{
  if (this->RenderModule)
    {
    // Remove old interactors.
    vtkRenderWindowInteractor* interactor = this->RenderModule->GetInteractor();
    interactor->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(RenderModule, vtkSMRenderModuleProxy, renModule);
  if (this->RenderModule)
    {
    vtkRenderWindowInteractor* interactor = this->RenderModule->GetInteractor();
    interactor->AddObserver(vtkCommand::StartInteractionEvent,
      this->Observer);
    interactor->AddObserver(vtkCommand::EndInteractionEvent,
      this->Observer, 100);
    }
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::ExecuteEvent(
  vtkObject* vtkNotUsed(caller), unsigned long event, void* vtkNotUsed(data))
{
  switch (event)
    {
  case vtkCommand::StartInteractionEvent:
    this->StartInteraction();
    break;

  case vtkCommand::EndInteractionEvent:
    this->EndInteraction();
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::StartInteraction()
{
  // Interaction began -- get camera properties and save them.
  this->RenderModule->SynchronizeCameraProperties();
  this->UndoSet->RemoveAllElements();

  this->PropertyModified("CameraPosition");
  this->PropertyModified("CameraFocalPoint");
  this->PropertyModified("CameraViewUp");
  this->PropertyModified("CameraClippingRange");
  this->PropertyModified("CenterOfRotation");
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::EndInteraction()
{
  // Interaction end -- get final camera properties
  // Then push undo set on stack.

  if (this->UndoSet->GetNumberOfElements() == 0)
    {
    return;
    }

  this->RenderModule->SynchronizeCameraProperties();
  this->PropertyModified("CameraPosition");
  this->PropertyModified("CameraFocalPoint");
  this->PropertyModified("CameraViewUp");
  this->PropertyModified("CameraClippingRange");
  this->PropertyModified("CenterOfRotation");

  if (this->UndoStack)
    {
    this->UndoStack->Push(this->RenderModule->GetConnectionID(),
      "Interaction", this->UndoSet);
    }
  else
    {
    vtkWarningMacro("No UndoStack set.");
    }
  this->UndoSet->RemoveAllElements();
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::PropertyModified(const char* pname)
{
  vtkSMPropertyModificationUndoElement* elem =
    vtkSMPropertyModificationUndoElement::New();
  elem->SetConnectionID(this->RenderModule->GetConnectionID());
  elem->ModifiedProperty(this->RenderModule, pname);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::Clear()
{
  this->UndoSet->RemoveAllElements();
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderModule: " << this->RenderModule << endl;
  os << indent << "UndoStack: " << this->UndoStack << endl;
}
