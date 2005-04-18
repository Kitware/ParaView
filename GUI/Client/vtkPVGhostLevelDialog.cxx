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
vtkCxxRevisionMacro(vtkPVGhostLevelDialog, "1.9");

int vtkPVGhostLevelDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

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
void vtkPVGhostLevelDialog::Create(vtkKWApplication *app, const char *args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVGhostLevelDialog already created");
    return;
    }

  this->Superclass::Create(app,args);

  this->ButtonFrame->Create(app, 0);

  this->SelFrame1->Create(app, "-bd 3 -relief flat");
  this->SelFrame2->Create(app, "-bd 3 -relief flat");
  this->SelFrame3->Create(app, "-bd 3 -relief flat");

  this->SelButton1->Create(app, "-text 0");
  this->SelButton1->SetCommand(this, "SetGhostLevel 0");
  app->Script("pack %s -expand t", 
                            this->SelButton1->GetWidgetName());

  this->SelButton2->Create(app, "-text 1");
  this->SelButton2->SetCommand(this, "SetGhostLevel 1");
  app->Script("pack %s -expand t", 
                            this->SelButton2->GetWidgetName());

  this->SelButton3->Create(app, "-text 2");
  this->SelButton3->SetCommand(this, "SetGhostLevel 2");
  app->Script("pack %s -expand t", 
                            this->SelButton3->GetWidgetName());

  this->Separator->Create(app, 
                          "-bd 1 -height 3 -relief sunken");

  this->Label->Create(app, "");

  app->Script("pack %s %s %s -padx 4 -side left -expand t", 
                            this->SelFrame1->GetWidgetName(),
                            this->SelFrame2->GetWidgetName(),
                            this->SelFrame3->GetWidgetName());
  
  app->Script(
    "pack %s -ipadx 10 -ipady 10 -side top -expand t -fill x", 
    this->Label->GetWidgetName());
  app->Script(
    "pack %s -side top -expand t -fill x", 
    this->Separator->GetWidgetName());
  app->Script(
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
  this->Withdraw();
  this->ReleaseGrab();
  this->GhostLevel = level;  
  this->Done = 2;  
}

//----------------------------------------------------------------------------
void vtkPVGhostLevelDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
}
