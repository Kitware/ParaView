/*=========================================================================

  Module:    vtkKWLabeledCheckButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledCheckButton.h"

#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledCheckButton);
vtkCxxRevisionMacro(vtkKWLabeledCheckButton, "1.6");

int vtkKWLabeledCheckButtonCommand(ClientData cd, Tcl_Interp *interp,
                                   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledCheckButton::vtkKWLabeledCheckButton()
{
  this->CommandFunction = vtkKWLabeledCheckButtonCommand;

  this->CheckButton = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledCheckButton::~vtkKWLabeledCheckButton()
{
  if (this->CheckButton)
    {
    this->CheckButton->Delete();
    this->CheckButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledCheckButton already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the check button

  this->CheckButton->SetParent(this);
  this->CheckButton->Create(app, "");

  // Pack the label and the push button

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledCheckButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->CheckButton->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "pack " << this->GetLabel()->GetWidgetName() << " -side left\n";
    }

  tk_cmd << "pack " << this->CheckButton->GetWidgetName() 
         << " -side left -fill x -expand t" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CheckButton)
    {
    this->CheckButton->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledCheckButton::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->CheckButton)
    {
    this->CheckButton->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledCheckButton::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->CheckButton)
    {
    this->CheckButton->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " << this->CheckButton << endl;
}

