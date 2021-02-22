/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextInteractorStyle.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
