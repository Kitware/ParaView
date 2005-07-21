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
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWRadioButton.h"
#include "vtkKWText.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLookmark );
vtkCxxRevisionMacro( vtkKWLookmark, "1.24");

//----------------------------------------------------------------------------
vtkKWLookmark::vtkKWLookmark()
{
  this->LmkIcon= vtkKWLabel::New();
  this->Checkbox= vtkKWCheckButton::New();
  this->LmkLeftFrame= vtkKWFrame::New();
  this->LmkRightFrame= vtkKWFrame::New();
  this->LmkFrame = vtkKWFrame::New();
  this->LmkMainFrame = vtkKWFrameWithLabel::New();
  this->LmkCommentsFrame= vtkKWFrameWithLabel::New();
  this->LmkDatasetLabel= vtkKWLabel::New();
  this->LmkDatasetFrame = vtkKWFrame::New();
  this->LmkCommentsText= vtkKWText::New();
  this->LmkNameField = vtkKWText::New();
  this->SeparatorFrame = vtkKWFrame::New();

  this->Name = NULL;
  this->Comments = NULL;
  this->Dataset = NULL;
  this->Width = this->Height = 48; 
  this->PixelSize = 3;
  this->MacroFlag = 0;

}

//----------------------------------------------------------------------------
vtkKWLookmark::~vtkKWLookmark()
{

  if(this->LmkIcon)
    {
    this->LmkIcon->Delete();
    this->LmkIcon = 0;
    }

  if(this->LmkDatasetLabel)
    {
    this->LmkDatasetLabel->Delete();
    this->LmkDatasetLabel = NULL;
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

  if(this->Checkbox)
    {
    this->Checkbox->Delete();
    this->Checkbox = 0;
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
void vtkKWLookmark::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->LmkFrame->SetParent(this);
  this->LmkFrame->Create(app);

  this->LmkMainFrame->SetParent(this->LmkFrame);
  this->LmkMainFrame->ShowHideFrameOn();
  this->LmkMainFrame->Create(app);
  this->LmkMainFrame->SetLabelText("Lookmark");
//  this->LmkMainFrame->GetLabel()->SetBind(this, "<Double-1>", "EditLookmarkCallback");

  this->Checkbox->SetParent(this->LmkMainFrame->GetLabelFrame());
  this->Checkbox->SetIndicator(1);
  this->Checkbox->Create(app);
  this->Checkbox->SetSelectedState(0);

  this->GetDragAndDropTargetSet()->SetSourceAnchor(
    this->LmkMainFrame->GetLabel());

  this->SeparatorFrame->SetParent(this);
  this->SeparatorFrame->Create(app);


  this->LmkLeftFrame->SetParent(this->LmkMainFrame->GetFrame());
  this->LmkLeftFrame->Create(app);

  this->LmkRightFrame->SetParent(this->LmkMainFrame->GetFrame());
  this->LmkRightFrame->Create(app);

  this->LmkIcon->SetParent(this->LmkLeftFrame);
  this->LmkIcon->Create(app);

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

  this->LmkIcon->SetBalloonHelpString("Left click to visit lookmark");


  this->LmkDatasetFrame->SetParent(this->LmkRightFrame);
  this->LmkDatasetFrame->Create(app);

  this->LmkDatasetLabel->SetParent(this->LmkDatasetFrame);
  this->LmkDatasetLabel->Create(app);
  this->LmkDatasetLabel->SetText("Dataset: ");

  this->LmkCommentsFrame->SetParent(this->LmkRightFrame);
  this->LmkCommentsFrame->ShowHideFrameOn();
  this->LmkCommentsFrame->Create(app);
  this->LmkCommentsFrame->SetLabelText("Comments:");

  this->LmkCommentsText->SetParent(this->LmkCommentsFrame->GetFrame());
  this->LmkCommentsText->Create(app);
  this->LmkCommentsText->SetBinding("<KeyPress>", this, "CommentsModifiedCallback");

  this->LmkNameField->SetParent(this->LmkMainFrame->GetLabelFrame());
  this->LmkNameField->Create(app);

  this->Pack();

  this->LmkCommentsFrame->PerformShowHideFrame();

  // Update enable state
  this->UpdateEnableState();
}

void vtkKWLookmark::CommentsModifiedCallback()
{
  int num;
  char words[4][50];
  char str[250];

  this->SetComments(this->LmkCommentsText->GetValue());

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

  this->LmkCommentsFrame->SetLabelText(str);
}

void vtkKWLookmark::UpdateWidgetValues()
{
  this->LmkCommentsText->SetValue(this->Comments);
  this->LmkMainFrame->SetLabelText(this->Name);

  if(strstr(this->Dataset,"/") && !strstr(this->Dataset,"\\"))
    {
    char *ptr = this->Dataset;
    ptr+=strlen(ptr)-1;
    while(*ptr!='/' && *ptr!='\\')
      ptr--;
    ptr++;
    char *datasetLabel = new char[15+strlen(ptr)];
    strcpy(datasetLabel,"Dataset: ");
    strcat(datasetLabel,ptr);
    this->LmkDatasetLabel->SetText(datasetLabel);
    delete [] datasetLabel;
    }
  else
    {
    char *datasetLabel = new char[15+strlen(this->Dataset)];
    strcpy(datasetLabel,"Source: ");
    strcat(datasetLabel,this->Dataset);
    this->LmkDatasetLabel->SetText(datasetLabel);
    delete [] datasetLabel;
    }
}

void vtkKWLookmark::UpdateVariableValues()
{
  this->SetComments(this->LmkCommentsText->GetValue());
  this->SetName(this->LmkMainFrame->GetLabel()->GetText());
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

  strcpy(temp,this->LmkMainFrame->GetLabel()->GetText());
  this->LmkMainFrame->SetLabelText("");
  this->Script("pack %s", this->LmkNameField->GetWidgetName());
  this->Script("%s configure -bg white -height 1 -width %d -wrap none", this->LmkNameField->GetWidgetName(),strlen(temp));
  this->LmkNameField->SetValue(temp);
  this->LmkNameField->SetBinding("<KeyPress-Return>", this, "ChangeLookmarkName");

  delete [] temp;
}

//----------------------------------------------------------------------------
void vtkKWLookmark::ChangeLookmarkName()
{
  if(!strcmp(this->LmkNameField->GetValue(),"Macros"))
    {
    return;
    }

  char *lmkName = new char[100];

  strcpy(lmkName,this->LmkNameField->GetValue());
  this->LmkNameField->Unpack();
  this->Script("pack %s -anchor nw -side left -fill both -expand true -padx 2 -pady 0", this->LmkMainFrame->GetLabel()->GetWidgetName());
  this->LmkMainFrame->SetLabelText(lmkName);

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
  this->Script("pack %s -anchor nw -side left -padx 1 -pady 1", this->LmkIcon->GetWidgetName());
  if(!this->MacroFlag)
    {
    this->Script("pack %s -anchor w", this->LmkDatasetLabel->GetWidgetName());
    this->Script("pack %s -anchor w -fill x -expand true", this->LmkDatasetFrame->GetWidgetName());
    }
  this->Script("pack %s -anchor w", this->LmkCommentsText->GetWidgetName());
  this->Script("%s configure -bg white -height 3 -width 50 -wrap word", this->LmkCommentsText->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -expand true -padx 2 -pady 2", this->LmkCommentsFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -side left", this->LmkLeftFrame->GetWidgetName());
  this->Script("pack %s -anchor w -side left -expand true -fill x -padx 3", this->LmkRightFrame->GetWidgetName());
  this->Script("pack %s -before %s -anchor nw -side left", this->Checkbox->GetWidgetName(),this->LmkMainFrame->GetLabel()->GetWidgetName());
  this->Script("pack %s -fill x -expand true -side left", this->LmkMainFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -fill x -expand true", this->LmkFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill both", this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",this->SeparatorFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill x", this->SeparatorFrame->GetWidgetName());

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
}
