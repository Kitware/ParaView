/*=========================================================================

  Module:    vtkKWLabeledPopupButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledPopupButton.h"

#include "vtkKWLabel.h"
#include "vtkKWPopupButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledPopupButton);
vtkCxxRevisionMacro(vtkKWLabeledPopupButton, "1.8");

int vtkKWLabeledPopupButtonCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledPopupButton::vtkKWLabeledPopupButton()
{
  this->CommandFunction = vtkKWLabeledPopupButtonCommand;

  this->PackLabelLast = 0;

  this->PopupButton = vtkKWPopupButton::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledPopupButton::~vtkKWLabeledPopupButton()
{
  if (this->PopupButton)
    {
    this->PopupButton->Delete();
    this->PopupButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPopupButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledPopupButton already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the push button

  this->PopupButton->SetParent(this);
  this->PopupButton->Create(app, "");

  // Pack the label and the push button

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledPopupButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->PopupButton->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackLabelLast)
    {
    tk_cmd << "pack " << this->PopupButton->GetWidgetName() 
           << " -side left -fill x -expand t" << endl;
    }

  if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "pack " << this->GetLabel()->GetWidgetName() << " -side left\n";
    }

  if (!this->PackLabelLast)
    {
    tk_cmd << "pack " << this->PopupButton->GetWidgetName() 
           << " -side left -fill x -expand t" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledPopupButton::SetPopupButtonLabel(const char *text)
{
  if (this->PopupButton)
    {
    this->PopupButton->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPopupButton::SetPackLabelLast(int arg)
{
  if (this->PackLabelLast == arg)
    {
    return;
    }

  this->PackLabelLast = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledPopupButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PopupButton)
    {
    this->PopupButton->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledPopupButton::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->PopupButton)
    {
    this->PopupButton->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledPopupButton::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->PopupButton)
    {
    this->PopupButton->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPopupButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PopupButton: " << this->PopupButton << endl;
  os << indent << "PackLabelLast: " 
     << (this->PackLabelLast ? "On" : "Off") << endl;
}

