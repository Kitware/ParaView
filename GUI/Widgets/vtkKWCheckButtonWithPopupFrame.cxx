/*=========================================================================

  Module:    vtkKWCheckButtonWithPopupFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWCheckButtonWithPopupFrame.h"

#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWPopupButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCheckButtonWithPopupFrame );
vtkCxxRevisionMacro(vtkKWCheckButtonWithPopupFrame, "1.5");

//----------------------------------------------------------------------------
vtkKWCheckButtonWithPopupFrame::vtkKWCheckButtonWithPopupFrame()
{
  this->DisablePopupButtonWhenNotChecked = 0;

  // GUI

  this->CheckButton = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWCheckButtonWithPopupFrame::~vtkKWCheckButtonWithPopupFrame()
{
  // GUI

  if (this->CheckButton)
    {
    this->CheckButton->Delete();
    this->CheckButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithPopupFrame::CreateWidget()
{
  // Create the superclass widgets

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupFrameCheckButton already created");
    return;
    }

  this->Superclass::CreateWidget();

  // --------------------------------------------------------------
  // Annotation visibility

  if (this->PopupMode)
    {
    this->CheckButton->SetParent(this);
    }
  else
    {
    this->CheckButton->SetParent(this->Frame->GetFrame());
    }

  this->CheckButton->Create();

  this->CheckButton->SetCommand(this, "CheckButtonCallback");

  if (this->PopupMode)
    {
    this->Script("pack %s -side left -anchor w",
                 this->CheckButton->GetWidgetName());
    this->Script("pack %s -side left -anchor w -fill x -expand t -padx 2",
                 this->PopupButton->GetWidgetName());
    }
  else
    {
    this->Script("pack %s -side top -padx 2 -anchor nw",
                 this->CheckButton->GetWidgetName());
    }

  // --------------------------------------------------------------
  // Update the GUI according to the Ivars

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithPopupFrame::Update()
{
  this->UpdateEnableState();

  if (!this->IsCreated())
    {
    return;
    }

  // Check button (GetCheckButtonState() is overriden is subclasses)

  if (this->CheckButton)
    {
    this->CheckButton->SetSelectedState(this->GetCheckButtonState());
    }

  // Disable the popup button if not checked

  if (this->DisablePopupButtonWhenNotChecked && 
      this->PopupButton && 
      this->CheckButton && 
      this->CheckButton->IsCreated())
    {
    this->PopupButton->SetEnabled(
      this->CheckButton->GetSelectedState() ? this->GetEnabled() : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithPopupFrame::SetDisablePopupButtonWhenNotChecked(
  int _arg)
{
  if (this->DisablePopupButtonWhenNotChecked == _arg)
    {
    return;
    }

  this->DisablePopupButtonWhenNotChecked = _arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithPopupFrame::CheckButtonCallback(int) 
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithPopupFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CheckButton)
    {
    this->CheckButton->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithPopupFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " 
     << this->CheckButton << endl;
  os << indent << "DisablePopupButtonWhenNotChecked: " 
     << (this->DisablePopupButtonWhenNotChecked ? "On" : "Off") << endl;
}
