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
#include "vtkKWCheckButtonWithLabel.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWRadioButton.h"
#include "vtkKWText.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkKWPushButton.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLookmark );
vtkCxxRevisionMacro( vtkKWLookmark, "1.36");

//----------------------------------------------------------------------------
vtkKWLookmark::vtkKWLookmark()
{
  this->Icon= vtkKWPushButton::New();
  this->Checkbox= vtkKWCheckButton::New();
  this->LeftFrame= vtkKWFrame::New();
  this->RightFrame= vtkKWFrame::New();
  this->Frame = vtkKWFrame::New();
  this->MainFrame = vtkKWFrameWithLabel::New();
  this->CommentsFrame= vtkKWFrameWithLabel::New();
  this->DatasetLabel= vtkKWLabel::New();
  this->DatasetFrame = vtkKWFrame::New();
  this->CommentsText= vtkKWText::New();
  this->NameField = vtkKWText::New();
  this->SeparatorFrame = vtkKWFrame::New();

  this->Name = NULL;
  this->Comments = NULL;
  this->Dataset = NULL;
  this->DatasetList = NULL;
  this->Width = this->Height = 48; 
  this->PixelSize = 3;
  this->MacroFlag = 0;
  this->MainFrameCollapsedState = 0;
  this->CommentsFrameCollapsedState = 1;
}

