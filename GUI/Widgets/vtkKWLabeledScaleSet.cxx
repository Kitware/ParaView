/*=========================================================================

  Module:    vtkKWLabeledScaleSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledScaleSet.h"

#include "vtkKWLabel.h"
#include "vtkKWScaleSet.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledScaleSet);
vtkCxxRevisionMacro(vtkKWLabeledScaleSet, "1.5");

int vtkKWLabeledScaleSetCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledScaleSet::vtkKWLabeledScaleSet()
{
  this->CommandFunction = vtkKWLabeledScaleSetCommand;

  this->PackHorizontally = 0;

  this->ScaleSet = vtkKWScaleSet::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledScaleSet::~vtkKWLabeledScaleSet()
{
  if (this->ScaleSet)
    {
    this->ScaleSet->Delete();
    this->ScaleSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledScaleSet::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledScaleSet already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the scale set

  this->ScaleSet->SetParent(this);
  this->ScaleSet->Create(app, 0);

  // Pack the label and the scale set

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledScaleSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->ScaleSet->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -side left -anchor nw -fill y" << endl;
      }
    tk_cmd << "pack " << this->ScaleSet->GetWidgetName() 
           << " -side left -anchor nw -expand y -fill both" << endl;
    }
  else
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -side top -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->ScaleSet->GetWidgetName() 
           << " -side top -anchor nw -padx 10 -expand y -fill x" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledScaleSet::SetPackHorizontally(int _arg)
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
void vtkKWLabeledScaleSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->ScaleSet)
    {
    this->ScaleSet->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledScaleSet::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->ScaleSet)
    {
    this->ScaleSet->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledScaleSet::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->ScaleSet)
    {
    this->ScaleSet->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledScaleSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ScaleSet: " << this->ScaleSet << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}

