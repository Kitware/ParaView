/*=========================================================================

  Module:    vtkKWLabeledText.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledText.h"

#include "vtkKWLabel.h"
#include "vtkKWText.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledText);
vtkCxxRevisionMacro(vtkKWLabeledText, "1.7");

int vtkKWLabeledTextCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledText::vtkKWLabeledText()
{
  this->CommandFunction = vtkKWLabeledTextCommand;

  this->Text = vtkKWText::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledText::~vtkKWLabeledText()
{
  if (this->Text)
    {
    this->Text->Delete();
    this->Text = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledText::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledText already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Let's make the label slightly smaller

  if (this->Label)
    {
    this->Script("%s configure -bd 1", this->Label->GetWidgetName());
    }

  // Create the option menu

  this->Text->SetParent(this);
  this->Text->Create(app, "");

  // Pack the label and the option menu

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledText::Pack()
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
    tk_cmd << "pack " << this->Label->GetWidgetName() 
           << " -anchor nw -pady 0 -ipady 0" << endl;
    }
  tk_cmd << "pack " << this->Text->GetWidgetName() 
         << " -side top -anchor nw -fill both -expand t" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledText::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Text)
    {
    this->Text->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledText::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Text)
    {
    this->Text->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledText::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->Text)
    {
    this->Text->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledText::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Text: " << this->Text << endl;
}

