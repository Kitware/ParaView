/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGhostLevelDialog.cxx
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
#include "vtkPVGhostLevelDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVGhostLevelDialog );
vtkCxxRevisionMacro(vtkPVGhostLevelDialog, "1.3");

int vtkPVGhostLevelDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVGhostLevelDialog::vtkPVGhostLevelDialog()
{

  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Label->SetLabel("How many ghost levels would you like to save?");
  
  this->Separator = vtkKWWidget::New();
  this->Separator->SetParent(this);
  
  this->ButtonFrame = vtkKWFrame::New();
  this->ButtonFrame->SetParent(this);
  
  this->SelFrame1 = vtkKWWidget::New();
  this->SelFrame1->SetParent(this->ButtonFrame);

  this->SelFrame2 = vtkKWWidget::New();
  this->SelFrame2->SetParent(this->ButtonFrame);

  this->SelFrame3 = vtkKWWidget::New();
  this->SelFrame3->SetParent(this->ButtonFrame);

  this->SelButton1 = vtkKWWidget::New();
  this->SelButton1->SetParent(this->SelFrame1);

  this->SelButton2 = vtkKWWidget::New();
  this->SelButton2->SetParent(this->SelFrame2);
  
  this->SelButton3 = vtkKWWidget::New();
  this->SelButton3->SetParent(this->SelFrame3);

  this->GhostLevel = 0;
}

//-----------------------------------------------------------------------------
vtkPVGhostLevelDialog::~vtkPVGhostLevelDialog()
{
  this->SelFrame1->Delete();
  this->SelFrame2->Delete();
  this->SelFrame3->Delete();

  this->SelButton1->Delete();
  this->SelButton2->Delete();
  this->SelButton3->Delete();

  this->Separator->Delete();
  this->ButtonFrame->Delete();
  this->Label->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVGhostLevelDialog::Create(vtkKWApplication *app, const char *args)
{
  // invoke super method
  this->Superclass::Create(app,args);

  this->ButtonFrame->Create(this->Application, 0);

  this->SelFrame1->Create(app,"frame","-borderwidth 3 -relief flat");
  this->SelFrame2->Create(app,"frame","-borderwidth 3 -relief flat");
  this->SelFrame3->Create(app,"frame","-borderwidth 3 -relief flat");

  this->SelButton1->Create(this->Application, "button", "-text 0");
  this->SelButton1->SetCommand(this, "SetGhostLevel 0");
  this->Application->Script("pack %s -expand t", 
                            this->SelButton1->GetWidgetName());

  this->SelButton2->Create(this->Application, "button", "-text 1");
  this->SelButton2->SetCommand(this, "SetGhostLevel 1");
  this->Application->Script("pack %s -expand t", 
                            this->SelButton2->GetWidgetName());

  this->SelButton3->Create(this->Application, "button", "-text 2");
  this->SelButton3->SetCommand(this, "SetGhostLevel 2");
  this->Application->Script("pack %s -expand t", 
                            this->SelButton3->GetWidgetName());

  this->Separator->Create(this->Application, 
                 "frame", 
                 "-borderwidth 1 -height 3 -relief sunken");

  this->Label->Create(this->Application, "");

  this->Application->Script("pack %s %s %s -padx 4 -side left -expand t", 
                            this->SelFrame1->GetWidgetName(),
                            this->SelFrame2->GetWidgetName(),
                            this->SelFrame3->GetWidgetName());
  
  this->Application->Script(
    "pack %s -ipadx 10 -ipady 10 -side top -expand t -fill x", 
    this->Label->GetWidgetName());
  this->Application->Script(
    "pack %s -side top -expand t -fill x", 
    this->Separator->GetWidgetName());
  this->Application->Script(
    "pack %s -ipadx 10 -ipady 10 -side top -expand t -fill x", 
    this->ButtonFrame->GetWidgetName());


  this->SelButton1->SetBind("<FocusIn>", this->SelFrame1->GetWidgetName(), 
                      "configure -relief groove");
  this->SelButton1->SetBind("<FocusOut>", this->SelFrame1->GetWidgetName(), 
                      "configure -relief flat");
  this->SelButton1->SetBind(this, "<Return>", "SetGhostLevel 0");

  this->SelButton2->SetBind("<FocusIn>", this->SelFrame2->GetWidgetName(), 
                      "configure -relief groove");
  this->SelButton2->SetBind("<FocusOut>", this->SelFrame2->GetWidgetName(), 
                      "configure -relief flat");
  this->SelButton2->SetBind(this, "<Return>", "SetGhostLevel 1");

  this->SelButton3->SetBind("<FocusIn>", this->SelFrame3->GetWidgetName(), 
                      "configure -relief groove");
  this->SelButton3->SetBind("<FocusOut>", this->SelFrame3->GetWidgetName(), 
                      "configure -relief flat");
  this->SelButton3->SetBind(this, "<Return>", "SetGhostLevel 2");
}

int vtkPVGhostLevelDialog::Invoke()
{
  this->SelButton1->Focus();
  return this->vtkKWDialog::Invoke();
}

//----------------------------------------------------------------------------
void vtkPVGhostLevelDialog::SetGhostLevel(int level)
{
  this->Script("wm withdraw %s",this->GetWidgetName());
  this->Script("grab release %s",this->GetWidgetName());
  this->GhostLevel = level;  
  this->Done = 2;  
}

//----------------------------------------------------------------------------
void vtkPVGhostLevelDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
}
