/*=========================================================================

  Module:    vtkKWLabeledLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledLabel.h"

#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledLabel);
vtkCxxRevisionMacro(vtkKWLabeledLabel, "1.14");

int vtkKWLabeledLabelCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledLabel::vtkKWLabeledLabel()
{
  this->CommandFunction = vtkKWLabeledLabelCommand;

  this->PackHorizontally = 1;
  this->ExpandLabel2     = 0;
  this->LabelAnchor      = vtkKWWidget::ANCHOR_NW;

  this->Label2 = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledLabel::~vtkKWLabeledLabel()
{
  if (this->Label2)
    {
    this->Label2->Delete();
    this->Label2 = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledLabel already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the 2nd label

  this->Label2->SetParent(this);
  this->Label2->Create(app, "-anchor nw");

  // Pack the labels

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  tk_cmd << this->Label2->GetWidgetName() << " config -anchor " 
         << this->GetAnchorAsString(this->LabelAnchor) << endl;

  if (this->PackHorizontally)
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side left -anchor " 
             << this->GetAnchorAsString(this->LabelAnchor) << endl;
      }
    tk_cmd << "pack " << this->Label2->GetWidgetName() 
           << " -side left -anchor nw -fill both -expand " 
           << (this->ExpandLabel2 ? "y" : "n") << endl;
    }
  else
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side top -anchor " 
             << this->GetAnchorAsString(this->LabelAnchor) << endl;
      }
    tk_cmd << "pack " << this->Label2->GetWidgetName() 
           << " -side top -anchor nw -padx 10 -fill both -expand "
           << (this->ExpandLabel2 ? "y" : "n") << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetExpandLabel2(int _arg)
{
  if (this->ExpandLabel2 == _arg)
    {
    return;
    }
  this->ExpandLabel2 = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetLabelAnchor(int _arg)
{
  if (this->LabelAnchor == _arg)
    {
    return;
    }
  this->LabelAnchor = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Label2)
    {
    this->Label2->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetLabel2(const char *text)
{
  if (this->Label2)
    {
    this->Label2->SetLabel(text);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLabel::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Label2)
    {
    this->Label2->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLabel::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->Label2)
    {
    this->Label2->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label2: " << this->Label2 << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "ExpandLabel2: " 
     << (this->ExpandLabel2 ? "On" : "Off") << endl;

  os << indent << "LabelAnchor: " 
     << this->GetAnchorAsString(this->LabelAnchor) << endl;
}

