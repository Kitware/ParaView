/*=========================================================================

  Module:    vtkKWWidgetLabeled.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidgetLabeled.h"

#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWidgetLabeled);
vtkCxxRevisionMacro(vtkKWWidgetLabeled, "1.2");

int vtkKWWidgetLabeledCommand(ClientData cd, Tcl_Interp *interp,
                              int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWWidgetLabeled::vtkKWWidgetLabeled()
{
  this->CommandFunction = vtkKWWidgetLabeledCommand;
  this->ShowLabel       = 1;
  this->Label           = NULL;
  this->LabelPosition   = vtkKWWidgetLabeled::LabelPositionDefault;
}

//----------------------------------------------------------------------------
vtkKWWidgetLabeled::~vtkKWWidgetLabeled()
{
  if (this->Label)
    {
    this->Label->Delete();
    this->Label = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWWidgetLabeled::GetLabel()
{
  // Lazy evaluation. Create the label only when it is needed

  if (!this->Label)
    {
    this->Label = vtkKWLabel::New();
    this->Label->SetEnabled(this->Enabled);
    }

  return this->Label;
}

//----------------------------------------------------------------------------
int vtkKWWidgetLabeled::HasLabel()
{
  return this->Label ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(
        app, "frame", "-relief flat -bd 0 -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  // Create the label subwidget now if it has to be shown now

  if (this->ShowLabel)
    {
    this->CreateLabel(app);
    }

  // Subclasses will call this->Pack() here. Not now.
  // this->Pack();

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::CreateLabel(vtkKWApplication *app, const char *args)
{
  // Create the label. If the parent has been set before (i.e. by the subclass)
  // do not set it.
  // Note that GetLabel() will allocate the label on the fly
  
  vtkKWLabel *label = this->GetLabel();
  if (label->IsCreated())
    {
    return;
    }

  if (!label->GetParent())
    {
    label->SetParent(this);
    }

  label->Create(app, "-anchor w");
  label->ConfigureOptions(args);
  // -bd 0 -highlightthickness 0 -padx 0 -pady 0");

  label->SetBalloonHelpString(this->GetBalloonHelpString());
  label->SetBalloonHelpJustification(this->GetBalloonHelpJustification());

  // Since we have just created the label on the fly, it is likely that 
  // it needs to be displayed somehow, which is usually Pack()'s job

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::SetLabelText(const char *text)
{
  this->GetLabel()->SetText(text);
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::SetLabelPosition(int arg)
{
  if (arg < vtkKWWidgetLabeled::LabelPositionDefault)
    {
    arg = vtkKWWidgetLabeled::LabelPositionDefault;
    }
  else if (arg > vtkKWWidgetLabeled::LabelPositionRight)
    {
    arg = vtkKWWidgetLabeled::LabelPositionRight;
    }

  if (this->LabelPosition == arg)
    {
    return;
    }

  this->LabelPosition = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::SetLabelWidth(int width)
{
  this->GetLabel()->SetWidth(width);
}

//----------------------------------------------------------------------------
int vtkKWWidgetLabeled::GetLabelWidth()
{
  return this->GetLabel()->GetWidth();
}

// ----------------------------------------------------------------------------
void vtkKWWidgetLabeled::SetShowLabel(int _arg)
{
  if (this->ShowLabel == _arg)
    {
    return;
    }
  this->ShowLabel = _arg;
  this->Modified();

  // Make sure that if the label has to be show, we create it on the fly if
  // needed

  if (this->ShowLabel && this->IsCreated())
    {
    this->CreateLabel(this->GetApplication());
    }

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  
  // Do not use GetLabel() here, otherwise the label will be created 
  // on the fly, and we do not want this. Once the label gets created when
  // there is a real need for it, its Enabled state will be set correctly
  // anyway.

  if (this->Label)
    {
    this->Label->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWWidgetLabeled::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  // Do not use GetLabel() here, otherwise the label will be created 
  // on the fly, and we do not want this. Once the label gets created when
  // there is a real need for it, its Enabled state will be set correctly
  // anyway.

  if (this->Label)
    {
    this->Label->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWWidgetLabeled::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  // Do not use GetLabel() here, otherwise the label will be created 
  // on the fly, and we do not want this. Once the label gets created when
  // there is a real need for it, its Enabled state will be set correctly
  // anyway.

  if (this->Label)
    {
    this->Label->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetLabeled::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowLabel: " 
     << (this->ShowLabel ? "On" : "Off") << endl;

  os << indent << "LabelPosition: " << this->LabelPosition << endl;

  os << indent << "Label: ";
  if (this->Label)
    {
    os << endl;
    this->Label->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
 }

