/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWOKCancelDialog.cxx
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
#include "vtkKWOKCancelDialog.h"
#include "vtkObjectFactory.h"



//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWOKCancelDialog );




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
  this->CancelButton->Create(app,"button","-text Cancel -width 16");
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
