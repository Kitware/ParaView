/*=========================================================================

  Module:    vtkKWLabeledCheckButtonSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledCheckButtonSet.h"

#include "vtkKWCheckButtonSet.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledCheckButtonSet);
vtkCxxRevisionMacro(vtkKWLabeledCheckButtonSet, "1.7");

int vtkKWLabeledCheckButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledCheckButtonSet::vtkKWLabeledCheckButtonSet()
{
  this->CommandFunction = vtkKWLabeledCheckButtonSetCommand;

  this->PackHorizontally = 0;

  this->CheckButtonSet = vtkKWCheckButtonSet::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledCheckButtonSet::~vtkKWLabeledCheckButtonSet()
{
  if (this->CheckButtonSet)
    {
    this->CheckButtonSet->Delete();
    this->CheckButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledCheckButtonSet already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the checkbutton set

  this->CheckButtonSet->SetParent(this);
  this->CheckButtonSet->Create(app, 0);

  // Pack the label and the checkbutton

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    tk_cmd << "pack ";
    if (this->ShowLabel)
      {
      tk_cmd << this->Label->GetWidgetName() << " ";
      }
    tk_cmd << this->CheckButtonSet->GetWidgetName() 
           << " -side left -anchor nw" << endl;
    }
  else
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side top -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->CheckButtonSet->GetWidgetName() 
           << " -side top -anchor nw -padx 10" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CheckButtonSet)
    {
    this->CheckButtonSet->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->CheckButtonSet)
    {
    this->CheckButtonSet->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->CheckButtonSet)
    {
    this->CheckButtonSet->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButtonSet: " << this->CheckButtonSet << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}

