/*=========================================================================

  Module:    vtkKWLabeledPushButtonSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledPushButtonSet.h"

#include "vtkKWLabel.h"
#include "vtkKWPushButtonSet.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledPushButtonSet);
vtkCxxRevisionMacro(vtkKWLabeledPushButtonSet, "1.7");

int vtkKWLabeledPushButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledPushButtonSet::vtkKWLabeledPushButtonSet()
{
  this->CommandFunction = vtkKWLabeledPushButtonSetCommand;

  this->PackHorizontally = 0;

  this->PushButtonSet = vtkKWPushButtonSet::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledPushButtonSet::~vtkKWLabeledPushButtonSet()
{
  if (this->PushButtonSet)
    {
    this->PushButtonSet->Delete();
    this->PushButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPushButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledPushButtonSet already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the checkbutton set

  this->PushButtonSet->SetParent(this);
  this->PushButtonSet->Create(app, 0);

  // Pack the label and the checkbutton

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledPushButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->PushButtonSet->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -side left -anchor nw -fill y" << endl;
      }
    tk_cmd << "pack " << this->PushButtonSet->GetWidgetName() 
           << " -side left -anchor nw -fill y" << endl;
    }
  else
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -side top -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->PushButtonSet->GetWidgetName() 
           << " -side top -anchor nw -padx 10" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledPushButtonSet::SetPackHorizontally(int _arg)
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
void vtkKWLabeledPushButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PushButtonSet)
    {
    this->PushButtonSet->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledPushButtonSet::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->PushButtonSet)
    {
    this->PushButtonSet->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledPushButtonSet::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->PushButtonSet)
    {
    this->PushButtonSet->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledPushButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PushButtonSet: " << this->PushButtonSet << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}

