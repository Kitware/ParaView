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
#include "vtkKWMessageDialog.h"

#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWIcon.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMessageDialog );
vtkCxxRevisionMacro(vtkKWMessageDialog, "1.50.2.2");




int vtkKWMessageDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWMessageDialog::vtkKWMessageDialog()
{
  this->MessageDialogFrame = vtkKWWidget::New();
  this->MessageDialogFrame->SetParent(this);
  this->TopFrame = vtkKWWidget::New();
  this->TopFrame->SetParent(this->MessageDialogFrame);
  this->BottomFrame = vtkKWWidget::New();
  this->BottomFrame->SetParent(this->MessageDialogFrame);
  
  this->CommandFunction = vtkKWMessageDialogCommand;
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this->MessageDialogFrame);
  this->CheckButton = vtkKWCheckButton::New();
  this->CheckButton->SetParent(this->MessageDialogFrame);
  this->ButtonFrame = vtkKWWidget::New();
  this->ButtonFrame->SetParent(this->MessageDialogFrame);
  this->OKFrame = vtkKWWidget::New();
  this->OKFrame->SetParent(this->ButtonFrame);
  this->CancelFrame = vtkKWWidget::New();
  this->CancelFrame->SetParent(this->ButtonFrame);  
  this->OKButton = vtkKWWidget::New();
  this->OKButton->SetParent(this->OKFrame);
  this->CancelButton = vtkKWWidget::New();
  this->CancelButton->SetParent(this->CancelFrame);
  this->Style = vtkKWMessageDialog::Message;
  this->Icon = vtkKWLabel::New();
  this->Icon->SetParent(this);
  this->DialogName = 0;
  this->Options    = 0;
  this->DialogText = 0;
  this->OKButtonText = 0;
  this->SetOKButtonText("OK");
  this->CancelButtonText = 0;
  this->SetCancelButtonText("Cancel");
}

vtkKWMessageDialog::~vtkKWMessageDialog()
{
  this->Label->Delete();
  this->CheckButton->Delete();
  this->ButtonFrame->Delete();
  this->OKFrame->Delete();
  this->CancelFrame->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();
  this->Icon->Delete();
  this->MessageDialogFrame->Delete();
  this->TopFrame->Delete();
  this->BottomFrame->Delete();
  this->SetDialogName(0);
  this->SetDialogText(0);
  this->SetOKButtonText(0);
  this->SetCancelButtonText(0);
}

