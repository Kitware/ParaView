/*=========================================================================

  Program:   ParaView
  Module:    vtkKWLookmark.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/


#include "vtkKWLookmark.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCheckButtonLabeled.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWRadioButton.h"
#include "vtkKWText.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLookmark );
vtkCxxRevisionMacro( vtkKWLookmark, "1.5");

int vtkKWLookmarkCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLookmark::vtkKWLookmark()
{
  this->CommandFunction = vtkKWLookmarkCommand;

//  this->LmkIcon= vtkPVCameraIcon::New();
  this->LmkIcon= vtkKWLabel::New();
  this->Checkbox= vtkKWCheckButton::New();
  this->LmkLeftFrame= vtkKWFrame::New();
  this->LmkRightFrame= vtkKWFrame::New();
  this->LmkFrame = vtkKWFrame::New();
  this->LmkMainFrame = vtkKWFrameLabeled::New();
  this->LmkCommentsFrame= vtkKWFrameLabeled::New();
  this->LmkDatasetLabel= vtkKWLabel::New();
  this->LmkDatasetCheckbox = vtkKWCheckButtonLabeled::New();
  this->LmkDatasetFrame = vtkKWFrame::New();
  this->LmkCommentsText= vtkKWText::New();
  this->LmkNameField = vtkKWText::New();
  this->SeparatorFrame = vtkKWFrame::New();

  this->Dataset = NULL;

  this->Width = this->Height = 48; 
}

//----------------------------------------------------------------------------
vtkKWLookmark::~vtkKWLookmark()
{
  if(this->LmkIcon)
    {
    this->LmkIcon->Delete();
    this->LmkIcon = 0;
    }
  if(this->Checkbox)
    {
    this->Checkbox->Delete();
    this->Checkbox = 0;
    }
  if(this->LmkDatasetLabel)
    {
    this->LmkDatasetLabel->Delete();
    this->LmkDatasetLabel = NULL;
    }

  if(this->LmkDatasetCheckbox)
    {
    this->LmkDatasetCheckbox->Delete();
    this->LmkDatasetCheckbox = NULL;
    }

  if(this->LmkCommentsText)
    {
    this->LmkCommentsText->Delete();
    this->LmkCommentsText= NULL;
    }
  if(this->LmkNameField)
    {
    this->LmkNameField->Delete();
    this->LmkNameField = NULL;
    }
  if(this->LmkCommentsFrame)
    {
    this->LmkCommentsFrame->Delete();
    this->LmkCommentsFrame = NULL;
    }
  if(this->LmkDatasetFrame)
    {
    this->LmkDatasetFrame->Delete();
    this->LmkDatasetFrame = NULL;
    }
  if(this->LmkLeftFrame)
    {
    this->LmkLeftFrame->Delete();
    this->LmkLeftFrame= NULL;
    }
  if(this->LmkRightFrame)
    {
    this->LmkRightFrame->Delete();
    this->LmkRightFrame= NULL;
    }

  if(this->LmkMainFrame)
    {
    this->LmkMainFrame->Delete();
    this->LmkMainFrame = NULL;
    }
  if(this->SeparatorFrame)
    {
    this->SeparatorFrame->Delete();
    this->SeparatorFrame = 0;
    }

  if(this->LmkFrame)
    {
    this->LmkFrame->Delete();
    this->LmkFrame= NULL;
    }

  if(this->Dataset)
    {
    delete [] this->Dataset;
    this->Dataset = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkKWLookmark::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Lookmark Widget already created");
    return;
    }

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetApplication(app);

  const char *wname = this->GetWidgetName();

  this->Script("frame %s -borderwidth 0", wname);

  this->LmkFrame->SetParent(this);
  this->LmkFrame->Create(app,0);

  this->Checkbox->SetParent(this->LmkFrame->GetFrame());
  this->Checkbox->SetIndicator(1);
  this->Checkbox->Create(app, "");
  this->Checkbox->SetState(0);

  this->LmkMainFrame->SetParent(this->LmkFrame->GetFrame());
  this->LmkMainFrame->ShowHideFrameOn();
  this->LmkMainFrame->Create(app, 0);
  this->LmkMainFrame->SetLabelText("Lookmark");
//  this->LmkMainFrame->GetLabel()->SetBind(this, "<Double-1>", "EditLookmarkCallback");

  this->GetDragAndDropTargetSet()->SetSourceAnchor(
    this->LmkMainFrame->GetLabel());

  this->SeparatorFrame->SetParent(this);
  this->SeparatorFrame->SetScrollable(0);
  this->SeparatorFrame->Create(app,"");


  this->LmkLeftFrame->SetParent(this->LmkMainFrame->GetFrame());
  this->LmkLeftFrame->Create(app,0);

  this->LmkRightFrame->SetParent(this->LmkMainFrame->GetFrame());
  this->LmkRightFrame->Create(app,0);

  this->LmkIcon->SetParent(this->LmkLeftFrame->GetFrame());
  this->LmkIcon->Create(app, "");

  this->LmkIcon->SetText("Empty");
  this->Script("%s configure -relief raised -anchor center", 
               this->LmkIcon->GetWidgetName());

  int rw, rh, padx, pady, bd;
  this->Script("concat [winfo reqwidth %s] [winfo reqheight %s] "
               "[%s cget -padx] [%s cget -pady] [%s cget -bd]",
               this->LmkIcon->GetWidgetName(), this->LmkIcon->GetWidgetName(), 
               this->LmkIcon->GetWidgetName(), this->LmkIcon->GetWidgetName(), 
               this->LmkIcon->GetWidgetName());

  sscanf(this->GetApplication()->GetMainInterp()->result, 
         "%d %d %d %d %d", 
         &rw, &rh, &padx, &pady, &bd);
  
  this->Script("%s configure -padx %d -pady %d", 
               this->LmkIcon->GetWidgetName(), 
               padx + (int)ceil((double)(this->Width  - rw) / 2.0) + bd, 
               pady + (int)ceil((double)(this->Height - rh) / 2.0) + bd);

  this->LmkIcon->SetBalloonHelpString("Left click to generate lookmark");


  this->LmkDatasetFrame->SetParent(this->LmkRightFrame->GetFrame());
  this->LmkDatasetFrame->Create(app, 0);


  this->LmkDatasetLabel->SetParent(this->LmkDatasetFrame->GetFrame());
  this->LmkDatasetLabel->Create(app, "");
  this->LmkDatasetLabel->SetText("Dataset: ");

  this->LmkDatasetCheckbox->SetParent(this->LmkDatasetFrame->GetFrame());
  this->LmkDatasetCheckbox->Create(app, "");
  this->LmkDatasetCheckbox->GetWidget()->SetIndicator(1);
  this->LmkDatasetCheckbox->GetWidget()->SetState(1);
  this->LmkDatasetCheckbox->GetLabel()->SetText("Lock to Dataset");

  this->LmkCommentsFrame->SetParent(this->LmkRightFrame->GetFrame());
  this->LmkCommentsFrame->ShowHideFrameOn();
  this->LmkCommentsFrame->Create(app, 0);
  this->LmkCommentsFrame->SetLabelText("Comments:");

  this->LmkCommentsText->SetParent(this->LmkCommentsFrame->GetFrame());
  this->LmkCommentsText->Create(app, "");

  this->LmkNameField->SetParent(this->LmkMainFrame->GetLabelFrame());
  this->LmkNameField->Create(app, "");

  this->Pack();

  this->LmkCommentsFrame->PerformShowHideFrame();

  // Update enable state
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLookmark::DragAndDropPerformCommand(int x, int y, vtkKWWidget *vtkNotUsed(widget), vtkKWWidget *vtkNotUsed(anchor))
{
  if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->SeparatorFrame->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 2 -relief groove", this->SeparatorFrame->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    }
}


//----------------------------------------------------------------------------
int vtkKWLookmark::IsLockedToDataset()
{
  return this->LmkDatasetCheckbox->GetWidget()->GetState();
}


//----------------------------------------------------------------------------
void vtkKWLookmark::RemoveDragAndDropTargetCues()
{
  this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLookmark::EditLookmarkCallback()
{
  char *temp = new char[100];

  this->SetSelectionState(0);

  strcpy(temp,this->LmkMainFrame->GetLabel()->GetText());
  this->LmkMainFrame->SetLabelText("");
  this->Script("pack %s", this->LmkNameField->GetWidgetName());
  this->Script("%s configure -bg white -height 1 -width %d -wrap none", this->LmkNameField->GetTextWidget()->GetWidgetName(),strlen(temp));
  this->LmkNameField->SetValue(temp);
  this->LmkNameField->GetTextWidget()->SetBind(this, "<KeyPress-Return>", "ChangeLookmarkName");

  delete [] temp;
}

//----------------------------------------------------------------------------
void vtkKWLookmark::ChangeLookmarkName()
{
  char *lmkName = new char[100];

  strcpy(lmkName,this->LmkNameField->GetValue());
  this->LmkNameField->Unpack();
  this->Script("pack %s -anchor nw -side left -fill both -expand true -padx 2 -pady 0", this->LmkMainFrame->GetLabel()->GetWidgetName());
  this->LmkMainFrame->SetLabelText(lmkName);

  delete [] lmkName;
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetLookmarkName(char *name)
{
  this->LmkMainFrame->SetLabelText(name);
}

char *vtkKWLookmark::GetComments()
{
  if(this->LmkCommentsText)
    return this->LmkCommentsText->GetValue();
  else
    return 0;
}


//----------------------------------------------------------------------------
char *vtkKWLookmark::GetLookmarkName()
{
  return this->LmkMainFrame->GetLabel()->GetText();
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetComments(char *comm)
{
  this->LmkCommentsText->SetValue(comm);
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetDataset(char *dsetName)
{
  char *ptr = dsetName;
  ptr+=strlen(dsetName)-1;
  while(*ptr!='/' && *ptr!='\\')
    ptr--;
  ptr++;
  if(this->Dataset)
    delete [] this->Dataset;
  this->Dataset = new char[15+strlen(ptr)];
  strcpy(this->Dataset,"Dataset: ");
  strcat(this->Dataset,ptr);
  this->LmkDatasetLabel->SetText(this->Dataset);
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetLookmarkImage(vtkKWIcon *icon)
{
  if(this->LmkIcon)
    {
    this->LmkIcon->SetImageOption(icon);
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetSelectionState(int flag)
{
  this->Checkbox->SetState(flag);
}

//----------------------------------------------------------------------------
int vtkKWLookmark::GetSelectionState()
{
  return this->Checkbox->GetState();
}

//----------------------------------------------------------------------------
void vtkKWLookmark::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

//  this->PropagateEnableState(this->LmkFrame);
  this->PropagateEnableState(this->LmkMainFrame);
  this->PropagateEnableState(this->LmkRightFrame);
  this->PropagateEnableState(this->LmkLeftFrame);
//  this->PropagateEnableState(this->LmkCheckBox);
  this->PropagateEnableState(this->LmkCommentsFrame);
  this->PropagateEnableState(this->LmkCommentsText);
  this->PropagateEnableState(this->LmkIcon);
  this->PropagateEnableState(this->LmkDatasetLabel);
}

//----------------------------------------------------------------------------
void vtkKWLookmark::Pack()
{
  // Unpack everything
  this->LmkFrame->Unpack();
  this->SeparatorFrame->Unpack();

  // Repack everything
  this->Script("pack %s -anchor nw -side left", this->Checkbox->GetWidgetName());
  this->Script("pack %s -anchor nw -side left -padx 1 -pady 1", this->LmkIcon->GetWidgetName());
  this->Script("pack %s -anchor w", this->LmkDatasetLabel->GetWidgetName());
  this->Script("pack %s -anchor w", this->LmkDatasetCheckbox->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -expand true", this->LmkDatasetFrame->GetWidgetName());
  this->Script("pack %s -anchor w", this->LmkCommentsText->GetWidgetName());
  this->Script("%s configure -bg white -height 3 -width 50 -wrap none", this->LmkCommentsText->GetTextWidget()->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -expand true -padx 2 -pady 2", this->LmkCommentsFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -side left", this->LmkLeftFrame->GetWidgetName());
  this->Script("pack %s -anchor w -side left -expand true -fill x -padx 3", this->LmkRightFrame->GetWidgetName());
  this->Script("pack %s -fill x -expand true -side left", this->LmkMainFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -fill x -expand true", this->LmkFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill both", this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",this->SeparatorFrame->GetFrame()->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill x", this->SeparatorFrame->GetWidgetName());

}

//----------------------------------------------------------------------------
void vtkKWLookmark::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Location: " << this->Location << endl;
  os << indent << "SeparatorFrame: " << this->SeparatorFrame << endl;
  os << indent << "LmkMainFrame: " << this->LmkMainFrame << endl;
  os << indent << "LmkIcon: " << this->LmkIcon << endl;
}
