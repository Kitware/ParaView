/*=========================================================================

  Module:    vtkKWChangeColorButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWChangeColorButton.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWInternationalization.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWChangeColorButton);
vtkCxxRevisionMacro(vtkKWChangeColorButton, "1.73");

//----------------------------------------------------------------------------
vtkKWChangeColorButton::vtkKWChangeColorButton()
{
  this->Command = NULL;

  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;

  this->LabelOutsideButton = 0;

  this->DialogTitle = NULL;

  this->ColorButton = vtkKWLabel::New();
  this->ButtonFrame = vtkKWFrame::New();
  
  this->ButtonDown = 0;

  this->GetLabel()->SetText(ks_("Change Color Button|Set Color..."));
}

//----------------------------------------------------------------------------
vtkKWChangeColorButton::~vtkKWChangeColorButton()
{
  if (this->Command)
    {
    delete [] this->Command;
    }

  this->SetDialogTitle(NULL);

  if (this->ColorButton)
    {
    this->ColorButton->Delete();
    this->ColorButton = NULL;
    }

  if (this->ButtonFrame)
    {
    this->ButtonFrame->Delete();
    this->ButtonFrame = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SetColor(double r, double g, double b)
{
  if (this->Color[0] == r && this->Color[1] == g && this->Color[2] == b)
    {
    return;
    }

  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;

  this->UpdateColorButton();

  this->InvokeEvent(vtkKWChangeColorButton::ColorChangedEvent, this->Color);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("vtkKWChangeColorButton already created");
    return;
    }

  // Call the superclass, this will set the application,
  // create the frame and the Label

  this->Superclass::CreateWidget();

  // Create the main frame

  this->CreateButtonFrame();

  // Create the color button

  this->ColorButton->SetParent(this->ButtonFrame);
  this->ColorButton->Create();
  this->ColorButton->SetBorderWidth(1);
  this->ColorButton->SetReliefToGroove();
  this->ColorButton->SetPadX(0);
  this->ColorButton->SetPadY(0);
  this->ColorButton->SetWidth(2);
  this->ColorButton->SetHighlightThickness(0);
  this->ColorButton->SetForegroundColor(0.0, 0.0, 0.0);

  this->UpdateColorButton();

  // Pack the whole stuff

  this->Pack();

  // Bind

  this->Bind();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::CreateLabel()
{
  // Tk bug: The button frame *has* to be created before the label if we
  // want to be able to pack the label inside the frame

  this->CreateButtonFrame();

  this->Superclass::CreateLabel();

  vtkKWLabel *label = this->GetLabel();
  label->SetPadX(2);
  label->SetPadY(0);
  label->SetBorderWidth(0);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::CreateButtonFrame()
{
  if (this->ButtonFrame->IsCreated())
    {
    return;
    }

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create();
  this->ButtonFrame->SetReliefToRaised();
  this->ButtonFrame->SetBorderWidth(2);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->ColorButton->UnpackSiblings();
  this->ButtonFrame->UnpackSiblings();

  ostrstream tk_cmd;

  // Repack everything

  int is_before = 
    (this->LabelPosition != vtkKWWidgetWithLabel::LabelPositionRight);

  if (this->LabelOutsideButton)
    {
    const char *side = is_before ? "left" : "right";
    if (this->LabelVisibility && this->HasLabel() && 
        this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -expand y -fill both -anchor w -padx 2 -in " 
             << this->GetWidgetName() << " -side " << side << endl;
      }
    if (this->ButtonFrame && this->ButtonFrame->IsCreated())
      { 
      tk_cmd << "pack " << this->ButtonFrame->GetWidgetName() 
             << " -expand n -fill both -side " << side << endl;
      }
    if (this->ColorButton && this->ColorButton->IsCreated())
      {
      tk_cmd << "pack " << this->ColorButton->GetWidgetName() 
             << " -side left -expand n -fill y -padx 2 -pady 2" << endl;
      }
    }
  else
    {
    if (this->ButtonFrame && this->ButtonFrame->IsCreated())
      { 
      int col = (is_before ? 0 : 1);
      tk_cmd << "pack " << this->ButtonFrame->GetWidgetName() 
             << " -side left -expand y -fill both " << endl;
      if (this->LabelVisibility && this->HasLabel() && 
          this->GetLabel()->IsCreated())
        {
        tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
               << " -sticky ns -row 0 -column " << col << " -in " 
               << this->ButtonFrame->GetWidgetName() << endl;
        tk_cmd << "grid columnconfigure " 
               << this->ButtonFrame->GetWidgetName() 
               << " " << col << " -weight 1" << endl;
        }
      }
    if (this->ColorButton && this->ColorButton->IsCreated())
      {
      int col = (is_before ? 1 : 0);
      tk_cmd << "grid " << this->ColorButton->GetWidgetName() 
             << " -padx 2 -pady 2 -sticky ns -row 0 -column " << col << endl;
      tk_cmd << "grid columnconfigure " 
             << this->ColorButton->GetParent()->GetWidgetName() 
             << " " << col << " -weight 0" << endl;
      }
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SetLabelOutsideButton(int arg)
{
  if (this->LabelOutsideButton == arg)
    {
    return;
    }

  this->LabelOutsideButton = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::UpdateColorButton()
{
  if (!this->ColorButton->IsCreated())
    {
    return;
    }

  if (this->GetEnabled())
    {
    this->ColorButton->SetBackgroundColor(this->Color);
    }
  else
    {
    this->ColorButton->SetBackgroundColor(
      vtkKWTkUtilities::GetOptionColor(
        this->ColorButton, "-disabledforeground"));
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::Bind()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->ButtonFrame->IsCreated())
    {
    this->ButtonFrame->AddBinding(
      "<Any-ButtonPress>", this, "ButtonPressCallback");
    this->ButtonFrame->AddBinding(
      "<Any-ButtonRelease>", this, "ButtonReleaseCallback");
    }

  if (!this->LabelOutsideButton && 
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    this->GetLabel()->AddBinding(
      "<Any-ButtonPress>", this, "ButtonPressCallback");
    this->GetLabel()->AddBinding(
      "<Any-ButtonRelease>", this, "ButtonReleaseCallback");
    }

  if (this->ColorButton->IsCreated())
    {
    this->ColorButton->AddBinding(
      "<Any-ButtonPress>", this, "ButtonPressCallback");
    this->ColorButton->AddBinding(
      "<Any-ButtonRelease>", this, "ButtonReleaseCallback");
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::UnBind()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->ButtonFrame->IsCreated())
    {
    this->ButtonFrame->RemoveBinding("<Any-ButtonPress>");
    this->ButtonFrame->RemoveBinding("<Any-ButtonRelease>");
    }

  if (!this->LabelOutsideButton &&
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    this->GetLabel()->RemoveBinding("<Any-ButtonPress>");
    this->GetLabel()->RemoveBinding("<Any-ButtonRelease>");
    }

  if (this->ColorButton->IsCreated())
    {
    this->ColorButton->RemoveBinding("<Any-ButtonPress>");
    this->ColorButton->RemoveBinding("<Any-ButtonRelease>");
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::ButtonPressCallback()
{  
  this->ButtonDown = 1;
  this->ButtonFrame->SetReliefToSunken();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::ButtonReleaseCallback()
{  
  if (!this->ButtonDown)
    {
    return;
    }
  
  this->ButtonDown = 0;
  this->ButtonFrame->SetReliefToRaised();
  this->QueryUserForColor();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Color button

  this->PropagateEnableState(this->ColorButton);

  // Now given the state, bind, or unbind

  if (this->IsCreated())
    {
    this->UpdateColorButton();
    if (this->GetEnabled())
      {
      this->Bind();
      }
    else
      {
      this->UnBind();
      }
    }
}

// ---------------------------------------------------------------------------
void vtkKWChangeColorButton::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->ColorButton)
    {
    this->ColorButton->SetBalloonHelpString(string);
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::QueryUserForColor()
{  
  if (vtkKWTkUtilities::QueryUserForColor(
        this->GetApplication(),
        this->GetWidgetName(),
        this->DialogTitle,
        this->Color[0], this->Color[1], this->Color[2],
        &this->Color[0], &this->Color[1], &this->Color[2]))
    {
    this->UpdateColorButton();
    this->InvokeCommand(this->Color[0], this->Color[1], this->Color[2]);
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SetCommand(vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::InvokeCommand(double r, double g, double b)
{  
  if (this->GetApplication() &&
      this->Command && *this->Command)
    {
    this->Script("%s %lf %lf %lf", this->Command, r, g, b);
    }
  double rgb[3];
  rgb[0] = r; rgb[1] = g; rgb[2] = b;
  this->InvokeEvent(vtkKWChangeColorButton::ColorChangedEvent, rgb);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DialogTitle: " 
     << (this->GetDialogTitle() ? this->GetDialogTitle() : "(none)")
     << endl;

  os << indent << "LabelOutsideButton: " 
     << (this->LabelOutsideButton ? "On\n" : "Off\n");
}

