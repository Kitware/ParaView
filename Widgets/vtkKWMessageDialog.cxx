/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMessageDialog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"



//-----------------------------------------------------------------------------
vtkKWMessageDialog* vtkKWMessageDialog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWMessageDialog");
  if(ret)
    {
    return (vtkKWMessageDialog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWMessageDialog;
}




int vtkKWMessageDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWMessageDialog::vtkKWMessageDialog()
{
  this->CommandFunction = vtkKWMessageDialogCommand;
  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this);
  this->ButtonFrame = vtkKWWidget::New();
  this->ButtonFrame->SetParent(this);
  this->OKButton = vtkKWWidget::New();
  this->OKButton->SetParent(this->ButtonFrame);
  this->CancelButton = vtkKWWidget::New();
  this->CancelButton->SetParent(this->ButtonFrame);
  this->Style = vtkKWMessageDialog::Message;
}

vtkKWMessageDialog::~vtkKWMessageDialog()
{
  this->Label->Delete();
  this->ButtonFrame->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();
}


void vtkKWMessageDialog::Create(vtkKWApplication *app, const char *args)
{
  // invoke super method
  this->vtkKWDialog::Create(app,args);
  
  this->Label->Create(app,"label","");
  this->ButtonFrame->Create(app,"frame","");
  
  switch (this->Style)
    {
    case vtkKWMessageDialog::Message :
      this->OKButton->Create(app,"button","-text OK -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->Script("pack %s -side left -padx 4 -expand yes",
                   this->OKButton->GetWidgetName());
      break;
    case vtkKWMessageDialog::YesNo :
      this->OKButton->Create(app,"button","-text Yes -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->CancelButton->Create(app,"button","-text No -width 16");
      this->CancelButton->SetCommand(this, "Cancel");
      this->Script("pack %s %s -side left -padx 4 -expand yes",
                   this->OKButton->GetWidgetName(),
                   this->CancelButton->GetWidgetName());
      break;
    case vtkKWMessageDialog::OkCancel :
      this->OKButton->Create(app,"button","-text OK -width 16");
      this->OKButton->SetCommand(this, "OK");
      this->CancelButton->Create(app,"button","-text Cancel -width 16");
      this->CancelButton->SetCommand(this, "Cancel");
      this->Script("pack %s %s -side left -padx 4 -expand yes",
                   this->OKButton->GetWidgetName(),
                   this->CancelButton->GetWidgetName());
      break;
    }
  
  this->Script("pack %s -side bottom -fill x -pady 4",
               this->ButtonFrame->GetWidgetName());
  this->Script("pack %s -side bottom -fill x -pady 4",
               this->Label->GetWidgetName());
}

void vtkKWMessageDialog::SetText(const char *txt)
{
  this->Script("%s configure -text {%s}",
               this->Label->GetWidgetName(),txt);
}
