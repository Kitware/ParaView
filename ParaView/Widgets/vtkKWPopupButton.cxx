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
#include "vtkKWPopupButton.h"

#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWPopupButton);
vtkCxxRevisionMacro(vtkKWPopupButton, "1.9");

int vtkKWPopupButtonCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPopupButton::vtkKWPopupButton()
{
  this->CommandFunction = vtkKWPopupButtonCommand;

  this->PopupTopLevel = vtkKWWidget::New();

  this->PopupFrame = vtkKWWidget::New();

  this->PopupCloseButton = vtkKWPushButton::New();

  this->PopupTitle = 0;
}

//----------------------------------------------------------------------------
vtkKWPopupButton::~vtkKWPopupButton()
{
  this->SetPopupTitle(0);

  if (this->PopupTopLevel)
    {
    this->PopupTopLevel->Delete();
    this->PopupTopLevel = NULL;
    }

  if (this->PopupFrame)
    {
    this->PopupFrame->Delete();
    this->PopupFrame = NULL;
    }

  if (this->PopupCloseButton)
    {
    this->PopupCloseButton->Delete();
    this->PopupCloseButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupButton already created");
    return;
    }

  // Call the superclass, this will set the application and 
  // create the pushbutton.

  this->Superclass::Create(app, args);

  ostrstream tk_cmd;

  // Create the toplevel

  this->PopupTopLevel->Create(app, "toplevel", "-bd 2 -relief flat");

  tk_cmd << "wm withdraw " << this->PopupTopLevel->GetWidgetName() << endl
         << "wm title " << this->PopupTopLevel->GetWidgetName() 
         << " [wm title [winfo toplevel " 
         << this->GetWidgetName() << "]]"<< endl
         << "wm transient " << this->PopupTopLevel->GetWidgetName() 
         << " " << this->GetWidgetName() << endl
         << "wm protocol " << this->PopupTopLevel->GetWidgetName()
         << " WM_DELETE_WINDOW {" 
         << this->GetTclName() << " WithdrawPopupCallback}" << endl;

  // Create the frame

  this->PopupFrame->SetParent(PopupTopLevel);
  this->PopupFrame->Create(app, "frame", "-bd 2 -relief flat");

  tk_cmd << "pack " << this->PopupFrame->GetWidgetName() 
         << " -side top -expand y -fill both" << endl;

  // Create the close button

  this->PopupCloseButton->SetParent(PopupTopLevel);
  this->PopupCloseButton->Create(app, 0);
  this->PopupCloseButton->SetLabel("Close");

  tk_cmd << "pack " << this->PopupCloseButton->GetWidgetName() 
         << " -side top -expand false -fill x -pady 2" << endl;

  // Pack, bind

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
  
  if ( this->PopupTitle )
    {
    this->Script("wm title %s {%s}", 
                 this->PopupTopLevel->GetWidgetName(), this->PopupTitle);
    }

  this->Bind();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWPopupButton::Bind()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Bind the button so that it popups the toplevel

  this->Script("bind %s <ButtonPress> {%s DisplayPopupCallback}",
               this->GetWidgetName(), this->GetTclName());

  if (this->PopupCloseButton && this->PopupCloseButton->IsCreated())
    {
    this->Script("bind %s <ButtonPress> {%s WithdrawPopupCallback}",
                 this->PopupCloseButton->GetWidgetName(), this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupButton::UnBind()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("bind %s <ButtonPress> {}", 
               this->GetWidgetName());

  if (this->PopupCloseButton)
    {
    this->Script("bind %s <ButtonPress> {}", 
                 this->PopupCloseButton->GetWidgetName());
    }
}

// ---------------------------------------------------------------------------
void vtkKWPopupButton::SetPopupTitle(const char* title)
{
  if ( this->PopupTitle == title )
    {
    return;
    }

  if ( vtkString::Equals(this->PopupTitle, title) )
    {
    return;
    }

  if ( this->PopupTitle )
    {
    delete [] this->PopupTitle;
    this->PopupTitle = 0;
    }

  if ( title )
    {
    this->PopupTitle = vtkString::Duplicate(title);

    if (this->IsCreated() && this->PopupTopLevel->IsCreated())
      {
      this->Script("wm title %s {%s}", 
                   this->PopupTopLevel->GetWidgetName(), this->PopupTitle);
      }
    }
}

// ---------------------------------------------------------------------------
void vtkKWPopupButton::DisplayPopupCallback()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Get the position of the mouse, the size of the toplevel.

  this->Script("concat "
               " [winfo pointerx %s] [winfo pointery %s]" 
               " [winfo reqwidth %s] [winfo reqheight %s]"
               " [winfo screenwidth %s] [winfo screenheight %s]",
               this->GetWidgetName(), 
               this->GetWidgetName(),
               this->PopupTopLevel->GetWidgetName(), 
               this->PopupTopLevel->GetWidgetName(),
               this->GetWidgetName(), 
               this->GetWidgetName());
  
  int px, py, tw, th, sw, sh;
  sscanf(this->Application->GetMainInterp()->result, 
         "%d %d %d %d %d %d", &px, &py, &tw, &th, &sw, &sh);

  px -= tw / 2;
  if (px + tw > sw)
    {
    px -= (px + tw - sw);
    }
  if (px < 0)
    {
    px = 0;
    }

  py -= th / 2;
  if (py + th > sh)
    {
    py -= (py + th - sh);
    }
  if (py < 0)
    {
    py = 0;
    }

  this->Script("wm geometry %s +%d+%d",
               this->PopupTopLevel->GetWidgetName(), px, py);

  this->Script("wm deiconify %s", 
               this->PopupTopLevel->GetWidgetName());
  
  this->Script("raise %s", 
               this->PopupTopLevel->GetWidgetName());
}

// ---------------------------------------------------------------------------
void vtkKWPopupButton::WithdrawPopupCallback()
{
  if ( this->Application->GetDialogUp() )
    {
    this->Script("bell");
    return;
    }
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("wm withdraw %s",
               this->PopupTopLevel->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWPopupButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PopupTopLevel)
    {
    this->PopupTopLevel->SetEnabled(this->Enabled);
    }

  if (this->PopupFrame)
    {
    this->PopupFrame->SetEnabled(this->Enabled);
    }

  if (this->PopupCloseButton)
    {
    this->PopupCloseButton->SetEnabled(this->Enabled);
    }

  // Now given the state, bind or unbind

  if (this->IsCreated())
    {
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

//----------------------------------------------------------------------------
void vtkKWPopupButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PopupTopLevel: " << this->PopupTopLevel << endl;
  os << indent << "PopupFrame: " << this->PopupFrame << endl;
  os << indent << "PopupCloseButton: " << this->PopupCloseButton << endl;
  os << indent << "PopupTitle: " 
     << (this->PopupTitle ? this->PopupTitle : "(none)") << endl;
}

