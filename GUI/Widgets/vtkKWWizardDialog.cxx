/*=========================================================================

  Module:    vtkKWWizardDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWizardDialog.h"

#include "vtkObjectFactory.h"

#include "vtkKWWizardWorkflow.h"
#include "vtkKWWizardWidget.h"
#include "vtkKWPushButton.h"
#include "vtkKWSeparator.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWizardDialog);
vtkCxxRevisionMacro(vtkKWWizardDialog, "1.2");

//----------------------------------------------------------------------------
vtkKWWizardDialog::vtkKWWizardDialog()
{
  this->WizardWidget = vtkKWWizardWidget::New();
}

//----------------------------------------------------------------------------
vtkKWWizardDialog::~vtkKWWizardDialog()
{
  if (this->WizardWidget)
    {
    this->WizardWidget->Delete();
    this->WizardWidget = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardDialog::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SetSize(500, 340);

  // -------------------------------------------------------------------
  // Wizard Widget

  if (!this->WizardWidget)
    {
    this->WizardWidget = vtkKWWizardWidget::New();
    }
  this->WizardWidget->SetParent(this->GetFrame());
  this->WizardWidget->Create();

  this->WizardWidget->GetCancelButton()->SetCommand(this, "Cancel");
  this->WizardWidget->GetOKButton()->SetCommand(this, "OK");
  
  this->Script("pack %s -side top -fill both -expand y -pady 1", 
               this->WizardWidget->GetWidgetName());

  // I'm going to add a little more space

  this->Script(
    "pack %s -pady 4", 
    this->WizardWidget->GetSeparatorBeforeButtons()->GetWidgetName());
}

//---------------------------------------------------------------------------
vtkKWWizardWorkflow* vtkKWWizardDialog::GetWizardWorkflow() 
{
  if (this->WizardWidget)
    {
    return this->WizardWidget->GetWizardWorkflow();
    }
  
  return NULL;
}

//---------------------------------------------------------------------------
void vtkKWWizardDialog::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->WizardWidget);
}

//----------------------------------------------------------------------------
void vtkKWWizardDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WizardWidget: ";
  if (this->WizardWidget)
    {
    os << endl;
    this->WizardWidget->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
