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
vtkCxxRevisionMacro(vtkKWChangeColorButton, "1.47");

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

  this->LabelAfterColor = 0;
  this->LabelOutsideButton = 0;

  this->Text = NULL;
  this->DialogText = 0;
  this->SetText("Set Color...");

  this->ColorButton = vtkKWLabel::New();
  this->MainFrame = vtkKWFrame::New();
  
  this->ButtonDown = 0;
}

//----------------------------------------------------------------------------
vtkKWChangeColorButton::~vtkKWChangeColorButton()
{
  if (this->Command)
    {
    delete [] this->Command;
    }

  this->SetText(0);
  this->SetDialogText(0);

  if (this->ColorButton)
    {
    this->ColorButton->Delete();
    this->ColorButton = NULL;
    }

  if (this->MainFrame)
    {
    this->MainFrame->Delete();
    this->MainFrame = NULL;
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
  // Do not call vtkKWLabeledWidget::Create() here since we have to create
  // the Label in a special way (i.e. as a child of the MainFrame widget
  // that would not have been known to the superclass).
  // Use vtkKWWidget's Create() instead.

  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(
        app, "frame", "-relief flat -bd 0 -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  // Create the main frame

  this->MainFrame->SetParent(this);
  this->MainFrame->Create(app, "-relief raised -bd 2");

  // Create the label now if it has to be shown now
  // CreateLabel() has been overriden so that the label can be attached
  // to the MainFrame if needed

  if (this->ShowLabel)
    {
    this->CreateLabel(app);
    }

  // Create the color button

  this->ColorButton->SetParent(this->MainFrame);
  this->ColorButton->Create(
    app, "-bd 1 -fg black -relief groove -highlightthickness 0 -padx 0 -pady 0 -width 2");

  this->UpdateColorButton();

  // Pack the whole stuff

  this->Pack();

  // Bind button presses

  this->Bind();

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::CreateLabel(vtkKWApplication *app)
{
  // Override the parent's CreateLabel()
  // This will also create the label on the fly, if needed
  
  vtkKWLabel *label = this->GetLabel();
  if (label->IsCreated())
    {
    return;
    }

  if (this->LabelOutsideButton)
    {
    label->SetParent(this);
    }
  else
    {
    label->SetParent(this->MainFrame);
    }

  label->Create(app, "-padx 2 -pady 0 -highlightthickness 0 -bd 0");
  label->SetLabel(this->Text);

  label->SetBalloonHelpString(this->GetBalloonHelpString());
  label->SetBalloonHelpJustification(this->GetBalloonHelpJustification());
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
  this->MainFrame->UnpackSiblings();

  // Repack everything

  if (this->LabelOutsideButton)
    {
    this->Script("pack %s -expand y -fill y -padx 2 -pady 2",
                 this->ColorButton->GetWidgetName());
    this->Script("grid %s -row 0 -column %d -sticky news", 
                 this->MainFrame->GetWidgetName(),
                 this->LabelAfterColor ? 0 : 1);
    }
  else
    {
    this->Script("pack %s -expand y -fill both",
                 this->MainFrame->GetWidgetName());
    this->Script("grid %s -row 0 -column %d -sticky news -padx 2 -pady 2", 
                 this->ColorButton->GetWidgetName(),
                 this->LabelAfterColor ? 0 : 1);
    }

  if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
    { 
    this->Script("grid %s -row 0 -column %d -sticky news", 
                 this->GetLabel()->GetWidgetName(),
                 this->LabelAfterColor ? 1 : 0);
    this->Script("grid columnconfigure %s %d -weight 1", 
                 this->GetLabel()->GetParent()->GetWidgetName(),
                 this->LabelAfterColor ? 1 : 0);
    this->Script("grid columnconfigure %s %d -weight 0", 
                 this->GetLabel()->GetParent()->GetWidgetName(),
                 this->LabelAfterColor ? 0 : 1);
    }

  this->Script("grid rowconfigure %s 0 -weight 1", 
               this->MainFrame->GetParent()->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SetLabelAfterColor(int arg)
{
  if (this->LabelAfterColor == arg)
    {
    return;
    }

  this->LabelAfterColor = arg;
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

  if (this->MainFrame->IsCreated())
    {
    this->Script(
      "bind %s <Any-ButtonPress> {+%s ButtonPressCallback %%X %%Y}",
      this->MainFrame->GetWidgetName(), this->GetTclName());
    this->Script(
      "bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback %%X %%Y}",
      this->MainFrame->GetWidgetName(), this->GetTclName());
    }

  if (!this->LabelOutsideButton && 
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    this->Script(
      "bind %s <Any-ButtonPress> {+%s ButtonPressCallback %%X %%Y}",
      this->GetLabel()->GetWidgetName(), this->GetTclName());
    this->Script(
      "bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback %%X %%Y}",
      this->GetLabel()->GetWidgetName(), this->GetTclName());
    }

  if (this->ColorButton->IsCreated())
    {
    this->Script(
      "bind %s <Any-ButtonPress> {+%s ButtonPressCallback %%X %%Y}",
      this->ColorButton->GetWidgetName(), this->GetTclName());
    this->Script(
      "bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback %%X %%Y}",
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

  if (this->MainFrame->IsCreated())
    {
    this->Script("bind %s <Any-ButtonPress> {}", 
                 this->MainFrame->GetWidgetName());
    this->Script("bind %s <Any-ButtonRelease> {}", 
                 this->MainFrame->GetWidgetName());
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
void vtkKWChangeColorButton::ButtonPressCallback(int /*x*/, int /*y*/)
{  
  this->ButtonDown = 1;
  this->Script("%s configure -relief sunken", 
               this->MainFrame->GetWidgetName());  
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::ButtonReleaseCallback(int x, int y)
{  
  if (!this->ButtonDown)
    {
    return;
    }
  
  this->ButtonDown = 0;
  
  this->Script("%s configure -relief raised", 
               this->MainFrame->GetWidgetName());  

  // Was it released over the button ?

  this->Script( "winfo rootx %s", this->MainFrame->GetWidgetName());
  int xw = vtkKWObject::GetIntegerResult(this->GetApplication());

  this->Script( "winfo rooty %s", this->MainFrame->GetWidgetName());
  int yw = vtkKWObject::GetIntegerResult(this->GetApplication());

  // Get the size and of the window

  this->Script( "winfo width %s", this->MainFrame->GetWidgetName());
  int dxw = vtkKWObject::GetIntegerResult(this->GetApplication());

  this->Script( "winfo height %s", this->MainFrame->GetWidgetName());
  int dyw = vtkKWObject::GetIntegerResult(this->GetApplication());

  if ((x >= xw) && (x<= xw+dxw) && (y >= yw) && (y <= yw + dyw))
    {
    this->QueryUserForColor();
    }  
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
     (this->DialogText?this->DialogText:"Chose Color"),
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
  this->ExtractRevision(os,"$Revision: 1.47 $");
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Text: " 
     << (this->GetText() ? this->GetText() : "(none)")
     << endl;

  os << indent << "DialogText: " 
     << (this->GetDialogText() ? this->GetDialogText() : "(none)")
     << endl;

  os << indent << "LabelAfterColor: " 
     << (this->LabelAfterColor ? "On\n" : "Off\n");

  os << indent << "LabelOutsideButton: " 
     << (this->LabelOutsideButton ? "On\n" : "Off\n");
}

