/*=========================================================================

  Module:    vtkKWLabeledWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledWidget.h"

#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledWidget);
vtkCxxRevisionMacro(vtkKWLabeledWidget, "1.12");

int vtkKWLabeledWidgetCommand(ClientData cd, Tcl_Interp *interp,
                              int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledWidget::vtkKWLabeledWidget()
{
  this->CommandFunction = vtkKWLabeledWidgetCommand;
  this->ShowLabel       = 1;
  this->Label           = NULL;
}

//----------------------------------------------------------------------------
vtkKWLabeledWidget::~vtkKWLabeledWidget()
{
  if (this->Label)
    {
    this->Label->Delete();
    this->Label = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWLabeledWidget::GetLabel()
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
int vtkKWLabeledWidget::HasLabel()
{
  return this->Label ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledWidget widget already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s -relief flat -bd 0 -highlightthickness 0 %s", 
               this->GetWidgetName(), args ? args : "");

  // Create the label now if it has to be shown now

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
void vtkKWLabeledWidget::CreateLabel(vtkKWApplication *app)
{
  // Create the label. If the parent has been set before (i.e. by the subclass)
  // do not set it.
  // This will also create the label on the fly, if needed
  
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
  // -bd 0 -highlightthickness 0 -padx 0 -pady 0");

  label->SetBalloonHelpString(this->GetBalloonHelpString());
  label->SetBalloonHelpJustification(this->GetBalloonHelpJustification());
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetLabel(const char *text)
{
  this->GetLabel()->SetLabel(text);
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetLabelWidth(int width)
{
  this->GetLabel()->SetWidth(width);
}

//----------------------------------------------------------------------------
int vtkKWLabeledWidget::GetLabelWidth()
{
  return this->GetLabel()->GetWidth();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetShowLabel(int _arg)
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
void vtkKWLabeledWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  
  // Do not use GetLabel() here, otherwise the label would be created 
  // on the fly, and we don't want this. Once the label gets created when
  // there is a real need for, it's Enabled state will be set correctly
  // anyway.

  if (this->Label)
    {
    this->Label->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledWidget::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  // Do not use GetLabel() here, otherwise the label would be created 
  // on the fly, and we don't want this. Once the label gets created when
  // there is a real need for, it's ballon help state will be set correctly
  // anyway.

  if (this->Label)
    {
    this->Label->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledWidget::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  // Do not use GetLabel() here, otherwise the label would be created 
  // on the fly, and we don't want this. Once the label gets created when
  // there is a real need for, it's ballon help state will be set correctly
  // anyway.

  if (this->Label)
    {
    this->Label->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowLabel: " 
     << (this->ShowLabel ? "On" : "Off") << endl;

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

