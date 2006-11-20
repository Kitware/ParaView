/*=========================================================================

  Module:    vtkKWLogDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLogDialog.h"
#include "vtkKWLogWidget.h"
#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLogDialog );
vtkCxxRevisionMacro(vtkKWLogDialog, "1.2");

//----------------------------------------------------------------------------
vtkKWLogDialog::vtkKWLogDialog()
{
  this->LogWidget = vtkKWLogWidget::New();
  this->CloseButton = NULL;
}

//----------------------------------------------------------------------------
vtkKWLogDialog::~vtkKWLogDialog()
{
  if (this->LogWidget)
    {
    this->LogWidget->Delete();  
    }
  if (this->CloseButton)
    {
    this->CloseButton->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWLogDialog::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }
  
  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SetResizable(1, 1);
  this->SetMinimumSize(400, 450);
  this->SetSize(650, 550);
  vtksys_stl::string title;
  if (this->GetApplication()->GetName())
    {
    title += this->GetApplication()->GetName();
    title += ": ";
    }
  title += "Log Viewer";
  this->SetTitle(title.c_str());
  
  // Record viewer

  if (!this->LogWidget)
    {
    this->LogWidget = vtkKWLogWidget::New();
    }
  this->LogWidget->SetParent(this);
  this->LogWidget->Create();
  this->Script("pack %s -anchor nw -fill both -expand true",
               this->LogWidget->GetWidgetName());
  
  // Close button

  if (!this->CloseButton)
    {
    this->CloseButton = vtkKWPushButton::New();
    }
  this->CloseButton->SetParent(this);
  this->CloseButton->Create();
  this->CloseButton->SetWidth(20);
  this->CloseButton->SetText("Close");
  this->CloseButton->SetCommand(this, "Withdraw");

  this->Script("pack %s -anchor center -pady 2 -expand n",
               this->CloseButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLogDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if(this->LogWidget)
    {
    this->LogWidget->PrintSelf(os, indent);
    }
}
