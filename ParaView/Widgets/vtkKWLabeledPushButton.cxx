/*=========================================================================

  Module:    vtkKWLabeledPushButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledPushButton.h"

#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledPushButton);
vtkCxxRevisionMacro(vtkKWLabeledPushButton, "1.5");

int vtkKWLabeledPushButtonCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledPushButton::vtkKWLabeledPushButton()
{
  this->CommandFunction = vtkKWLabeledPushButtonCommand;

  this->PushButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledPushButton::~vtkKWLabeledPushButton()
{
  if (this->PushButton)
    {
    this->PushButton->Delete();
    this->PushButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPushButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledPushButton already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the push button

  this->PushButton->SetParent(this);
  this->PushButton->Create(app, "");

  // Pack the label and the push button

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledPushButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->ShowLabel)
    {
    tk_cmd << "pack " << this->Label->GetWidgetName() << " -side left" << endl;
    }

  tk_cmd << "pack " << this->PushButton->GetWidgetName() 
         << " -side left -fill x -expand t" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledPushButton::SetPushButtonLabel(const char *text)
{
  if (this->PushButton)
    {
    this->PushButton->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPushButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PushButton)
    {
    this->PushButton->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledPushButton::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->PushButton)
    {
    this->PushButton->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledPushButton::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->PushButton)
    {
    this->PushButton->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPushButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PushButton: " << this->PushButton << endl;
}

