/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTraceFileDialog.cxx
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
#include "vtkPVTraceFileDialog.h"

#include "vtkKWFrame.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTraceFileDialog );
vtkCxxRevisionMacro(vtkPVTraceFileDialog, "1.4");

int vtkPVTraceFileDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::vtkPVTraceFileDialog()
{
  this->SaveFrame = vtkKWWidget::New();
  this->SaveFrame->SetParent(this->ButtonFrame);

  this->SaveButton = vtkKWWidget::New();
  this->SaveButton->SetParent(this->SaveFrame);

  this->SetStyleToOkCancel();
  this->SetOptions(
    vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::Beep | 
    vtkKWMessageDialog::YesDefault );
  this->SetOKButtonText("Delete");
  this->SetCancelButtonText("{Do Nothing}");

}

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::~vtkPVTraceFileDialog()
{
  this->SaveFrame->Delete();
  this->SaveButton->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Create(vtkKWApplication *app, const char *args)
{
  // invoke super method
  this->Superclass::Create(app,args);

  this->SaveFrame->Create(app,"frame","-borderwidth 3 -relief flat");

  this->SaveButton->Create(app,"button","-text Save -width 16");
  this->SaveButton->SetCommand(this, "Save");

  this->Script("pack %s -side left -expand yes",
               this->SaveButton->GetWidgetName());
  this->Script("pack %s -side left -padx 4 -expand yes",
               this->SaveFrame->GetWidgetName());

  if ( this->SaveButton->GetApplication() )
    {
    this->SaveButton->SetBind("<FocusIn>", this->SaveFrame->GetWidgetName(), 
                            "configure -relief groove");
    this->SaveButton->SetBind("<FocusOut>", this->SaveFrame->GetWidgetName(), 
                            "configure -relief flat");
    this->SaveButton->SetBind(this, "<Return>", "Save");
    }
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Save()
{
  this->Script("wm withdraw %s",this->GetWidgetName());
  this->Script("grab release %s",this->GetWidgetName());
  this->Done = 3;  
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
