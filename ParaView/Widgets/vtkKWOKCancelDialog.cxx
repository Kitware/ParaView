/*=========================================================================

  Module:    vtkKWOKCancelDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWOKCancelDialog.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWOKCancelDialog );
vtkCxxRevisionMacro(vtkKWOKCancelDialog, "1.10");




int vtkKWOKCancelDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWOKCancelDialog::vtkKWOKCancelDialog()
{
  this->CommandFunction = vtkKWOKCancelDialogCommand;
  this->Message = vtkKWWidget::New();
  this->Message->SetParent(this);
  this->ButtonFrame = vtkKWWidget::New();
  this->ButtonFrame->SetParent(this);
  this->OKButton = vtkKWWidget::New();
  this->OKButton->SetParent(this->ButtonFrame);
  this->CancelButton = vtkKWWidget::New();
  this->CancelButton->SetParent(this->ButtonFrame);
}

vtkKWOKCancelDialog::~vtkKWOKCancelDialog()
{
  this->Message->Delete();
  this->ButtonFrame->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();
}


void vtkKWOKCancelDialog::Create(vtkKWApplication *app, const char *args)
{
  // invoke super method
  this->Superclass::Create(app,args);
  
  this->Message->Create(app,"label","");
  this->ButtonFrame->Create(app,"frame","");
  this->OKButton->Create(app, "button", "-width 16");
  this->OKButton->SetCommand(this, "OK");
  this->OKButton->SetTextOption("OK");
  this->CancelButton->Create(app,"button","-width 16");
  this->CancelButton->SetCommand(this, "Cancel");
  this->CancelButton->SetTextOption("Cancel");
  this->Script("pack %s %s -side left -padx 4 -expand yes",
               this->OKButton->GetWidgetName(),
               this->CancelButton->GetWidgetName() );
  this->Script("pack %s -side bottom -fill x -pady 4",
               this->ButtonFrame->GetWidgetName());
  this->Script("pack %s -side bottom -fill x -pady 4",
               this->Message->GetWidgetName());
}

void vtkKWOKCancelDialog::SetText(const char *txt)
{
  this->Message->SetTextOption(txt);
}

//----------------------------------------------------------------------------
void vtkKWOKCancelDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

