/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWChangeColorButton.h"

#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWChangeColorButton);
vtkCxxRevisionMacro(vtkKWChangeColorButton, "1.39.2.1");

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

  this->ColorButton = vtkKWWidget::New();
  this->MainFrame = vtkKWWidget::New();
  
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
void vtkKWChangeColorButton::SetColor(float r, float g, float b)
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
    vtkErrorMacro("Change color button already created");
    return;
    }

  this->SetApplication(app);
 
  // Do not call vtkKWLabeledWidget::Create() here since we have to create
  // the Label in a special way.

  // Create the container frame

  this->Script("frame %s -relief flat -bd 0 -highlightthickness 0 %s", 
               this->GetWidgetName(), args ? args : "");

  // Create the main frame

  this->MainFrame->SetParent(this);
  this->MainFrame->Create(this->Application, "frame", "-relief raised -bd 2");

  // Create the label.

  this->Label->SetParent(this->LabelOutsideButton ? this : this->MainFrame);
  this->Label->Create(app, "-padx 2 -pady 0 -highlightthickness 0 -bd 0");
  this->Label->SetLabel(this->Text);

  // Create the color button

  this->ColorButton->SetParent(this->MainFrame);
  this->ColorButton->Create(
    this->Application, "label", 
    "-bd 0 -highlightthickness 0 -padx 0 -pady 0 -width 2");

  this->UpdateColorButton();

  // Pack the whole stuff

  this->Pack();

  // Bind button presses

  this->Bind();
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();
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

  if (this->ShowLabel)
    { 
    this->Script("grid %s -row 0 -column %d -sticky news", 
                 this->Label->GetWidgetName(),
                 this->LabelAfterColor ? 1 : 0);
    this->Script("grid columnconfigure %s %d -weight 1", 
                 this->Label->GetParent()->GetWidgetName(),
                 this->LabelAfterColor ? 1 : 0);
    this->Script("grid columnconfigure %s %d -weight 0", 
                 this->Label->GetParent()->GetWidgetName(),
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
  if (!this->IsCreated())
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

  this->Script("bind %s <Any-ButtonPress> {+%s ButtonPressCallback %%X %%Y}",
               this->MainFrame->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback %%X %%Y}",
               this->MainFrame->GetWidgetName(), this->GetTclName());

  if (!this->LabelOutsideButton)
    {
    this->Script("bind %s <Any-ButtonPress> {+%s ButtonPressCallback %%X %%Y}",
                 this->Label->GetWidgetName(), this->GetTclName());
    this->Script("bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback %%X %%Y}",
                 this->Label->GetWidgetName(), this->GetTclName());
    }

  this->Script("bind %s <Any-ButtonPress> {+%s ButtonPressCallback %%X %%Y}",
               this->ColorButton->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonRelease> {+%s ButtonReleaseCallback %%X %%Y}",
               this->ColorButton->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::UnBind()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("bind %s <Any-ButtonPress> {}", 
               this->MainFrame->GetWidgetName());
  this->Script("bind %s <Any-ButtonRelease> {}", 
               this->MainFrame->GetWidgetName());

  if (!this->LabelOutsideButton)
    {
    this->Script("bind %s <Any-ButtonPress> {}", 
                 this->Label->GetWidgetName());
    this->Script("bind %s <Any-ButtonRelease> {}", 
                 this->Label->GetWidgetName());
    }

  this->Script("bind %s <Any-ButtonPress> {}",
               this->ColorButton->GetWidgetName());
  this->Script("bind %s <Any-ButtonRelease> {}", 
               this->ColorButton->GetWidgetName());
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
  int xw = vtkKWObject::GetIntegerResult(this->Application);

  this->Script( "winfo rooty %s", this->MainFrame->GetWidgetName());
  int yw = vtkKWObject::GetIntegerResult(this->Application);

  // Get the size and of the window

  this->Script( "winfo width %s", this->MainFrame->GetWidgetName());
  int dxw = vtkKWObject::GetIntegerResult(this->Application);

  this->Script( "winfo height %s", this->MainFrame->GetWidgetName());
  int dyw = vtkKWObject::GetIntegerResult(this->Application);

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

  this->Application->SetDialogUp(1);

  this->Script(
     "tk_chooseColor -initialcolor {#%02x%02x%02x} -title {%s} -parent %s",
     (int)(this->Color[0] * 255.5), 
     (int)(this->Color[1] * 255.5), 
     (int)(this->Color[2] * 255.5),
     (this->DialogText?this->DialogText:"Chose Color"),
     this->GetWidgetName() );

  result = this->Application->GetMainInterp()->result;

  this->Application->SetDialogUp(0);

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
    
    this->Color[0] = (float)r / 255.0;
    this->Color[1] = (float)g / 255.0;
    this->Color[2] = (float)b / 255.0;

    this->UpdateColorButton();

    if ( this->Command )
      {
      this->Script("eval %s %f %f %f", 
                   this->Command, 
                   this->Color[0], this->Color[1], this->Color[2]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWChangeColorButton::SetCommand( vtkKWObject* CalledObject, 
                                         const char *CommandString )
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  ostrstream command;
  command << CalledObject->GetTclName() << " " << CommandString << ends;

  this->Command = command.str();
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
  float clr[3];
  if (!strcmp(token,"Color"))
    {
    is >> clr[0] >> clr[1] >> clr[2];
    this->SetColor(clr);
    if ( this->Command )
      {
      this->Script("eval %s %f %f %f", this->Command, 
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
  this->ExtractRevision(os,"$Revision: 1.39.2.1 $");
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

