/*=========================================================================

  Module:    vtkKWWidgetWithLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidgetWithLabel.h"

#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWidgetWithLabel);
vtkCxxRevisionMacro(vtkKWWidgetWithLabel, "1.1");

//----------------------------------------------------------------------------
vtkKWWidgetWithLabel::vtkKWWidgetWithLabel()
{
  this->ShowLabel       = 1;
  this->Label           = NULL;
  this->LabelPosition   = vtkKWWidgetWithLabel::LabelPositionDefault;
}

//----------------------------------------------------------------------------
vtkKWWidgetWithLabel::~vtkKWWidgetWithLabel()
{
  if (this->Label)
    {
    this->Label->Delete();
    this->Label = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWWidgetWithLabel::GetLabel()
{
  // Lazy evaluation. Create the label only when it is needed

  if (!this->Label)
    {
    this->Label = vtkKWLabel::New();
    this->PropagateEnableState(this->Label);
    }

  return this->Label;
}

//----------------------------------------------------------------------------
int vtkKWWidgetWithLabel::HasLabel()
{
  return this->Label ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithLabel::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

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
void vtkKWWidgetWithLabel::CreateLabel(vtkKWApplication *app)
{
  // Create the label. If the parent has been set before (i.e. by the subclass)
  // do not set it.
  // Note that GetLabel() will allocate the label on the fly
  
  if (this->HasLabel() && this->GetLabel()->IsCreated())
    {
    return;
    }

  vtkKWLabel *label = this->GetLabel();
  if (!label->GetParent())
    {
    label->SetParent(this);
    }

  label->Create(app);
  label->SetAnchorToWest();
  // -bd 0 -highlightthickness 0 -padx 0 -pady 0");

  label->SetBalloonHelpString(this->GetBalloonHelpString());

  // Since we have just created the label on the fly, it is likely that 
  // it needs to be displayed somehow, which is usually Pack()'s job

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithLabel::SetLabelText(const char *text)
{
  this->GetLabel()->SetText(text);
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithLabel::SetLabelPosition(int arg)
{
  if (arg < vtkKWWidgetWithLabel::LabelPositionDefault)
    {
    arg = vtkKWWidgetWithLabel::LabelPositionDefault;
    }
  else if (arg > vtkKWWidgetWithLabel::LabelPositionRight)
    {
    arg = vtkKWWidgetWithLabel::LabelPositionRight;
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
void vtkKWWidgetWithLabel::SetLabelWidth(int width)
{
  this->GetLabel()->SetWidth(width);
}

//----------------------------------------------------------------------------
int vtkKWWidgetWithLabel::GetLabelWidth()
{
  return this->GetLabel()->GetWidth();
}

// ----------------------------------------------------------------------------
void vtkKWWidgetWithLabel::SetShowLabel(int _arg)
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
void vtkKWWidgetWithLabel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  
  // Do not use GetLabel() here, otherwise the label will be created 
  // on the fly, and we do not want this. Once the label gets created when
  // there is a real need for it, its Enabled state will be set correctly
  // anyway.

  this->PropagateEnableState(this->Label);
}

// ---------------------------------------------------------------------------
void vtkKWWidgetWithLabel::SetBalloonHelpString(const char *string)
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

//----------------------------------------------------------------------------
void vtkKWWidgetWithLabel::PrintSelf(ostream& os, vtkIndent indent)
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

