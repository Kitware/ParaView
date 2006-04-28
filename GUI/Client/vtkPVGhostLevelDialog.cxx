/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGhostLevelDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGhostLevelDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabel.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVGhostLevelDialog );
vtkCxxRevisionMacro(vtkPVGhostLevelDialog, "1.15");

//-----------------------------------------------------------------------------
vtkPVGhostLevelDialog::vtkPVGhostLevelDialog()
{

  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Label->SetText("How many ghost levels would you like to save?");
  
  this->Separator = vtkKWFrame::New();
  this->Separator->SetParent(this);
  
  this->ButtonFrame = vtkKWFrame::New();
  this->ButtonFrame->SetParent(this);
  
  this->SelFrame1 = vtkKWFrame::New();
  this->SelFrame1->SetParent(this->ButtonFrame);

  this->SelFrame2 = vtkKWFrame::New();
  this->SelFrame2->SetParent(this->ButtonFrame);

  this->SelFrame3 = vtkKWFrame::New();
  this->SelFrame3->SetParent(this->ButtonFrame);

  this->SelButton1 = vtkKWPushButton::New();
  this->SelButton1->SetParent(this->SelFrame1);

  this->SelButton2 = vtkKWPushButton::New();
  this->SelButton2->SetParent(this->SelFrame2);
  
  this->SelButton3 = vtkKWPushButton::New();
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
void vtkPVGhostLevelDialog::CreateWidget()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVGhostLevelDialog already created");
    return;
    }

  this->Superclass::CreateWidget();

  this->ButtonFrame->Create();

  this->SelFrame1->Create();
  this->SelFrame1->SetBorderWidth(3);
  this->SelFrame2->Create();
  this->SelFrame2->SetBorderWidth(3);
  this->SelFrame3->Create();
  this->SelFrame3->SetBorderWidth(3);

  this->SelButton1->Create();
  this->SelButton1->SetText("0");
  this->SelButton1->SetCommand(this, "SetGhostLevel 0");
  this->Script("pack %s -expand t", 
                            this->SelButton1->GetWidgetName());

  this->SelButton2->Create();
  this->SelButton2->SetText("1");
  this->SelButton2->SetCommand(this, "SetGhostLevel 1");
  this->Script("pack %s -expand t", 
                            this->SelButton2->GetWidgetName());

  this->SelButton3->Create();
  this->SelButton3->SetText("2");
  this->SelButton3->SetCommand(this, "SetGhostLevel 2");
  this->Script("pack %s -expand t", 
                            this->SelButton3->GetWidgetName());

  this->Separator->Create();
  this->Separator->SetBorderWidth(1);
  this->Separator->SetHeight(3);
  this->Separator->SetReliefToSunken();

  this->Label->Create();

  this->Script("pack %s %s %s -padx 4 -side left -expand t", 
                            this->SelFrame1->GetWidgetName(),
                            this->SelFrame2->GetWidgetName(),
                            this->SelFrame3->GetWidgetName());
  
  this->Script(
    "pack %s -ipadx 10 -ipady 10 -side top -expand t -fill x", 
    this->Label->GetWidgetName());
  this->Script(
    "pack %s -side top -expand t -fill x", 
    this->Separator->GetWidgetName());
  this->Script(
    "pack %s -ipadx 10 -ipady 10 -side top -expand t -fill x", 
    this->ButtonFrame->GetWidgetName());


  this->SelButton1->AddBinding(
    "<FocusIn>", this->SelFrame1, "SetReliefToGroove");
  this->SelButton1->AddBinding(
    "<FocusOut>", this->SelFrame1, "SetReliefToFlat");
  this->SelButton1->AddBinding(
    "<Return>", this, "SetGhostLevel 0");

  this->SelButton2->AddBinding(
    "<FocusIn>", this->SelFrame2, "SetReliefToGroove");
  this->SelButton2->AddBinding(
    "<FocusOut>", this->SelFrame2, "SetReliefToFlat");
  this->SelButton2->AddBinding(
    "<Return>", this, "SetGhostLevel 1");

  this->SelButton3->AddBinding(
    "<FocusIn>", this->SelFrame3, "SetReliefToGroove");
  this->SelButton3->AddBinding(
    "<FocusOut>", this->SelFrame1, "SetReliefToFlat");
  this->SelButton3->AddBinding(
    "<Return>", this, "SetGhostLevel 2");
}

//----------------------------------------------------------------------------
int vtkPVGhostLevelDialog::Invoke()
{
  this->SelButton1->Focus();
  return this->vtkKWDialog::Invoke();
}

//----------------------------------------------------------------------------
void vtkPVGhostLevelDialog::SetGhostLevel(int level)
{
  this->GhostLevel = level;  
  this->Done = 2;  
}

//----------------------------------------------------------------------------
void vtkPVGhostLevelDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
}
