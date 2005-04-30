/*=========================================================================

  Module:    vtkKWCheckButtonWithChangeColor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWCheckButtonWithChangeColor.h"

#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWCheckButtonWithChangeColor);
vtkCxxRevisionMacro(vtkKWCheckButtonWithChangeColor, "1.7");

int vtkKWCheckButtonWithChangeColorCommand(ClientData cd, Tcl_Interp *interp,
                                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCheckButtonWithChangeColor::vtkKWCheckButtonWithChangeColor()
{
  this->CommandFunction = vtkKWCheckButtonWithChangeColorCommand;

  this->CheckButton       = vtkKWCheckButton::New();
  this->ChangeColorButton = vtkKWChangeColorButton::New();

  this->DisableChangeColorButtonWhenNotChecked = 0;
}

//----------------------------------------------------------------------------
vtkKWCheckButtonWithChangeColor::~vtkKWCheckButtonWithChangeColor()
{
  if (this->CheckButton)
    {
    this->CheckButton->Delete();
    this->CheckButton = NULL;
    }

  if (this->ChangeColorButton)
    {
    this->ChangeColorButton->Delete();
    this->ChangeColorButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(
        app, "frame", "-relief flat -bd 0 -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);
  
  // Create the checkbutton. 
  
  this->CheckButton->SetParent(this);
  this->CheckButton->Create(app, "-anchor w");

  // Create the change color button

  this->ChangeColorButton->SetParent(this);
  this->ChangeColorButton->Create(app, "");

  // Pack the checkbutton and the change color button

  this->Pack();

  // Update

  this->Update();
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->CheckButton->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  tk_cmd << "pack " << this->CheckButton->GetWidgetName() 
         << " -side left -anchor w" << endl
         << "pack " << this->ChangeColorButton->GetWidgetName() 
         << " -side left -anchor w -fill x -expand t -padx 2" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // Disable the color change button if not checked

  if (this->DisableChangeColorButtonWhenNotChecked &&
      this->ChangeColorButton && 
      this->CheckButton && this->CheckButton->IsCreated())
    {
    this->ChangeColorButton->SetEnabled(
      this->CheckButton->GetState() ? this->GetEnabled() : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::SetDisableChangeColorButtonWhenNotChecked(
  int _arg)
{
  if (this->DisableChangeColorButtonWhenNotChecked == _arg)
    {
    return;
    }
  this->DisableChangeColorButtonWhenNotChecked = _arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->CheckButton);
  this->PropagateEnableState(this->ChangeColorButton);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " << this->CheckButton << endl;
  os << indent << "ChangeColorButton: " << this->ChangeColorButton << endl;

  os << indent << "DisableChangeColorButtonWhenNotChecked: " 
     << (this->DisableChangeColorButtonWhenNotChecked ? "On" : "Off") << endl;
}

