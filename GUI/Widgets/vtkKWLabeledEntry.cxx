/*=========================================================================

  Module:    vtkKWLabeledEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledEntry.h"

#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledEntry);
vtkCxxRevisionMacro(vtkKWLabeledEntry, "1.18");

int vtkKWLabeledEntryCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledEntry::vtkKWLabeledEntry()
{
  this->CommandFunction = vtkKWLabeledEntryCommand;

  this->Entry = vtkKWEntry::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledEntry::~vtkKWLabeledEntry()
{
  if (this->Entry)
    {
    this->Entry->Delete();
    this->Entry = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledEntry::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledEntry already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the entry

  this->Entry->SetParent(this);
  this->Entry->Create(app, "");

  // Pack the label and the entry

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledEntry::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Entry->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "pack " << this->GetLabel()->GetWidgetName() << " -side left\n";
    }

  tk_cmd << "pack " << this->Entry->GetWidgetName() 
         << " -side left -fill x -expand t" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Entry)
    {
    this->Entry->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledEntry::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Entry)
    {
    this->Entry->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledEntry::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->Entry)
    {
    this->Entry->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Entry: " << this->Entry << endl;
}