void vtkKWMessageDialog::Create(vtkKWApplication *app, const char *args)
{
  // invoke super method
  this->Superclass::Create(app,args);
  
  this->MessageDialogFrame->Create(app,"frame","");
  this->TopFrame->Create(app,"frame","");
  this->BottomFrame->Create(app,"frame","");
  this->Label->SetLineType(vtkKWLabel::MultiLine);
  this->Label->SetWidth(300);
  this->Label->Create(app,"");
  if ( this->DialogText )
    {
    this->Label->SetLabel(this->DialogText);
    }
  this->CheckButton->Create(app, "");
  this->ButtonFrame->Create(app,"frame","");
  
  switch (this->Style)
    {
    case vtkKWMessageDialog::Message :
      this->OKFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      this->OKButton->Create(app,"button","-text OK -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->Script("pack %s -side left -expand yes",
                   this->OKButton->GetWidgetName());
      this->Script("pack %s -side left -padx 2 -expand yes",
                   this->OKFrame->GetWidgetName());
      break;
    case vtkKWMessageDialog::YesNo: 
      this->SetOKButtonText("Yes");
      this->SetCancelButtonText("No");
    case vtkKWMessageDialog::OkCancel:
      this->OKFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      ostrstream oktext;
      oktext << "-text " << this->OKButtonText << " -width 16" << ends;
      this->OKButton->Create(app, "button", oktext.str());
      oktext.rdbuf()->freeze(0);
      this->OKButton->SetCommand(this, "OK");
      this->CancelFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      ostrstream canceltext;
      canceltext << "-text " << this->CancelButtonText << " -width 16" << ends;
      this->CancelButton->Create(app,"button", canceltext.str());
      canceltext.rdbuf()->freeze(0);
      this->CancelButton->SetCommand(this, "Cancel");
      this->Script("pack %s %s -side left -expand yes",
                   this->OKButton->GetWidgetName(),
                   this->CancelButton->GetWidgetName());
      this->Script("pack %s %s -side left -padx 2 -expand yes",
                   this->OKFrame->GetWidgetName(),
                   this->CancelFrame->GetWidgetName());
      break;
    }

  if ( this->OKButton->GetApplication() )
    {
    this->OKButton->SetBind("<FocusIn>", this->OKFrame->GetWidgetName(), 
                            "configure -relief groove");
    this->OKButton->SetBind("<FocusOut>", this->OKFrame->GetWidgetName(), 
                            "configure -relief flat");
    {
    this->OKButton->SetBind(this, "<Return>", "OK");
    }
    }
  if ( this->CancelButton->GetApplication() )
    {
    this->CancelButton->SetBind(
      "<FocusIn>", this->CancelFrame->GetWidgetName(), 
      "configure -relief groove");
    this->CancelButton->SetBind(
      "<FocusOut>", this->CancelFrame->GetWidgetName(),
      "configure -relief flat");
    {
    this->CancelButton->SetBind(this, "<Return>", "Cancel");
    }
    }
  
  this->Script("pack %s -side right -fill both -expand true -pady 4",
               this->MessageDialogFrame->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand true",
               this->TopFrame->GetWidgetName());
  this->Script("pack %s -side top -fill x -padx 20 -pady 5",
               this->Label->GetWidgetName());
  if ( this->GetDialogName() )
    {
    this->CheckButton->SetText("Do not show this dialog anymore.");
    this->Script("pack %s -side top -fill x -padx 20 -pady 5",
                 this->CheckButton->GetWidgetName());
    }

  this->Script("pack %s -side top -fill both -expand true",
               this->BottomFrame->GetWidgetName());
  this->Script("pack %s -side top -fill x -pady 2",
               this->ButtonFrame->GetWidgetName());
  this->Icon->Create(app,"-width 0 -pady 0 -padx 0 -borderwidth 0");
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

void vtkKWMessageDialog::SetText(const char *txt)
{
  this->SetDialogText(txt);
  if ( this->Label )
    {
    this->Label->SetLabel(this->DialogText);
    }
}

int vtkKWMessageDialog::Invoke()
{
  if ( !this->Application )
    {
    return 0;
    }
  this->InvokeEvent(vtkKWEvent::MessageDialogInvokeEvent, 
                    this->DialogText);
  if ( !this->GetApplication()->GetUseMessageDialogs() )
    {
    return 0;
    }

  if ( this->DialogName )
    {
    int res = this->Application->GetMessageDialogResponse(this->DialogName);
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
  if ( this->OKButton->IsCreated() && this->CancelButton->IsCreated() )
    {
    this->OKButton->SetBind("<Right>", "focus [ tk_focusNext %W ]");
    this->OKButton->SetBind("<Left>",  "focus [ tk_focusPrev %W ]");
    this->CancelButton->SetBind("<Right>", "focus [ tk_focusNext %W ]");
    this->CancelButton->SetBind("<Left>",  "focus [ tk_focusPrev %W ]");
    }
  
  this->Script("wm resizable %s 0 0", this->GetWidgetName());

  int res = vtkKWDialog::Invoke();
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
    this->Application->SetMessageDialogResponse(this->DialogName, ires);
    }
  return res;
}

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
    this->Script("%s configure -width 0 -pady 0 -padx 0 -borderwidth 0",
                 this->Icon->GetWidgetName());
    this->Script("pack forget %s", this->Icon->GetWidgetName());
    return;
    }  
  
  this->Script("%s configure -anchor n "
               "-pady 5 -padx 4 -borderwidth 4",
               this->Icon->GetWidgetName());
  this->Script("pack %s -pady 17 -side left -fill y", 
               this->Icon->GetWidgetName());
}

void vtkKWMessageDialog::PopupMessage(vtkKWApplication *app, vtkKWWindow *win,
                                      const char* title, 
                                      const char*message, int options)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetMasterWindow(win);
  dlg2->SetOptions(
    options | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->Create(app,"");
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon();
  dlg2->BeepOn();
  dlg2->Invoke();
  dlg2->Delete();
}

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
  dlg2->Create(app,"");
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon();
  dlg2->BeepOn();
  int ret = dlg2->Invoke();
  dlg2->Delete();
  return ret;
}

int vtkKWMessageDialog::PopupYesNo(vtkKWApplication *app, vtkKWWindow *win,
                                   const char* title, 
                                   const char*message, int options)
{
  return vtkKWMessageDialog::PopupYesNo(app, win, 0, title, message, 
                                        options);
}

int vtkKWMessageDialog::PopupOkCancel(vtkKWApplication *app, vtkKWWindow *win,
                                      const char* title, 
                                      const char*message, int options)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetStyleToOkCancel();
  dlg2->SetOptions(
    options | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->SetMasterWindow(win);
  dlg2->Create(app,"");
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon();
  int ret = dlg2->Invoke();
  dlg2->Delete();
  return ret;
}

int vtkKWMessageDialog::GetRememberMessage()
{
  int res = this->CheckButton->GetState();
  return res;
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
  os << indent << "DialogName: " << (this->DialogName?this->DialogName:"none")
     << endl;
  os << indent << "TopFrame: " << this->TopFrame << endl;
  os << indent << "BottomFrame: " << this->BottomFrame << endl;
  os << indent << "OKButton: " << this->OKButton << endl;
  os << indent << "CancelButton: " << this->CancelButton << endl;
}

