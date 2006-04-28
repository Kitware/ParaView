/*=========================================================================

  Module:    vtkKWCheckButtonWithChangeColorButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWCheckButtonWithChangeColorButton.h"

#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWCheckButtonWithChangeColorButton);
vtkCxxRevisionMacro(vtkKWCheckButtonWithChangeColorButton, "1.4");

//----------------------------------------------------------------------------
vtkKWCheckButtonWithChangeColorButton::vtkKWCheckButtonWithChangeColorButton()
{
  this->CheckButton       = vtkKWCheckButton::New();
  this->ChangeColorButton = vtkKWChangeColorButton::New();

  this->DisableChangeColorButtonWhenNotChecked = 0;
}

//----------------------------------------------------------------------------
vtkKWCheckButtonWithChangeColorButton::~vtkKWCheckButtonWithChangeColorButton()
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
void vtkKWCheckButtonWithChangeColorButton::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // Create the checkbutton. 
  
  this->CheckButton->SetParent(this);
  this->CheckButton->Create();
  this->CheckButton->SetAnchorToWest();

  // Create the change color button

  this->ChangeColorButton->SetParent(this);
  this->ChangeColorButton->Create();

  // Pack the checkbutton and the change color button

  this->Pack();

  // Update

  this->UpdateVariableBindings();
  this->Update();
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::Pack()
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
         << " -side left -anchor w -fill x -expand t -padx 2 -pady 2" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // Disable the color change button if not checked

  if (this->DisableChangeColorButtonWhenNotChecked &&
      this->ChangeColorButton && 
      this->CheckButton && this->CheckButton->IsCreated())
    {
    this->ChangeColorButton->SetEnabled(
      this->CheckButton->GetSelectedState() ? this->GetEnabled() : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::SetDisableChangeColorButtonWhenNotChecked(
  int _arg)
{
  if (this->DisableChangeColorButtonWhenNotChecked == _arg)
    {
    return;
    }
  this->DisableChangeColorButtonWhenNotChecked = _arg;
  this->Modified();

  this->UpdateVariableBindings();
  this->Update();
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::UpdateVariableBindings()
{
  if (!this->IsCreated() || 
      !this->CheckButton || !this->CheckButton->GetVariableName())
    {
    return;
    }

  // If the variable of the checkbutton is changed, i.e. its state has
  // changed, then call our Update() so that we are given a chance
  // to disable the state of the color change button.
  // Nope, we can't use the checkbutton's Command, it might be used
  // already (most likely).

  vtksys_stl::string cmd(this->GetTclName());
  cmd += " UpdateVariableCallback";
  this->Script("trace remove variable %s {write} {%s}",
               this->CheckButton->GetVariableName(), cmd.c_str());

  if (this->DisableChangeColorButtonWhenNotChecked)
    {
    this->Script("trace add variable %s {write} {%s}",
                 this->CheckButton->GetVariableName(), cmd.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::UpdateVariableCallback(
  const char*, const char*, const char*)
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->CheckButton);
  this->PropagateEnableState(this->ChangeColorButton);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColorButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " << this->CheckButton << endl;
  os << indent << "ChangeColorButton: " << this->ChangeColorButton << endl;

  os << indent << "DisableChangeColorButtonWhenNotChecked: " 
     << (this->DisableChangeColorButtonWhenNotChecked ? "On" : "Off") << endl;
}

