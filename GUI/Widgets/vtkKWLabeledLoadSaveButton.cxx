/*=========================================================================

  Module:    vtkKWLabeledLoadSaveButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledLoadSaveButton.h"

#include "vtkKWLabel.h"
#include "vtkKWLoadSaveButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledLoadSaveButton);
vtkCxxRevisionMacro(vtkKWLabeledLoadSaveButton, "1.2");

int vtkKWLabeledLoadSaveButtonCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledLoadSaveButton::vtkKWLabeledLoadSaveButton()
{
  this->CommandFunction = vtkKWLabeledLoadSaveButtonCommand;

  this->PackLabelLast = 0;

  this->LoadSaveButton = vtkKWLoadSaveButton::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledLoadSaveButton::~vtkKWLabeledLoadSaveButton()
{
  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->Delete();
    this->LoadSaveButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledLoadSaveButton already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the push button

  this->LoadSaveButton->SetParent(this);
  this->LoadSaveButton->Create(app, "");

  // Pack the label and the push button

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackLabelLast)
    {
    tk_cmd << "pack " << this->LoadSaveButton->GetWidgetName() 
           << " -side left -fill x" << endl;
    }

  if (this->ShowLabel)
    {
    tk_cmd << "pack " << this->Label->GetWidgetName() << " -side left" << endl;
    }

  if (!this->PackLabelLast)
    {
    tk_cmd << "pack " << this->LoadSaveButton->GetWidgetName() 
           << " -side left -fill x" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetLoadSaveButtonLabel(const char *text)
{
  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
char* vtkKWLabeledLoadSaveButton::GetLoadSaveButtonFileName()
{
  if (this->LoadSaveButton)
    {
    return this->LoadSaveButton->GetFileName();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetPackLabelLast(int arg)
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
void vtkKWLabeledLoadSaveButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LoadSaveButton: " << this->LoadSaveButton << endl;
  os << indent << "PackLabelLast: " 
     << (this->PackLabelLast ? "On" : "Off") << endl;
}

