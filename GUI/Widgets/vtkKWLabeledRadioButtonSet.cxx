/*=========================================================================

  Module:    vtkKWLabeledRadioButtonSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledRadioButtonSet.h"

#include "vtkKWLabel.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledRadioButtonSet);
vtkCxxRevisionMacro(vtkKWLabeledRadioButtonSet, "1.8");

int vtkKWLabeledRadioButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledRadioButtonSet::vtkKWLabeledRadioButtonSet()
{
  this->CommandFunction = vtkKWLabeledRadioButtonSetCommand;

  this->PackHorizontally = 0;

  this->RadioButtonSet = vtkKWRadioButtonSet::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledRadioButtonSet::~vtkKWLabeledRadioButtonSet()
{
  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->Delete();
    this->RadioButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledRadioButtonSet already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the radiobutton set

  this->RadioButtonSet->SetParent(this);
  this->RadioButtonSet->Create(app, 0);

  // Pack the label and the checkbutton

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->RadioButtonSet->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    tk_cmd << "pack ";
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << this->GetLabel()->GetWidgetName() << " ";
      }
    tk_cmd << this->RadioButtonSet->GetWidgetName() 
           << " -side left -anchor nw" << endl;
    }
  else
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -side top -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->RadioButtonSet->GetWidgetName() 
           << " -side top -anchor nw -padx 10" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::SetPackHorizontally(int _arg)
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
void vtkKWLabeledRadioButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RadioButtonSet: " << this->RadioButtonSet << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}

