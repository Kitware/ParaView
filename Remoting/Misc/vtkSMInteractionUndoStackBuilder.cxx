// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMInteractionUndoStackBuilder.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMUndoStack.h"
#include "vtkUndoSet.h"

//-----------------------------------------------------------------------------
class vtkSMInteractionUndoStackBuilderObserver : public vtkCommand
{
public:
  static vtkSMInteractionUndoStackBuilderObserver* New()
  {
    return new vtkSMInteractionUndoStackBuilderObserver();
  }

  void SetTarget(vtkSMInteractionUndoStackBuilder* target) { this->Target = target; }

  void Execute(vtkObject* caller, unsigned long event, void* data) override
  {
    if (this->Target)
    {
      this->Target->ExecuteEvent(caller, event, data);
    }
  }

protected:
  vtkSMInteractionUndoStackBuilderObserver() { this->Target = nullptr; }
  vtkSMInteractionUndoStackBuilder* Target;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMInteractionUndoStackBuilder);
vtkCxxSetObjectMacro(vtkSMInteractionUndoStackBuilder, UndoStack, vtkSMUndoStack);

//-----------------------------------------------------------------------------
vtkSMInteractionUndoStackBuilder::vtkSMInteractionUndoStackBuilder()
{
  this->RenderView = nullptr;
  this->UndoStack = nullptr;

  vtkSMInteractionUndoStackBuilderObserver* observer =
    vtkSMInteractionUndoStackBuilderObserver::New();

  observer->SetTarget(this);
  this->Observer = observer;

  this->UndoSet = vtkUndoSet::New();
}

//-----------------------------------------------------------------------------
vtkSMInteractionUndoStackBuilder::~vtkSMInteractionUndoStackBuilder()
{
  this->Observer->SetTarget(nullptr);
  this->Observer->Delete();

  this->SetRenderView(nullptr);
  this->SetUndoStack(nullptr);
  this->UndoSet->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMInteractionUndoStackBuilder::SetRenderView(vtkSMRenderViewProxy* renView)
{
  if (this->RenderView)
  {
    // Remove old interactors.
    vtkRenderWindowInteractor* interactor = this->RenderView->GetInteractor();
    interactor->RemoveObserver(this->Observer);
  }

  vtkSetObjectBodyMacro(RenderView, vtkSMRenderViewProxy, renView);
  if (this->RenderView)
  {
    vtkRenderWindowInteractor* interactor = this->RenderView->GetInteractor();
    interactor->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
    interactor->AddObserver(vtkCommand::EndInteractionEvent, this->Observer, 100);
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
  // this->RenderView->SynchronizeCameraProperties();
  this->UndoSet->RemoveAllElements();

  this->PropertyModified("CameraPosition");
  this->PropertyModified("CameraFocalPoint");
  this->PropertyModified("CameraViewUp");
  this->PropertyModified("CameraViewAngle");
  this->PropertyModified("CenterOfRotation");
  this->PropertyModified("RotationFactor");
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

  // this->RenderView->SynchronizeCameraProperties();
  this->PropertyModified("CameraPosition");
  this->PropertyModified("CameraFocalPoint");
  this->PropertyModified("CameraViewUp");
  this->PropertyModified("CameraViewAngle");
  this->PropertyModified("CenterOfRotation");
  this->PropertyModified("RotationFactor");

  if (this->UndoStack)
  {
    this->UndoStack->Push("Interaction", this->UndoSet);
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
  vtkSMPropertyModificationUndoElement* elem = vtkSMPropertyModificationUndoElement::New();
  elem->ModifiedProperty(this->RenderView, pname);
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
  os << indent << "RenderView: " << this->RenderView << endl;
  os << indent << "UndoStack: " << this->UndoStack << endl;
}
