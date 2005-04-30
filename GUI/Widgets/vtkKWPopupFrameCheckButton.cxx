/*=========================================================================

  Module:    vtkKWPopupFrameCheckButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWPopupFrameCheckButton.h"

#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWPopupButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWPopupFrameCheckButton );
vtkCxxRevisionMacro(vtkKWPopupFrameCheckButton, "1.5");

int vtkKWPopupFrameCheckButtonCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPopupFrameCheckButton::vtkKWPopupFrameCheckButton()
{
  this->CommandFunction = vtkKWPopupFrameCheckButtonCommand;

  this->LinkPopupButtonStateToCheckButton = 0;

  // GUI

  this->CheckButton = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWPopupFrameCheckButton::~vtkKWPopupFrameCheckButton()
{
  // GUI

  if (this->CheckButton)
    {
    this->CheckButton->Delete();
    this->CheckButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::Create(vtkKWApplication *app, 
                                        const char *args)
{
  // Create the superclass widgets

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupFrameCheckButton already created");
    return;
    }

  this->Superclass::Create(app, args);

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

  this->CheckButton->Create(app, "");

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
void vtkKWPopupFrameCheckButton::Update()
{
  this->UpdateEnableState();

  if (!this->IsCreated())
    {
    return;
    }

  // Check button (GetCheckButtonState() is overriden is subclasses)

  if (this->CheckButton)
    {
    this->CheckButton->SetState(this->GetCheckButtonState());
    }

  // Disable the popup button if not checked

  if (this->LinkPopupButtonStateToCheckButton && 
      this->PopupButton && 
      this->CheckButton && 
      this->CheckButton->IsCreated())
    {
    this->PopupButton->SetEnabled(
      this->CheckButton->GetState() ? this->GetEnabled() : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::SetLinkPopupButtonStateToCheckButton(
  int _arg)
{
  if (this->LinkPopupButtonStateToCheckButton == _arg)
    {
    return;
    }

  this->LinkPopupButtonStateToCheckButton = _arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::CheckButtonCallback() 
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CheckButton)
    {
    this->CheckButton->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " 
     << this->CheckButton << endl;
  os << indent << "LinkPopupButtonStateToCheckButton: " 
     << (this->LinkPopupButtonStateToCheckButton ? "On" : "Off") << endl;
}
