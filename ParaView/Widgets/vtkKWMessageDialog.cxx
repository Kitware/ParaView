/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMessageDialog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWApplication.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkKWWindow.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMessageDialog );




int vtkKWMessageDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWMessageDialog::vtkKWMessageDialog()
{
  this->MessageDialogFrame = vtkKWWidget::New();
  this->MessageDialogFrame->SetParent(this);
  this->CommandFunction = vtkKWMessageDialogCommand;
  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this->MessageDialogFrame);
  this->ButtonFrame = vtkKWWidget::New();
  this->ButtonFrame->SetParent(this->MessageDialogFrame);
  this->OKFrame = vtkKWWidget::New();
  this->OKFrame->SetParent(this->ButtonFrame);
  this->CancelFrame = vtkKWWidget::New();
  this->CancelFrame->SetParent(this->ButtonFrame);  
  this->OKButton = vtkKWWidget::New();
  //this->OKButton->SetParent(this->ButtonFrame);
  this->OKButton->SetParent(this->OKFrame);
  this->CancelButton = vtkKWWidget::New();
  //this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->SetParent(this->CancelFrame);
  this->Style = vtkKWMessageDialog::Message;
  this->Icon = vtkKWWidget::New();
  this->Icon->SetParent(this);
  this->Default = NoneDefault;
}

vtkKWMessageDialog::~vtkKWMessageDialog()
{
  this->Label->Delete();
  this->ButtonFrame->Delete();
  this->OKFrame->Delete();
  this->CancelFrame->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();
  this->Icon->Delete();
  this->MessageDialogFrame->Delete();
}


void vtkKWMessageDialog::Create(vtkKWApplication *app, const char *args)
{
  // invoke super method
  this->vtkKWDialog::Create(app,args);
  
  this->MessageDialogFrame->Create(app,"frame","");
  this->Label->Create(app,"label","");
  this->ButtonFrame->Create(app,"frame","");
  
  switch (this->Style)
    {
    case vtkKWMessageDialog::Message :
      this->OKFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      this->OKButton->Create(app,"button","-text OK -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->Script("pack %s -side left -expand yes",
                   this->OKButton->GetWidgetName());
      this->Script("pack %s -side left -padx 4 -expand yes",
                   this->OKFrame->GetWidgetName());
      break;
    case vtkKWMessageDialog::YesNo :
      this->OKFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      this->OKButton->Create(app,"button","-text Yes -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->CancelFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      this->CancelButton->Create(app,"button","-text No -width 16");
      this->CancelButton->SetCommand(this, "Cancel");
      this->Script("pack %s %s -side left -expand yes",
                   this->OKButton->GetWidgetName(),
                   this->CancelButton->GetWidgetName());
      this->Script("pack %s %s -side left -padx 4 -expand yes",
                   this->OKFrame->GetWidgetName(),
                   this->CancelFrame->GetWidgetName());
      break;
    case vtkKWMessageDialog::OkCancel :
      this->OKFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      this->OKButton->Create(app,"button","-text OK -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->CancelFrame->Create(app,"frame","-borderwidth 3 -relief flat");
      this->CancelButton->Create(app,"button","-text Cancel -width 16");
      this->CancelButton->SetCommand(this, "Cancel");
      this->Script("pack %s %s -side left -expand yes",
                   this->OKButton->GetWidgetName(),
                   this->CancelButton->GetWidgetName());
      this->Script("pack %s %s -side left -padx 4 -expand yes",
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
      "<FocusOut>", this->CancelFrame->GetWidgetName(), \
      "configure -relief flat");
    {
    this->CancelButton->SetBind(this, "<Return>", "Cancel");
    }
    }
  
  this->Script("pack %s -side bottom -fill x -pady 4",
               this->ButtonFrame->GetWidgetName());
  this->Script("pack %s -side bottom -fill x -padx 20 -pady 10",
               this->Label->GetWidgetName());
  this->Script("pack %s -side right -fill both -expand true -pady 4",
	       this->MessageDialogFrame->GetWidgetName());
  this->Icon->Create(app,"label","-width 0 -pady 0 -padx 0 -borderwidth 0");
  this->Script("pack %s -side left -fill y",
	       this->Icon->GetWidgetName());
}

void vtkKWMessageDialog::SetText(const char *txt)
{
  this->Script("%s configure -text {%s}",
               this->Label->GetWidgetName(),txt);
}

int vtkKWMessageDialog::Invoke()
{
  if ( this->Default == vtkKWMessageDialog::YesDefault )
    {
    this->OKButton->Focus();
    }
  else if( this->Default == vtkKWMessageDialog::NoDefault )
    {
    this->CancelButton->Focus();
    } 
  if ( this->GetDefault() != vtkKWMessageDialog::NoneDefault )
    {
    this->SetBindAll("<Right>", "focus [ tk_focusNext %W ]");
    this->SetBindAll("<Left>", "focus [ tk_focusPrev %W ]");
    }

  return vtkKWDialog::Invoke();
}

void vtkKWMessageDialog::SetIcon( int ico )
{
  if( ico <= vtkKWMessageDialog::NoIcon || ico > vtkKWMessageDialog::Info )
    {
    this->Script("%s configure -width 0 -pady 0 -padx 0 -borderwidth 0",
		 this->Icon->GetWidgetName());
    return;
    }
  char icon_array[][9] = { "", "error", "warning", "question", "info" };
  this->Script("%s configure -bitmap {%s} -anchor n -pady 4 -padx 4 -borderwidth 4",
	       this->Icon->GetWidgetName(),
	       icon_array[ico]);
}

void vtkKWMessageDialog::PopupMessage(vtkKWApplication *app, vtkKWWindow *win,
				      unsigned int icon, const char* title, 
				      const char*message)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetMasterWindow(win);
  dlg2->Create(app,"");
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon( icon );
  dlg2->BeepOn();
  dlg2->SetDefault(vtkKWMessageDialog::YesDefault);
  dlg2->Invoke();
  dlg2->Delete();
}

int vtkKWMessageDialog::PopupYesNo(vtkKWApplication *app, vtkKWWindow *win,
				   unsigned int icon, const char* title, 
				   const char*message)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetStyleToYesNo();
  dlg2->SetMasterWindow(win);
  dlg2->Create(app,"");
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon( icon );
  dlg2->BeepOn();
  dlg2->SetDefault(vtkKWMessageDialog::YesDefault);
  int ret = dlg2->Invoke();
  dlg2->Delete();
  return ret;
}
int vtkKWMessageDialog::PopupOkCancel(vtkKWApplication *app, vtkKWWindow *win,
				      unsigned int icon, const char* title, 
				      const char*message)
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetStyleToOkCancel();
  dlg2->SetMasterWindow(win);
  dlg2->Create(app,"");
  dlg2->SetText( message );
  dlg2->SetTitle( title );
  dlg2->SetIcon( icon );
  dlg2->BeepOn();
  dlg2->SetDefault(vtkKWMessageDialog::YesDefault);
  int ret = dlg2->Invoke();
  dlg2->Delete();
  return ret;
}
