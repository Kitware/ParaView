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

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWChangeColorButton);
vtkCxxRevisionMacro(vtkKWChangeColorButton, "1.50");

int vtkKWChangeColorButtonCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWChangeColorButton::vtkKWChangeColorButton()
{
  this->CommandFunction = vtkKWChangeColorButtonCommand;

  this->Command = NULL;

  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;

  this->LabelOutsideButton = 0;

  this->DialogText = 0;

  this->ColorButton = vtkKWLabel::New();
  this->ButtonFrame = vtkKWFrame::New();
  
  this->ButtonDown = 0;

  this->GetLabel()->SetText("Set Color...");
}

//----------------------------------------------------------------------------
vtkKWChangeColorButton::~vtkKWChangeColorButton()
{
  if (this->Command)
    {
    delete [] this->Command;
    }

  this->SetDialogText(0);

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
  if ( this->Color[0] == r && this->Color[1] == g && this->Color[2] == b )
    {
    return;
    }

  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;

  this->UpdateColorButton();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("vtkKWChangeColorButton already created");
    return;
    }

  // Call the superclass, this will set the application,
  // create the frame and the Label

  this->Superclass::Create(app, args);

  // Create the main frame

  this->CreateButtonFrame(app);

  // Create the color button

  this->ColorButton->SetParent(this->ButtonFrame);
  this->ColorButton->Create(app, "-bd 1 -fg black -relief groove -highlightthickness 0 -padx 0 -pady 0 -width 2");

  this->UpdateColorButton();

  // Pack the whole stuff

  this->Pack();

  // Bind

  this->Bind();

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::CreateLabel(vtkKWApplication *app, const char *vtkNotUsed(args))
{
  // Tk bug: The button frame *has* to be created before the label if we
  // want to be able to pack the label inside the frame

  this->CreateButtonFrame(app);

  this->Superclass::CreateLabel(
    app, "-padx 2 -pady 0 -highlightthickness 0 -bd 0");
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::CreateButtonFrame(vtkKWApplication *app, const char *args)
{
  if (this->ButtonFrame->IsCreated())
    {
    return;
    }

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "-relief raised -bd 2");
  this->ButtonFrame->ConfigureOptions(args);
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
    (this->LabelPosition != vtkKWWidgetLabeled::LabelPositionRight);

  if (this->LabelOutsideButton)
    {
    const char *side = is_before ? "left" : "right";
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -expand y -fill both -anchor w -in " 
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
      if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
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

  if (this->Enabled)
    {
    if ( this->Color[0] < 0 ) { this->Color[0] = 0; }
    if ( this->Color[1] < 0 ) { this->Color[1] = 0; }
    if ( this->Color[2] < 0 ) { this->Color[2] = 0; }
    char color[256];
    sprintf(color, "#%02x%02x%02x", 
            (int)(this->Color[0] * 255.5), 
            (int)(this->Color[1] * 255.5), 
            (int)(this->Color[2] * 255.5));
    this->Script("%s configure -bg %s", 
                 this->ColorButton->GetWidgetName(), color);
    }
  else
    {
#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 3)
    this->Script("%s configure -bg #808080", 
                 this->ColorButton->GetWidgetName());
#else
    this->Script("%s configure -bg [%s cget -disabledforeground] ", 
                 this->ColorButton->GetWidgetName(), 
                 this->ColorButton->GetWidgetName());
#endif   
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
    this->Script(
      "bind %s <Any-ButtonPress> {+%s ButtonPressCallback}",
      this->ButtonFrame->GetWidgetName(), this->GetTclName());
    this->Script(
      "bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback}",
      this->ButtonFrame->GetWidgetName(), this->GetTclName());
    }

  if (!this->LabelOutsideButton && 
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    this->Script(
      "bind %s <Any-ButtonPress> {+%s ButtonPressCallback}",
      this->GetLabel()->GetWidgetName(), this->GetTclName());
    this->Script(
      "bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback}",
      this->GetLabel()->GetWidgetName(), this->GetTclName());
    }

  if (this->ColorButton->IsCreated())
    {
    this->Script(
      "bind %s <Any-ButtonPress> {+%s ButtonPressCallback}",
      this->ColorButton->GetWidgetName(), this->GetTclName());
    this->Script(
      "bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback}",
      this->ColorButton->GetWidgetName(), this->GetTclName());
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
    this->Script("bind %s <Any-ButtonPress> {}", 
                 this->ButtonFrame->GetWidgetName());
    this->Script("bind %s <Any-ButtonRelease> {}", 
                 this->ButtonFrame->GetWidgetName());
    }

  if (!this->LabelOutsideButton &&
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    this->Script("bind %s <Any-ButtonPress> {}", 
                 this->GetLabel()->GetWidgetName());
    this->Script("bind %s <Any-ButtonRelease> {}", 
                 this->GetLabel()->GetWidgetName());
    }

  if (this->ColorButton->IsCreated())
    {
    this->Script("bind %s <Any-ButtonPress> {}",
                 this->ColorButton->GetWidgetName());
    this->Script("bind %s <Any-ButtonRelease> {}", 
                 this->ColorButton->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::ButtonPressCallback()
{  
  this->ButtonDown = 1;
  this->Script("%s configure -relief sunken", 
               this->ButtonFrame->GetWidgetName());  
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::ButtonReleaseCallback()
{  
  if (!this->ButtonDown)
    {
    return;
    }
  
  this->ButtonDown = 0;
  
  this->Script("%s configure -relief raised", 
               this->ButtonFrame->GetWidgetName());  

  this->QueryUserForColor();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Color button

  if (this->ColorButton)
    {
    this->ColorButton->SetEnabled(this->Enabled);
    }

  // Now given the state, bind or unbind

  if (this->IsCreated())
    {
    this->UpdateColorButton();
    if (this->Enabled)
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

// ---------------------------------------------------------------------------
void vtkKWChangeColorButton::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->ColorButton)
    {
    this->ColorButton->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::QueryUserForColor()
{  
  int r, g, b;
  char *result, tmp[3];

  this->GetApplication()->SetDialogUp(1);

  this->Script(
     "tk_chooseColor -initialcolor {#%02x%02x%02x} -title {%s} -parent %s",
     (int)(this->Color[0] * 255.5), 
     (int)(this->Color[1] * 255.5), 
     (int)(this->Color[2] * 255.5),
     (this->DialogText?this->DialogText:"Choose Color"),
     this->GetWidgetName() );

  result = this->GetApplication()->GetMainInterp()->result;

  this->GetApplication()->SetDialogUp(0);

  if (strlen(result) > 6)
    {
    tmp[2] = '\0';
    tmp[0] = result[1];
    tmp[1] = result[2];
    sscanf(tmp, "%x", &r);
    tmp[0] = result[3];
    tmp[1] = result[4];
    sscanf(tmp, "%x", &g);
    tmp[0] = result[5];
    tmp[1] = result[6];
    sscanf(tmp, "%x", &b);
    
    this->Color[0] = (double)r / 255.0;
    this->Color[1] = (double)g / 255.0;
    this->Color[2] = (double)b / 255.0;

    this->UpdateColorButton();

    if ( this->Command )
      {
      this->Script("eval %s %lf %lf %lf", 
                   this->Command, 
                   this->Color[0], this->Color[1], this->Color[2]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SetCommand( vtkKWObject* CalledObject, 
                                         const char *CommandString )
{
  this->SetObjectMethodCommand(&this->Command, CalledObject, CommandString);
}

//----------------------------------------------------------------------------
// Description:
// Chaining method to serialize an object and its superclasses.
void vtkKWChangeColorButton::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->Superclass::SerializeSelf(os,indent);
  os << indent << "Color " << this->Color[0] << " " << this->Color[1] <<
    " " << this->Color[2] << endl;
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SerializeToken(istream& is, const char *token)
{
  double clr[3];
  if (!strcmp(token,"Color"))
    {
    is >> clr[0] >> clr[1] >> clr[2];
    this->SetColor(clr);
    if ( this->Command )
      {
      this->Script("eval %s %lf %lf %lf", this->Command, 
                   clr[0], clr[1], clr[2]);
      }
    return;
    }
  vtkKWWidget::SerializeToken(is, token);
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWWidget::SerializeRevision(os,indent);
  os << indent << "vtkKWChangeColorButton ";
  this->ExtractRevision(os,"$Revision: 1.50 $");
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DialogText: " 
     << (this->GetDialogText() ? this->GetDialogText() : "(none)")
     << endl;

  os << indent << "LabelOutsideButton: " 
     << (this->LabelOutsideButton ? "On\n" : "Off\n");
}

