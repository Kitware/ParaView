/*=========================================================================

  Module:    vtkKWPopupButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWPopupButton.h"

#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWPopupButton);
vtkCxxRevisionMacro(vtkKWPopupButton, "1.17");

int vtkKWPopupButtonCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPopupButton::vtkKWPopupButton()
{
  this->CommandFunction = vtkKWPopupButtonCommand;

  this->PopupTopLevel = vtkKWWidget::New();

  this->PopupFrame = vtkKWFrame::New();

  this->PopupCloseButton = vtkKWPushButton::New();

  this->PopupTitle = 0;

  this->WithdrawCommand = 0;
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

  this->SetWithdrawCommand(0);
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
  this->PopupFrame->Create(app, "-bd 2 -relief flat");

  tk_cmd << "pack " << this->PopupFrame->GetWidgetName() 
         << " -side top -expand y -fill both" << endl;

  // Create the close button

  this->PopupCloseButton->SetParent(PopupTopLevel);
  this->PopupCloseButton->Create(app, 0);
  this->PopupCloseButton->SetText("Close");

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

  if (this->PopupCloseButton && this->PopupCloseButton->IsCreated())
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

  if (this->PopupTitle && title && !strcmp(this->PopupTitle, title))
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
    this->PopupTitle = kwsys::SystemTools::DuplicateString(title);

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

  const char *res = 
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
  sscanf(res, "%d %d %d %d %d %d", &px, &py, &tw, &th, &sw, &sh);

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
  if ( this->GetApplication()->GetDialogUp() )
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
  if ( this->WithdrawCommand )
    {
    this->Script(this->WithdrawCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupButton::SetWithdrawCommand(vtkKWObject* obj, const char* command)
{
  ostrstream ostr;
  ostr << obj->GetTclName() << " " << command;
  this->SetWithdrawCommand(ostr.str());
  ostr.rdbuf()->freeze(0);
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
  os << indent << "WithdrawCommand: "
     << (this->WithdrawCommand ? this->WithdrawCommand : "(none)") << endl;
}

