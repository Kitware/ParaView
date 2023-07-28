// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVContextInteractorStyle.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVContextInteractorStyle);
//----------------------------------------------------------------------------
vtkPVContextInteractorStyle::vtkPVContextInteractorStyle() = default;

//----------------------------------------------------------------------------
vtkPVContextInteractorStyle::~vtkPVContextInteractorStyle() = default;
//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnLeftButtonDown()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->Superclass::OnLeftButtonDown();
}

//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnLeftButtonUp()
{
  this->Superclass::OnLeftButtonUp();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnMiddleButtonDown()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->Superclass::OnMiddleButtonDown();
}

//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnMiddleButtonUp()
{
  this->Superclass::OnMiddleButtonUp();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}
//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnRightButtonDown()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->Superclass::OnRightButtonDown();
}

//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnRightButtonUp()
{
  this->Superclass::OnRightButtonUp();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}
//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnMouseWheelForward()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->Superclass::OnMouseWheelForward();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::OnMouseWheelBackward()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->Superclass::OnMouseWheelBackward();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkPVContextInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
