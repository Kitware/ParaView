/*=========================================================================

  Program:   ParaView
  Module:    vtkKWLookmarkFolder.cxx

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


#include "vtkKWLookmarkFolder.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWLookmark.h"
#include "vtkKWText.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLookmarkFolder );
vtkCxxRevisionMacro( vtkKWLookmarkFolder, "1.34");

//----------------------------------------------------------------------------
vtkKWLookmarkFolder::vtkKWLookmarkFolder()
{
  this->MainFrame = vtkKWFrame::New();
  this->LabelFrame= vtkKWFrameWithLabel::New();
  this->SeparatorFrame = vtkKWFrame::New();
  this->NestedSeparatorFrame = vtkKWFrame::New();
  this->NameField = vtkKWText::New();
  this->Checkbox = vtkKWCheckButton::New();
  this->MacroFlag = 0;
  this->Location=0;
  this->MainFrameCollapsedState = 0;
}

//----------------------------------------------------------------------------
vtkKWLookmarkFolder::~vtkKWLookmarkFolder()
{
  this->RemoveFolder();
}


void vtkKWLookmarkFolder::RemoveFolder()
{
  if(this->NameField)
    {
    this->NameField->Delete();
    this->NameField = NULL;
    }

  if(this->Checkbox)
    {
    this->Checkbox->Delete();
    this->Checkbox = NULL;
    }

  if(this->MainFrame)
    {
    this->MainFrame->Delete();
    this->MainFrame = NULL;
    }

  if(this->LabelFrame)
    {
    this->LabelFrame->Delete();
    this->LabelFrame= NULL;
    }
  if(this->SeparatorFrame)
    {
    this->SeparatorFrame->Delete();
    this->SeparatorFrame = 0;
    }
  if(this->NestedSeparatorFrame)
    {
    this->NestedSeparatorFrame->Delete();
    this->NestedSeparatorFrame = 0;
    }
}


//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::Create()
{
  // Check if already created
  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget
  this->Superclass::Create();

  this->MainFrame->SetParent(this);
  this->MainFrame->Create();

  this->LabelFrame->SetParent(this->MainFrame);
  this->LabelFrame->AllowFrameToCollapseOn();
  this->LabelFrame->Create();
  this->LabelFrame->SetLabelText("Folder");
  this->LabelFrame->GetLabel()->SetBalloonHelpString("Drag and drop folder");

  this->Checkbox->SetParent(this->LabelFrame->GetLabelFrame());
  this->Checkbox->IndicatorVisibilityOn();
  this->Checkbox->Create();
  this->Checkbox->SetSelectedState(0);

  if(!this->MacroFlag)
    {
    this->GetDragAndDropTargetSet()->SetStartCommand(
      this, "DragAndDropStartCallback");
    this->GetDragAndDropTargetSet()->SetEndCommand(
      this, "DragAndDropEndCallback");
    this->GetDragAndDropTargetSet()->SetSourceAnchor(
      this->LabelFrame->GetLabel());
    }

  this->SeparatorFrame->SetParent(this);
  this->SeparatorFrame->Create();

  this->NestedSeparatorFrame->SetParent(this->LabelFrame->GetFrame());
  this->NestedSeparatorFrame->Create();

  this->NameField->SetParent(this->LabelFrame->GetLabelFrame());
  this->NameField->Create();
  this->NameField->SetState(vtkKWTkOptions::StateNormal);

  this->Pack();

  this->UpdateEnableState();

}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::DragAndDropPerformCommand(int x, int y, vtkKWWidget *vtkNotUsed(widget), vtkKWWidget *vtkNotUsed(anchor))
{
  if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->SeparatorFrame->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 2 -relief groove", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 0 -relief flat", this->NestedSeparatorFrame->GetWidgetName());
    }
  else if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->NestedSeparatorFrame->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 2 -relief groove", this->NestedSeparatorFrame->GetWidgetName());
    }
  else if (  vtkKWTkUtilities::ContainsCoordinates(
        this->GetApplication()->GetMainInterp(),
        this->LabelFrame->GetLabel()->GetWidgetName(),
        x, y))
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 2 -relief groove", this->NestedSeparatorFrame->GetWidgetName());
    }
  else 
    {
    this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
    this->Script("%s configure -bd 0 -relief flat", this->NestedSeparatorFrame->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::RemoveDragAndDropTargetCues()
{
  this->Script("%s configure -bd 0 -relief flat", this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -bd 0 -relief flat", this->NestedSeparatorFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::SetFolderName(const char *name)
{
  this->LabelFrame->SetLabelText(name);
}

//----------------------------------------------------------------------------
char* vtkKWLookmarkFolder::GetFolderName()
{
  return this->LabelFrame->GetLabel()->GetText();
}


//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::EditCallback()
{
  char *temp = new char[100];

  strcpy(temp,this->LabelFrame->GetLabel()->GetText());
  this->LabelFrame->SetLabelText("");
  this->Script("pack %s", this->NameField->GetWidgetName());
  this->Script("%s configure -bg white -height 1 -width %d -wrap none", this->NameField->GetWidgetName(), strlen(temp));
  if(this->NameField)
    this->NameField->SetText(temp);
  this->NameField->SetBinding("<KeyPress-Return>", this, "ChangeName");

  delete [] temp;
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::ChangeName()
{
  char *lmkName = new char[100];

  strcpy(lmkName,this->NameField->GetText());
  this->NameField->Unpack();
  this->Script("pack %s -side left -fill x -expand t -padx 2", this->LabelFrame->GetLabel()->GetWidgetName());
  this->LabelFrame->SetLabelText(lmkName);

  this->ToggleNestedCheckBoxes(this->LabelFrame, 0);

  delete [] lmkName;
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::Pack()
{
  // Unpack everything
  this->MainFrame->Unpack();
  this->SeparatorFrame->Unpack();

  this->Script("pack %s -anchor nw -expand t -fill x -side top", 
              this->NestedSeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",
              this->NestedSeparatorFrame->GetWidgetName()); 
  if(this->MacroFlag==0)//
    {
    this->Script("pack %s -before %s -anchor nw -side left", this->Checkbox->GetWidgetName(),this->LabelFrame->GetLabel()->GetWidgetName());
    }
  this->Script("pack %s -fill x -expand t -side left", 
              this->LabelFrame->GetWidgetName());
  this->Script("%s configure -bd 3",
              this->LabelFrame->GetFrame()->GetParent()->GetWidgetName());
  this->Script("pack %s -anchor w -fill x -expand t", 
              this->MainFrame->GetWidgetName());
  this->Script("pack %s -anchor nw -expand t -fill x", 
              this->SeparatorFrame->GetWidgetName());
  this->Script("%s configure -height 12",
              this->SeparatorFrame->GetWidgetName());

  this->UpdateWidgetValues();

}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::DragAndDropStartCallback(int, int )
{
  this->ToggleNestedLabels(this->LabelFrame,1);
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::DragAndDropEndCallback(int, int)
{
  this->ToggleNestedLabels(this->LabelFrame,0);
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::SetSelectionState(int flag)
{
  // if this is a macros folder don't let it be selected
  if(!this->MacroFlag)
    {
    this->Checkbox->SetSelectedState(flag);
    }
}

//----------------------------------------------------------------------------
int vtkKWLookmarkFolder::GetSelectionState()
{
  return this->Checkbox->GetSelectedState();
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::SelectCallback()
{
  if(this->Checkbox->GetSelectedState())
    {
    this->ToggleNestedCheckBoxes(this->LabelFrame, 1);
    }
  else
    {
    this->ToggleNestedCheckBoxes(this->LabelFrame, 0);
    }
}


void vtkKWLookmarkFolder::ToggleNestedCheckBoxes(vtkKWWidget *parent, int onoff)
{
  int nb_children = parent->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    vtkKWWidget *widget = parent->GetNthChild(i);
    if(widget->IsA("vtkKWCheckButton") && widget->IsPacked())
      {
      vtkKWCheckButton *checkBox = vtkKWCheckButton::SafeDownCast(widget);
      if(checkBox)
        {
        checkBox->SetSelectedState(onoff);
        }
      //else cleanup
      }
    else if(!widget->IsA("vtkKWCheckButtonWithLabel"))
      {
        this->ToggleNestedCheckBoxes(widget, onoff);
      }
    }
}


//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::ToggleNestedLabels(vtkKWWidget *widget, int onoff)
{
  vtkKWLookmark *lmkWidget;
  vtkKWLookmarkFolder *lmkFolder;

 
  if(!widget)
    {
    return;
    }

  if( widget->IsA("vtkKWLookmark") && widget->IsPacked())
    {
    lmkWidget = vtkKWLookmark::SafeDownCast(widget);
    if(lmkWidget)
      {
      double fr, fg, fb, br, bg, bb;
      vtkKWCoreWidget *anchor_as_core = vtkKWCoreWidget::SafeDownCast(
        lmkWidget->GetDragAndDropTargetSet()->GetSourceAnchor());
      anchor_as_core->GetConfigurationOptionAsColor("-fg", &fr, &fg, &fb);
      anchor_as_core->GetConfigurationOptionAsColor("-bg", &br, &bg, &bb);
      anchor_as_core->SetConfigurationOptionAsColor("-fg", br, bg, bb);
      anchor_as_core->SetConfigurationOptionAsColor("-bg", fr, fg, fb);
      }
    }
  else if( widget->IsA("vtkKWLookmarkFolder") && widget->IsPacked())
    {
    lmkFolder = vtkKWLookmarkFolder::SafeDownCast(widget);
    if(lmkFolder)
      {
      double fr, fg, fb, br, bg, bb;
      vtkKWCoreWidget *anchor_as_core = vtkKWCoreWidget::SafeDownCast(
        lmkFolder->GetDragAndDropTargetSet()->GetSourceAnchor());
      anchor_as_core->GetConfigurationOptionAsColor("-fg", &fr, &fg, &fb);
      anchor_as_core->GetConfigurationOptionAsColor("-bg", &br, &bg, &bb);
      anchor_as_core->SetConfigurationOptionAsColor("-fg", br, bg, bb);
      anchor_as_core->SetConfigurationOptionAsColor("-bg", fr, fg, fb);

      lmkFolder->ToggleNestedLabels(lmkFolder->GetLabelFrame()->GetFrame(), onoff);
      }
    }
  else
    {
    int nb_children = widget->GetNumberOfChildren();
    for (int i = 0; i < nb_children; i++)
      {
      this->ToggleNestedLabels(widget->GetNthChild(i), onoff);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::UpdateWidgetValues()
{
  if(this->MainFrameCollapsedState)
    {
    this->LabelFrame->CollapseFrame();
    }
  else
    {
    this->LabelFrame->ExpandFrame();
    }
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::UpdateVariableValues()
{
  this->SetMainFrameCollapsedState(this->LabelFrame->IsFrameCollapsed());
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->MainFrame);
  this->PropagateEnableState(this->LabelFrame);
  this->PropagateEnableState(this->Checkbox);
  this->PropagateEnableState(this->SeparatorFrame);
  this->PropagateEnableState(this->NestedSeparatorFrame);
}

//----------------------------------------------------------------------------
void vtkKWLookmarkFolder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Location: " << this->Location << endl;
  os << indent << "LabelFrame: " << this->LabelFrame << endl;
  os << indent << "SeparatorFrame: " << this->SeparatorFrame << endl;
  os << indent << "NestedSeparatorFrame: " << this->NestedSeparatorFrame << endl;
  os << indent << "MacroFlag: " << this->GetMacroFlag() << endl;
  os << indent << "Checkbox: " << this->GetCheckbox() << endl;
  os << indent << "MainFrameCollapsedState: " << this->GetMainFrameCollapsedState() << endl;

}