//----------------------------------------------------------------------------
vtkKWLookmark::~vtkKWLookmark()
{

  if(this->Icon)
    {
    this->Icon->Delete();
    this->Icon = 0;
    }

  if(this->DatasetLabel)
    {
    this->DatasetLabel->Delete();
    this->DatasetLabel = NULL;
    }

  if(this->CommentsText)
    {
    this->CommentsText->Delete();
    this->CommentsText= NULL;
    }
  if(this->NameField)
    {
    this->NameField->Delete();
    this->NameField = NULL;
    }
  if(this->CommentsFrame)
    {
    this->CommentsFrame->Delete();
    this->CommentsFrame = NULL;
    }
  if(this->DatasetFrame)
    {
    this->DatasetFrame->Delete();
    this->DatasetFrame = NULL;
    }
  if(this->LeftFrame)
    {
    this->LeftFrame->Delete();
    this->LeftFrame= NULL;
    }
  if(this->RightFrame)
    {
    this->RightFrame->Delete();
    this->RightFrame= NULL;
    }

  if(this->Checkbox)
    {
    this->Checkbox->Delete();
    this->Checkbox = 0;
    }

  if(this->MainFrame)
    {
    this->MainFrame->Delete();
    this->MainFrame = NULL;
    }
  if(this->SeparatorFrame)
    {
    this->SeparatorFrame->Delete();
    this->SeparatorFrame = 0;
    }

  if(this->Frame)
    {
    this->Frame->Delete();
    this->Frame= NULL;
    }

  if(this->Dataset)
    {
    delete [] this->Dataset;
    this->Dataset = NULL;
    }
  if(this->DatasetList)
    {
    delete [] this->DatasetList;
    this->DatasetList = NULL;
    }
  if(this->Name)
    {
    delete [] this->Name;
    this->Name = NULL;
    }
  if(this->Comments)
    {
    delete [] this->Comments;
    this->Comments = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkKWLookmark::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->Frame->SetParent(this);
  this->Frame->Create();

  this->MainFrame->SetParent(this->Frame);
  this->MainFrame->AllowFrameToCollapseOn();
  this->MainFrame->Create();
  this->MainFrame->SetLabelText("Lookmark");
  //  this->MainFrame->GetLabel()->SetBind(this, "<Double-1>", "EditLookmarkCallback");
  this->MainFrame->GetLabel()->SetBalloonHelpString("Drag and drop lookmark");

  this->Checkbox->SetParent(this->MainFrame->GetLabelFrame());
  this->Checkbox->IndicatorVisibilityOn();
  this->Checkbox->Create();
  this->Checkbox->SetSelectedState(0);

  this->SeparatorFrame->SetParent(this);
  this->SeparatorFrame->Create();

  this->LeftFrame->SetParent(this->MainFrame->GetFrame());
  this->LeftFrame->Create();

  this->RightFrame->SetParent(this->MainFrame->GetFrame());
  this->RightFrame->Create();
/*
  this->Icon->SetParent(this->LeftFrame);
  this->Icon->Create();
  this->Icon->SetText("Empty");
  this->Script("%s configure -relief raised -anchor center", 
               this->Icon->GetWidgetName());
*/
  this->Icon->SetParent(this->LeftFrame);
  this->Icon->Create();

  this->GetDragAndDropTargetSet()->SetSourceAnchor(this->Icon);

  int rw, rh, padx, pady, bd;
  this->Script("concat [winfo reqwidth %s] [winfo reqheight %s] "
               "[%s cget -padx] [%s cget -pady] [%s cget -bd]",
               this->Icon->GetWidgetName(), this->Icon->GetWidgetName(), 
               this->Icon->GetWidgetName(), this->Icon->GetWidgetName(), 
               this->Icon->GetWidgetName());

  sscanf(this->GetApplication()->GetMainInterp()->result, 
         "%d %d %d %d %d", 
         &rw, &rh, &padx, &pady, &bd);
  
  this->Script("%s configure -padx %d -pady %d", 
               this->Icon->GetWidgetName(), 
               padx + (int)ceil((double)(this->Width  - rw) / 2.0) + bd, 
               pady + (int)ceil((double)(this->Height - rh) / 2.0) + bd);

  this->Icon->SetBalloonHelpString("Left click to visit lookmark");


  this->DatasetFrame->SetParent(this->RightFrame);
  this->DatasetFrame->Create();

  this->DatasetLabel->SetParent(this->DatasetFrame);
  this->DatasetLabel->Create();
  this->DatasetLabel->SetText("Dataset: ");

  this->CommentsFrame->SetParent(this->RightFrame);
  this->CommentsFrame->AllowFrameToCollapseOn();
  this->CommentsFrame->Create();
  this->CommentsFrame->SetLabelText("Comments:");

  this->CommentsText->SetParent(this->CommentsFrame->GetFrame());
  this->CommentsText->Create();
  this->CommentsText->SetBinding("<KeyPress>", this, "CommentsModifiedCallback");
  this->CommentsText->SetStateToNormal();

  this->NameField->SetParent(this->MainFrame->GetLabelFrame());
  this->NameField->Create();
  this->NameField->SetStateToNormal();

  this->Pack();

  this->CommentsFrame->CollapseFrame();

  this->UpdateEnableState();
}

void vtkKWLookmark::CommentsModifiedCallback()
{
  int num;
  char words[4][50];
  char str[250];

  this->SetComments(this->CommentsText->GetText());

  num = sscanf(this->Comments,"%s %s %s %s",words[0],words[1],words[2],words[3]);
  switch (num)
    {
    case 1:
      sprintf(str,"Comments:  %s...",words[0]);
      break;
    case 2:
      sprintf(str,"Comments:  %s %s...",words[0],words[1]);
      break;
    case 3:
      sprintf(str,"Comments:  %s %s %s...",words[0],words[1],words[2]);
      break;
    case 4:
      sprintf(str,"Comments:  %s %s %s %s...",words[0],words[1],words[2],words[3]);
      break;
    default:
      strcpy(str,"Comments:  ");
    }
  
  if(strlen(str) > 30)
    {
    str[30] = '\0';
    strcat(str,"...");
    }

  this->CommentsFrame->SetLabelText(str);
}


void vtkKWLookmark::CreateDatasetList()
{
  int i=0;
  if(!this->Dataset)
    { 
    return;
    }
  char *ds = new char[strlen(this->Dataset)+1];
  strcpy(ds,this->Dataset);
  char *ptr = strtok(ds,";");

  while(ptr)
    {
    ptr = strtok(NULL,";");
    i++;
    }

  this->DatasetList = new char*[i+1];
  // Initialize with NULLs.
  for (int idx = 0; idx < i+1; ++idx)
    {
    this->DatasetList[idx] = NULL;
    }

  i=0;
  strcpy(ds,this->Dataset);
  ptr = strtok(ds,";");
  while(ptr)
    {
    this->DatasetList[i] = new char[strlen(ptr)+1];
    strcpy(this->DatasetList[i],ptr);
    ptr = strtok(NULL,";");
    i++;
    }

  delete [] ds;
}

void vtkKWLookmark::UpdateVariableValues()
{
  // Use the current widget values to update the internal variables
  this->SetComments(this->CommentsText->GetText());
  this->SetName(this->MainFrame->GetLabel()->GetText());
  this->SetMainFrameCollapsedState(this->MainFrame->IsFrameCollapsed());
  this->SetCommentsFrameCollapsedState(this->CommentsFrame->IsFrameCollapsed());
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetIcon(vtkKWIcon *icon)
{
  if(this->Icon)
    {
    this->Icon->SetImageToIcon(icon);
    }
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
void vtkKWLookmark::RemoveDragAndDropTargetCues()
{
  this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLookmark::EditLookmarkCallback()
{
  char *temp = new char[100];

  this->SetSelectionState(0);

  strcpy(temp,this->MainFrame->GetLabel()->GetText());
  this->MainFrame->SetLabelText("");
  this->Script("pack %s", this->NameField->GetWidgetName());
  this->Script("%s configure -bg white -height 1 -width %d -wrap none", this->NameField->GetWidgetName(),strlen(temp));
  this->NameField->SetText(temp);
  this->NameField->SetBinding("<KeyPress-Return>", this, "ChangeLookmarkName");

  delete [] temp;
}

//----------------------------------------------------------------------------
void vtkKWLookmark::ChangeLookmarkName()
{
  if(strcmp(this->NameField->GetText(),"Macros")==0)
    {
    return;
    }

  char *lmkName = new char[100];

  strcpy(lmkName,this->NameField->GetText());
  this->NameField->Unpack();
  this->Script("pack %s -anchor nw -side left -fill both -expand true -padx 2 -pady 0", this->MainFrame->GetLabel()->GetWidgetName());
  this->MainFrame->SetLabelText(lmkName);

  delete [] lmkName;
}

//----------------------------------------------------------------------------
void vtkKWLookmark::SetSelectionState(int flag)
{
  this->Checkbox->SetSelectedState(flag);
}

//----------------------------------------------------------------------------
int vtkKWLookmark::GetSelectionState()
{
  return this->Checkbox->GetSelectedState();
}

//----------------------------------------------------------------------------
void vtkKWLookmark::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->MainFrame);
  this->PropagateEnableState(this->RightFrame);
  this->PropagateEnableState(this->LeftFrame);
  this->PropagateEnableState(this->Checkbox);
  this->PropagateEnableState(this->CommentsFrame);
  this->PropagateEnableState(this->SeparatorFrame);
  this->PropagateEnableState(this->CommentsText);
  this->PropagateEnableState(this->Icon);
  this->PropagateEnableState(this->DatasetLabel);
  this->PropagateEnableState(this->DatasetFrame);
  this->PropagateEnableState(this->NameField);
}

//----------------------------------------------------------------------------
void vtkKWLookmark::Pack()
{
  // Unpack everything
  this->Frame->Unpack();
  this->SeparatorFrame->Unpack();

  // Repack everything
  this->Script("pack %s -anchor nw -side left -padx 1 -pady 1", this->Icon->GetWidgetName());
  if(!this->MacroFlag)
    {
    this->Script("pack %s -anchor w", this->DatasetLabel->GetWidgetName());
    this->Script("pack %s -anchor w -fill x -expand true", this->DatasetFrame->GetWidgetName());
    }
  this->Script("pack %s -anchor w", this->CommentsText->GetWidgetName());
  this->Script("%s configure -bg white -height 3 -width 50 -wrap word", this->CommentsText->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -expand true -padx 2 -pady 2", this->CommentsFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -side left", this->LeftFrame->GetWidgetName());
  this->Script("pack %s -anchor w -side left -expand true -fill x -padx 3", this->RightFrame->GetWidgetName());
  this->Script("pack %s -before %s -anchor nw -side left", this->Checkbox->GetWidgetName(),this->MainFrame->GetLabel()->GetWidgetName());
  this->Script("pack %s -fill x -expand true -side left", this->MainFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -fill x -expand true", this->Frame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill both", this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",this->SeparatorFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill x", this->SeparatorFrame->GetWidgetName());

  if(this->MainFrameCollapsedState)
    {
    this->MainFrame->CollapseFrame();
    }
  else
    {
    this->MainFrame->ExpandFrame();
    }
  if(this->CommentsFrameCollapsedState)
    {
    this->CommentsFrame->CollapseFrame();
    }
  else
    {
    this->CommentsFrame->ExpandFrame();
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmark::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << this->GetName() << endl;
  os << indent << "Comments: " << this->GetComments() << endl;
  os << indent << "Dataset: " << this->GetDataset() << endl;
  os << indent << "Width: " << this->GetWidth() << endl;
  os << indent << "Height: " << this->GetHeight() << endl;
  os << indent << "PixelSize: " << this->GetPixelSize() << endl;
  os << indent << "SeparatorFrame: " << this->GetSeparatorFrame() << endl;
  os << indent << "MacroFlag: " << this->GetMacroFlag() << endl;
  os << indent << "Checkbox: " << this->GetCheckbox() << endl;
  os << indent << "MainFrameCollapsedState: " << this->GetMainFrameCollapsedState() << endl;
  os << indent << "CommentsFrameCollapsedState: " << this->GetCommentsFrameCollapsedState() << endl;
}
