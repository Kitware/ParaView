/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTraceFileDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTraceFileDialog.h"

#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTraceFileDialog );
vtkCxxRevisionMacro(vtkPVTraceFileDialog, "1.7");

int vtkPVTraceFileDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::vtkPVTraceFileDialog()
{
  this->SaveFrame = vtkKWFrame::New();
  this->SaveFrame->SetParent(this->ButtonFrame);

  this->SaveButton = vtkKWPushButton::New();
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
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("TraceFileDialogx already created");
    return;
    }

  // invoke super method
  this->Superclass::Create(app,args);

  this->SaveFrame->Create(app, "-bd 3 -relief flat");

  this->SaveButton->Create(app, "-text Save -width 16");
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
