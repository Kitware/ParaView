/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisProgressDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeVisProgressDialog.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisProgressDialog );
vtkCxxRevisionMacro(vtkPVComparativeVisProgressDialog, "1.3");

//-----------------------------------------------------------------------------
vtkPVComparativeVisProgressDialog::vtkPVComparativeVisProgressDialog()
{
  this->ProgressFrame = vtkKWFrame::New();
  this->ProgressLabel = vtkKWLabel::New();
  this->ProgressBar = vtkKWProgressGauge::New();
  this->Message = vtkKWLabel::New();
  this->CancelButton = vtkKWPushButton::New();

  this->Modal = 1;

  this->AbortFlag = 0;
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisProgressDialog::~vtkPVComparativeVisProgressDialog()
{
  this->ProgressFrame->Delete();
  this->ProgressLabel->Delete();
  this->ProgressBar->Delete();
  this->Message->Delete();
  this->CancelButton->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisProgressDialog::CreateWidget()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVComparativeVisProgressDialog already created");
    return;
    }

  this->Superclass::CreateWidget();

  this->Message->SetParent(this);
  this->Message->Create();
  this->Message->SetText("ParaView is creating comparative visualization "
                         "frames. Please wait.");
  this->Script("pack %s -side top -pady 5", this->Message->GetWidgetName());

  this->ProgressFrame->SetParent(this);
  this->ProgressFrame->Create();
  this->Script("pack %s -side top -pady 5", 
               this->ProgressFrame->GetWidgetName());

  this->ProgressLabel->SetParent(this->ProgressFrame);
  this->ProgressLabel->Create();
  this->ProgressLabel->SetText("Progress: ");
  this->Script("pack %s -side left -padx 5", 
               this->ProgressLabel->GetWidgetName());

  this->ProgressBar->SetParent(this->ProgressFrame);
  this->ProgressBar->Create();
  this->ProgressBar->SetHeight(15);
  this->Script("pack %s -side left", this->ProgressBar->GetWidgetName());

  this->CancelButton->SetParent(this);
  this->CancelButton->Create();
  this->CancelButton->SetText("Abort");
  this->CancelButton->SetCommand(this, "SetAbortFlag 1");
  this->Script("pack %s -side top -pady 5", this->CancelButton->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisProgressDialog::SetProgress(double progress)
{
  this->ProgressBar->SetValue(progress*100);
  // To refresh the label
  this->Script("update");
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisProgressDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AbortFlag: " << this->AbortFlag << endl;
}
