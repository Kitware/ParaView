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
#include "vtkKWTopLevel.h"

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWPopupButton);
vtkCxxRevisionMacro(vtkKWPopupButton, "1.19");

int vtkKWPopupButtonCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPopupButton::vtkKWPopupButton()
{
  this->CommandFunction = vtkKWPopupButtonCommand;

  this->PopupTopLevel = vtkKWTopLevel::New();

  this->PopupFrame = vtkKWFrame::New();

  this->PopupCloseButton = vtkKWPushButton::New();

  this->WithdrawCommand = 0;
}

//----------------------------------------------------------------------------
vtkKWPopupButton::~vtkKWPopupButton()
{
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

  // Create  top level window

  this->PopupTopLevel->SetMasterWindow(this);
  this->PopupTopLevel->Create(app, "-bd 2 -relief flat");
  this->PopupTopLevel->Withdraw();

  if (!this->PopupTopLevel->GetTitle())
    {
    this->PopupTopLevel->SetTitle(
      this->Script("wm title [winfo toplevel %s]", this->GetWidgetName()));
    }

  this->PopupTopLevel->SetDeleteWindowProtocolCommand(
    this, "WithdrawPopupCallback");

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

  // Bind the button so that it popups the top level window

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
  this->PopupTopLevel->SetTitle(title);
}

// ---------------------------------------------------------------------------
void vtkKWPopupButton::DisplayPopupCallback()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Get the position of the mouse, the size of the top level window.

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

  this->PopupTopLevel->SetPosition(px, py);
  this->PopupTopLevel->DeIconify();
  this->PopupTopLevel->Raise();
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

  this->PopupTopLevel->Withdraw();
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

  this->PropagateEnableState(this->PopupTopLevel);
  this->PropagateEnableState(this->PopupFrame);
  this->PropagateEnableState(this->PopupCloseButton);

  // Now given the state, bind or unbind

  if (this->IsCreated())
    {
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

//----------------------------------------------------------------------------
void vtkKWPopupButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PopupTopLevel: " << this->PopupTopLevel << endl;
  os << indent << "PopupFrame: " << this->PopupFrame << endl;
  os << indent << "PopupCloseButton: " << this->PopupCloseButton << endl;
  os << indent << "WithdrawCommand: "
     << (this->WithdrawCommand ? this->WithdrawCommand : "(none)") << endl;
}

