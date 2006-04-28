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
vtkCxxRevisionMacro(vtkPVTraceFileDialog, "1.16");

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::vtkPVTraceFileDialog()
{
  this->SaveFrame = vtkKWFrame::New();
  this->SaveFrame->SetParent(this->ButtonFrame);
  this->SaveButton = vtkKWPushButton::New();
  this->SaveButton->SetParent(this->SaveFrame);

  this->RetraceFrame = vtkKWFrame::New();
  this->RetraceFrame->SetParent(this->ButtonFrame);
  this->RetraceButton = vtkKWPushButton::New();
  this->RetraceButton->SetParent(this->RetraceFrame);

  this->SetStyleToOkCancel();
  this->SetOptions(
    vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::Beep | 
    vtkKWMessageDialog::YesDefault );
  this->SetOKButtonText("Delete");
  this->SetCancelButtonText("Do Nothing");

}

//-----------------------------------------------------------------------------
vtkPVTraceFileDialog::~vtkPVTraceFileDialog()
{
  this->SaveFrame->Delete();
  this->SaveButton->Delete();
  this->RetraceFrame->Delete();
  this->RetraceButton->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVTraceFileDialog::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("TraceFileDialogx already created");
    return;
    }

  // invoke super method
  this->Superclass::CreateWidget();

  this->SaveFrame->Create();
  this->SaveFrame->SetBorderWidth(3);

  this->SaveButton->Create();
  this->SaveButton->SetText("Save");
  this->SaveButton->SetWidth(16);
  this->SaveButton->SetCommand(this, "Save");

  this->Script("pack %s -side left -expand yes",
               this->SaveButton->GetWidgetName());
  this->Script("pack %s -side left -padx 4 -expand yes",
               this->SaveFrame->GetWidgetName());

  if ( this->SaveButton->GetApplication() )
    {
    this->SaveButton->AddBinding(
      "<FocusIn>", this->SaveFrame, "SetReliefToGroove");
    this->SaveButton->AddBinding(
      "<FocusOut>", this->SaveFrame, "SetReliefToFlat");
    this->SaveButton->AddBinding(
      "<Return>", this, "Save");
    }
  this->RetraceFrame->Create();
  this->SaveFrame->SetBorderWidth(3);

  this->RetraceButton->Create();
  this->RetraceButton->SetText("Recover");
  this->RetraceButton->SetWidth(16);
  this->RetraceButton->SetCommand(this, "Retrace");

  this->Script("pack %s -side left -expand yes",
               this->RetraceButton->GetWidgetName());
  this->Script("pack %s -side left -padx 4 -expand yes",
               this->RetraceFrame->GetWidgetName());

  if ( this->RetraceButton->GetApplication() )
    {
    this->RetraceButton->AddBinding(
      "<FocusIn>", this->RetraceFrame, "SetReliefToGroove");
    this->RetraceButton->AddBinding(
      "<FocusOut>", this->RetraceFrame, "SetReliefToFlat");
    this->RetraceButton->AddBinding(
      "<Return>", this, "Retrace");
    }
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Save()
{
  this->Withdraw();
  this->ReleaseGrab();
  this->Done = 3;  
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::Retrace()
{
  this->Withdraw();
  this->ReleaseGrab();
  this->Done = 4;  
}

//----------------------------------------------------------------------------
void vtkPVTraceFileDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
