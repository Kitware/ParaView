/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWOKCancelDialog.cxx
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
#include "vtkKWOKCancelDialog.h"
#include "vtkObjectFactory.h"



//-----------------------------------------------------------------------------
vtkKWOKCancelDialog* vtkKWOKCancelDialog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWOKCancelDialog");
  if(ret)
    {
    return (vtkKWOKCancelDialog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWOKCancelDialog;
}




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
  this->vtkKWDialog::Create(app,args);
  
  this->Message->Create(app,"label","");
  this->ButtonFrame->Create(app,"frame","");
  this->OKButton->Create(app,"button","-text OK -width 16");
  this->OKButton->SetCommand(this, "OK");
  this->CancelButton->Create(app,"button","-text OK -width 16");
  this->CancelButton->SetCommand(this, "Cancel");
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
  this->Script("%s configure -text {%s}",
               this->Message->GetWidgetName(),txt);
}
