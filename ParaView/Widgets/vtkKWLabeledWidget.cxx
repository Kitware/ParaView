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
vtkCxxRevisionMacro(vtkKWLabeledWidget, "1.11");

int vtkKWLabeledWidgetCommand(ClientData cd, Tcl_Interp *interp,
                              int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledWidget::vtkKWLabeledWidget()
{
  this->CommandFunction = vtkKWLabeledWidgetCommand;

  this->ShowLabel = 1;

  this->Label = vtkKWLabel::New();
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

  // Create the label. If the parent has been set before (i.e. by the subclass)
  // do not set it.
  
  if (!this->Label->GetParent())
    {
    this->Label->SetParent(this);
    }

  this->Label->Create(app, "-anchor w");
  // -bd 0 -highlightthickness 0 -padx 0 -pady 0");

  // Subclasses will call this->Pack() here
  // this->Pack();

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetLabel(const char *text)
{
  if (this->Label)
    {
    this->Label->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetLabelWidth(int width)
{
  if (this->Label)
    {
    this->Label->SetWidth(width);
    }
}

//----------------------------------------------------------------------------
int vtkKWLabeledWidget::GetLabelWidth()
{
  if (this->Label)
    {
    return this->Label->GetWidth();
    }
  return 0;
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

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Label)
    {
    this->Label->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledWidget::SetBalloonHelpString(const char *string)
{
  if (this->Label)
    {
    this->Label->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledWidget::SetBalloonHelpJustification(int j)
{
  if (this->Label)
    {
    this->Label->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label: " << this->Label << endl;

  os << indent << "ShowLabel: " 
     << (this->ShowLabel ? "On" : "Off") << endl;
}

