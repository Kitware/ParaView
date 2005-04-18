/*=========================================================================

  Module:    vtkKWMessageDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMessageDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkKWRegistryHelper.h"
#include "vtkKWWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMessageDialog );
vtkCxxRevisionMacro(vtkKWMessageDialog, "1.70");

//----------------------------------------------------------------------------
int vtkKWMessageDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWMessageDialog::vtkKWMessageDialog()
{
  this->MessageDialogFrame = vtkKWFrame::New();
  this->TopFrame           = vtkKWFrame::New();
  this->BottomFrame        = vtkKWFrame::New();
  this->CommandFunction    = vtkKWMessageDialogCommand;
  this->Label              = vtkKWLabel::New();
  this->CheckButton        = vtkKWCheckButton::New();
  this->ButtonFrame        = vtkKWFrame::New();
  this->OKFrame            = vtkKWFrame::New();
  this->CancelFrame        = vtkKWFrame::New();
  this->OtherFrame         = vtkKWFrame::New();
  this->OKButton           = vtkKWPushButton::New();
  this->CancelButton       = vtkKWPushButton::New();
  this->OtherButton        = vtkKWPushButton::New();
  this->Style              = vtkKWMessageDialog::Message;
  this->Icon               = vtkKWLabel::New();

  this->DialogName = 0;
  this->Options    = 0;
  this->DialogText = 0;

  this->OKButtonText = 0;
  this->SetOKButtonText("OK");

  this->CancelButtonText = 0;
  this->SetCancelButtonText("Cancel");

  this->OtherButtonText = 0;
  this->SetOtherButtonText("Other");
}

//----------------------------------------------------------------------------
vtkKWMessageDialog::~vtkKWMessageDialog()
{
  this->Label->Delete();
  this->CheckButton->Delete();
  this->ButtonFrame->Delete();
  this->OKFrame->Delete();
  this->CancelFrame->Delete();
  this->OtherFrame->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();
  this->OtherButton->Delete();
  this->Icon->Delete();
  this->MessageDialogFrame->Delete();
  this->TopFrame->Delete();
  this->BottomFrame->Delete();
  this->SetDialogName(0);
  this->SetDialogText(0);
  this->SetOKButtonText(0);
  this->SetCancelButtonText(0);
  this->SetOtherButtonText(0);
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("MessageDialog already created");
    return;
    }

  this->Superclass::Create(app,args);
  
  this->MessageDialogFrame->SetParent(this);
  this->MessageDialogFrame->Create(app, NULL);

  this->Script("pack %s -side right -fill both -expand true -pady 0",
               this->MessageDialogFrame->GetWidgetName());

  this->TopFrame->SetParent(this->MessageDialogFrame);
  this->TopFrame->Create(app, NULL);

  this->Script("pack %s -side top -fill both -expand true",
               this->TopFrame->GetWidgetName());

  this->BottomFrame->SetParent(this->MessageDialogFrame);
  this->BottomFrame->Create(app, NULL);

  this->Script("pack %s -side top -fill both -expand true",
               this->BottomFrame->GetWidgetName());

  this->Label->SetParent(this->MessageDialogFrame);
  this->Label->SetLineType(vtkKWLabel::MultiLine);
  this->Label->SetWidth(300);
  this->Label->Create(app,NULL);
  if ( this->DialogText )
    {
    this->Label->SetText(this->DialogText);
    }

  this->Script("pack %s -side top -fill x -padx 20 -pady 5",
               this->Label->GetWidgetName());

  this->CheckButton->SetParent(this->MessageDialogFrame);
  this->CheckButton->Create(app, NULL);

  if ( this->GetDialogName() )
    {
    this->CheckButton->SetText("Do not show this dialog anymore.");
    this->Script("pack %s -side top -fill x -padx 20 -pady 5",
                 this->CheckButton->GetWidgetName());
    }

  this->ButtonFrame->SetParent(this->MessageDialogFrame);
  this->ButtonFrame->Create(app, NULL);
  
  this->Script("pack %s -side top -fill x -pady 2 -expand y",
               this->ButtonFrame->GetWidgetName());

  int one_b = (this->Style == vtkKWMessageDialog::Message) ? 1 : 0;
  int two_b = (this->Style == vtkKWMessageDialog::YesNo || 
               this->Style == vtkKWMessageDialog::OkCancel) ? 2 : 0;
  int three_b = (this->Style == vtkKWMessageDialog::OkOtherCancel)  ? 3 : 0;
  int nb_buttons = one_b + two_b + three_b;

  if (this->Style == vtkKWMessageDialog::YesNo)
    {
    this->SetOKButtonText("Yes");
    this->SetCancelButtonText("No");
    }
  else if (this->Style == vtkKWMessageDialog::Message)
    {
    this->SetOKButtonText("OK");
    }

  // Pack buttons

  ostrstream pack_opt;
  if (this->Options & vtkKWMessageDialog::PackVertically)
    {
    pack_opt << "-side top -expand yes -fill x -padx 4" << ends;
    }
  else
    {
    pack_opt << "-side left -expand yes -padx 2" << ends;
    }

  if (nb_buttons >= 1)
    {
    this->OKFrame->SetParent(this->ButtonFrame);
    this->OKFrame->Create(app, "-bd 3 -relief flat");
    this->OKButton->SetParent(this->OKFrame);
    this->OKButton->Create(app, "-width 16");
    this->OKButton->SetTextOption(this->OKButtonText);
    this->OKButton->SetCommand(this, "OK");
    this->Script("pack %s %s %s",
                 this->OKButton->GetWidgetName(),
                 this->OKFrame->GetWidgetName(), pack_opt.str());
    }

  if (nb_buttons >= 3)
    {
    this->OtherFrame->SetParent(this->ButtonFrame);  
    this->OtherFrame->Create(app, "-bd 3 -relief flat");
    this->OtherButton->SetParent(this->OtherFrame);
    this->OtherButton->Create(app, "-width 16");
    this->OtherButton->SetTextOption(this->OtherButtonText);
    this->OtherButton->SetCommand(this, "Other");
    this->Script("pack %s %s %s",
                 this->OtherButton->GetWidgetName(),
                 this->OtherFrame->GetWidgetName(), pack_opt.str());
    }

  if (nb_buttons >= 2)
    {
    this->CancelFrame->SetParent(this->ButtonFrame);  
    this->CancelFrame->Create(app, "-bd 3 -relief flat");
    this->CancelButton->SetParent(this->CancelFrame);
    this->CancelButton->Create(app, "-width 16");
    this->CancelButton->SetTextOption(this->CancelButtonText);
    this->CancelButton->SetCommand(this, "Cancel");
    this->Script("pack %s %s %s",
                 this->CancelButton->GetWidgetName(),
                 this->CancelFrame->GetWidgetName(), pack_opt.str());
    }

  pack_opt.rdbuf()->freeze(0);

  // Configure button aspect

  if (this->OKButton->IsCreated())
    {
    this->OKButton->SetBind(
      "<FocusIn>", this->OKFrame->GetWidgetName(), 
      "configure -relief groove");
    this->OKButton->SetBind(
      "<FocusOut>", this->OKFrame->GetWidgetName(), 
      "configure -relief flat");
    this->OKButton->SetBind(this, "<Return>", "OK");
    }

  if (this->CancelButton->IsCreated())
    {
    this->CancelButton->SetBind(
      "<FocusIn>", this->CancelFrame->GetWidgetName(), 
      "configure -relief groove");
    this->CancelButton->SetBind(
      "<FocusOut>", this->CancelFrame->GetWidgetName(),
      "configure -relief flat");
    this->CancelButton->SetBind(this, "<Return>", "Cancel");
    }

  if (this->OtherButton->IsCreated())
    {
    this->OtherButton->SetBind(
      "<FocusIn>", this->OtherFrame->GetWidgetName(), 
      "configure -relief groove");
    this->OtherButton->SetBind(
      "<FocusOut>", this->OtherFrame->GetWidgetName(),
      "configure -relief flat");
    this->OtherButton->SetBind(this, "<Return>", "Other");
    }

  // Icon
  
  this->Icon->SetParent(this);
  this->Icon->Create(app,"-width 0 -pady 0 -padx 0 -bd 0");
  this->Script("pack %s -side left -fill y",
               this->Icon->GetWidgetName());
  this->Script("pack forget %s", this->Icon->GetWidgetName());

#ifdef KW_MESSAGEDIALOG_DEBUG
  this->Script("%s configure -bg red -height 5", this->TopFrame->GetWidgetName());
  this->Script("%s configure -bg green", this->MessageDialogFrame->GetWidgetName());
  this->Script("%s configure -bg blue -height 5", this->BottomFrame->GetWidgetName());
  this->Script("%s configure -bg purple", this->ButtonFrame->GetWidgetName());
#endif
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::SetText(const char *txt)
{
  this->SetDialogText(txt);
  if (this->Label)
    {
    this->Label->SetText(this->DialogText);
    }
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::SetTextWidth(int w)
{
  if (this->Label)
    {
    this->Label->SetWidth(w);
    }
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::GetTextWidth()
{
  if (this->Label)
    {
    return this->Label->GetWidth();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::Invoke()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  this->InvokeEvent(vtkKWEvent::MessageDialogInvokeEvent, this->DialogText);

  if ( this->DialogName )
    {
    int res = this->GetMessageDialogResponseFromRegistry(
      this->GetApplication(), this->DialogName);
    if ( res == 1 )
      {
      return 1;
      }
    if ( res == -1 )
      {
      return 0;
      }
    }
  
  if( this->Options & vtkKWMessageDialog::NoDefault ||
           this->Options & vtkKWMessageDialog::CancelDefault )
    {
    this->CancelButton->Focus();
    } 
  else if ( this->Options & vtkKWMessageDialog::YesDefault ||
            this->Options & vtkKWMessageDialog::OkDefault )
    {
    this->OKButton->Focus();
    }
  if (this->OKButton->IsCreated() && this->CancelButton->IsCreated())
    {
    this->OKButton->SetBind("<Right>", "focus [ tk_focusNext %W ]");
    this->OKButton->SetBind("<Left>",  "focus [ tk_focusPrev %W ]");
    this->CancelButton->SetBind("<Right>", "focus [ tk_focusNext %W ]");
    this->CancelButton->SetBind("<Left>",  "focus [ tk_focusPrev %W ]");
    if (this->OtherButton->IsCreated())
      {
      this->OtherButton->SetBind("<Right>", "focus [ tk_focusNext %W ]");
      this->OtherButton->SetBind("<Left>",  "focus [ tk_focusPrev %W ]");
      }
    }
  if (this->Options & vtkKWMessageDialog::InvokeAtPointer)
    {
    this->InvokeAtPointerOn();
    }

  this->Script("wm resizable %s 0 0", this->GetWidgetName());

  int res = this->Superclass::Invoke();

  if ( this->DialogName && this->GetRememberMessage() )
    {
    int ires = res;
    if ( this->Options & vtkKWMessageDialog::RememberYes )
      {
      ires = 1;
      }
    else if ( this->Options & vtkKWMessageDialog::RememberNo )
      {
      ires = -1;
      }
    else
      {
      if ( !ires )
        {
        ires = -1;
        }
      }
    this->StoreMessageDialogResponseInRegistry(
      this->GetApplication(), this->DialogName, ires);
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::SetIcon()
{
  if ( this->Options & vtkKWMessageDialog::ErrorIcon )
    {
    this->Icon->SetImageOption(vtkKWIcon::ICON_ERROR);
    }
  else if ( this->Options & vtkKWMessageDialog::QuestionIcon )
    {
    this->Icon->SetImageOption(vtkKWIcon::ICON_QUESTION);
    }
  else if ( this->Options & vtkKWMessageDialog::WarningIcon )
    {
    this->Icon->SetImageOption(vtkKWIcon::ICON_WARNING);
    }
  else
    {
    this->Script("%s configure -width 0 -pady 0 -padx 0 -bd 0",
                 this->Icon->GetWidgetName());
    this->Script("pack forget %s", this->Icon->GetWidgetName());
    return;
    }  
  
  this->Script("%s configure -anchor n "
               "-pady 5 -padx 4 -bd 4",
               this->Icon->GetWidgetName());
  this->Script("pack %s -pady 17 -side left -fill y", 
               this->Icon->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::PopupMessage(vtkKWApplication *app, vtkKWWindow *win,
                                      const char* title, 
                                      const char*message, int options)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetMasterWindow(win);
  dlg2->SetOptions(
    options | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->Create(app,NULL);
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon();
  dlg2->BeepOn();
  dlg2->Invoke();
  dlg2->Delete();
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::PopupYesNo(vtkKWApplication *app, vtkKWWindow *win,
                                   const char* name, 
                                   const char* title, const char* message,
                                   int options)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetStyleToYesNo();
  dlg2->SetMasterWindow(win);
  dlg2->SetOptions(
    options | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->SetDialogName(name);
  dlg2->Create(app,NULL);
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon();
  dlg2->BeepOn();
  int ret = dlg2->Invoke();
  dlg2->Delete();
  return ret;
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::PopupYesNo(vtkKWApplication *app, vtkKWWindow *win,
                                   const char* title, 
                                   const char*message, int options)
{
  return vtkKWMessageDialog::PopupYesNo(app, win, 0, title, message, 
                                        options);
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::PopupOkCancel(vtkKWApplication *app, vtkKWWindow *win,
                                      const char* title, 
                                      const char*message, int options)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetStyleToOkCancel();
  dlg2->SetOptions(
    options | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->SetMasterWindow(win);
  dlg2->Create(app,NULL);
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon();
  int ret = dlg2->Invoke();
  dlg2->Delete();
  return ret;
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::GetRememberMessage()
{
  int res = this->CheckButton->GetState();
  return res;
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::Other()
{
  this->Withdraw();
  this->ReleaseGrab();
  this->Done = 3;  
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::GetWidth()
{
  int width = this->Superclass::GetWidth();

  if (!this->IsCreated() || this->GetHasBeenMapped())
    {
    return width;
    }

  // GetWidth() is called by vtkKWDialog when it's time to center the
  // dialog. Unfortunately, since the widget has not been mapped at that
  // time, 'winfo width' will most probably return 1, and 'winfo reqwidth'
  // will return the default size of the top level window. Let's try to
  // help a bit by checking the size of the internal frames (which width can
  // be set explicitly).

  if (this->TopFrame)
    {
    int iwidth = atoi(this->Script("winfo reqwidth %s", 
                                   this->TopFrame->GetWidgetName()));
    if (iwidth > width)
      {
      width = iwidth;
      }
    }

  if (this->MessageDialogFrame)
    {
    int iwidth = atoi(this->Script("winfo reqwidth %s", 
                                   this->MessageDialogFrame->GetWidgetName()));
    if (iwidth > width)
      {
      width = iwidth;
      }
    }

  if (this->BottomFrame)
    {
    int iwidth = atoi(this->Script("winfo reqwidth %s", 
                                   this->BottomFrame->GetWidgetName()));
    if (iwidth > width)
      {
      width = iwidth;
      }
    }

  return width;
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::GetHeight()
{
  int height = this->Superclass::GetHeight();

  if (!this->IsCreated() || this->GetHasBeenMapped())
    {
    return height;
    }

  // GetHeight() is called by vtkKWDialog when it's time to center the
  // dialog. Unfortunately, since the widget has not been mapped at that
  // time, 'winfo height' will most probably return 1, and 'winfo reqheight'
  // will return the default size of the top level window. Let's try to help
  // a bit by checking the size of the internal frames (which height can be
  // set explicitly).

  if (this->TopFrame)
    {
    height += atoi(this->Script("winfo reqheight %s", 
                                this->TopFrame->GetWidgetName()));
    }

  if (this->MessageDialogFrame)
    {
    height += atoi(this->Script("winfo reqheight %s", 
                                this->MessageDialogFrame->GetWidgetName()));
    }

  if (this->BottomFrame)
    {
    height += atoi(this->Script("winfo reqheight %s", 
                                this->BottomFrame->GetWidgetName()));
    }

  return height;
}

//----------------------------------------------------------------------------
int vtkKWMessageDialog::GetMessageDialogResponseFromRegistry(
  vtkKWApplication *app,
  const char* dialogname)
{
  char buffer[REG_KEY_VALUE_SIZE_MAX];
  int retval = 0;
  if (app && dialogname && 
      app->GetRegistryValue(3, "Dialogs", dialogname, buffer))
    {
    retval = atoi(buffer);
    }
  return retval;
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::StoreMessageDialogResponseInRegistry(
  vtkKWApplication *app,
  const char* dialogname, 
  int response)
{
  if (app && dialogname)
    {
    app->SetRegistryValue(3, "Dialogs", dialogname, "%d", response);
    }
}

//----------------------------------------------------------------------------
void vtkKWMessageDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DialogName: " << (this->DialogName?this->DialogName:"none")
     << endl;
  os << indent << "Options: " << this->GetOptions() << endl;
  os << indent << "Style: " << this->GetStyle() << endl;
  os << indent << "MessageDialogFrame: " << this->MessageDialogFrame << endl;
  os << indent << "OKButtonText: " << (this->OKButtonText?
                                       this->OKButtonText:"none") << endl;
  os << indent << "CancelButtonText: " << (this->CancelButtonText?
                                       this->CancelButtonText:"none") << endl;
  os << indent << "OtherButtonText: " << (this->OtherButtonText?
                                       this->OtherButtonText:"none") << endl;
  os << indent << "DialogName: " << (this->DialogName?this->DialogName:"none")
     << endl;
  os << indent << "TopFrame: " << this->TopFrame << endl;
  os << indent << "BottomFrame: " << this->BottomFrame << endl;
  os << indent << "OKButton: " << this->OKButton << endl;
  os << indent << "CancelButton: " << this->CancelButton << endl;
  os << indent << "OtherButton: " << this->OtherButton << endl;
}
