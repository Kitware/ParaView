/*=========================================================================

  Module:    vtkKWListBoxToListBoxSelectionEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWListBoxToListBoxSelectionEditor.h"

#include "vtkKWIcon.h"
#include "vtkCommand.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWListBoxWithScrollbars.h"
#include "vtkKWListBoxWithScrollbarsWithLabel.h"
#include "vtkKWIcon.h"
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkKWFrame.h"
#include "vtkKWEvent.h"

#include <vtksys/stl/string>
#include <vtksys/stl/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWListBoxToListBoxSelectionEditor );
vtkCxxRevisionMacro(vtkKWListBoxToListBoxSelectionEditor, "1.25");

//----------------------------------------------------------------------------
vtkKWListBoxToListBoxSelectionEditor::vtkKWListBoxToListBoxSelectionEditor()
{
  this->SourceList = vtkKWListBoxWithScrollbarsWithLabel::New();
  this->FinalList = vtkKWListBoxWithScrollbarsWithLabel::New();

  this->AddButton = vtkKWPushButton::New();
  this->AddAllButton = vtkKWPushButton::New();
  this->RemoveButton = vtkKWPushButton::New();
  this->ButtonFrame = vtkKWFrame::New();
  this->RemoveAllButton = vtkKWPushButton::New();
  this->UpButton = vtkKWPushButton::New();
  this->DownButton = vtkKWPushButton::New();
  this->EllipsisCommand = 0;
  this->FinalListChangedCommand = 0;
  this->EllipsisDisplayed = 0;
  this->AllowReordering = 1;
}

//----------------------------------------------------------------------------
vtkKWListBoxToListBoxSelectionEditor::~vtkKWListBoxToListBoxSelectionEditor()
{
  this->SourceList->Delete();
  this->FinalList->Delete();

  this->AddButton->Delete();
  this->AddAllButton->Delete();
  this->RemoveButton->Delete();
  this->RemoveAllButton->Delete();
  this->UpButton->Delete();
  this->DownButton->Delete();
  this->ButtonFrame->Delete();

  if (this->EllipsisCommand)
    {
    delete [] this->EllipsisCommand;
    this->EllipsisCommand = NULL;
    }
  if(this->FinalListChangedCommand)
    {
    delete [] this->FinalListChangedCommand;
    this->FinalListChangedCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SourceList->SetParent(this);
  this->SourceList->SetLabelVisibility(0);
  this->SourceList->SetLabelPositionToTop();
  this->SourceList->Create();
  this->Script("pack %s -side left -expand true -fill both",
    this->SourceList->GetWidgetName());

  vtkKWListBox *listbox = this->SourceList->GetWidget()->GetWidget();
  listbox->SetSelectionCommand(this, "SourceSelectionChangedCallback");
  listbox->SetSelectionModeToExtended();

  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create();
  int width = 32;

  this->AddButton->SetParent(frame);
  this->AddButton->Create();
  this->AddButton->SetText(ks_("List Box To List Box|Button|Add"));
  this->AddButton->SetBalloonHelpString(this->AddButton->GetText());
  this->AddButton->SetImageToPredefinedIcon(vtkKWIcon::IconTransportPlay);
  this->AddButton->SetWidth(width);
  this->AddButton->SetCommand(this, "AddCallback");

  this->AddAllButton->SetParent(frame);
  this->AddAllButton->Create();
  this->AddAllButton->SetText(ks_("List Box To List Box|Button|Add All"));
  this->AddAllButton->SetBalloonHelpString(this->AddAllButton->GetText());
  this->AddAllButton->SetImageToPredefinedIcon(
    vtkKWIcon::IconTransportFastForward);
  this->AddAllButton->SetWidth(width);
  this->AddAllButton->SetCommand(this, "AddAllCallback");

  this->RemoveButton->SetParent(frame);
  this->RemoveButton->Create();
  this->RemoveButton->SetText(ks_("List Box To List Box|Button|Remove"));
  this->RemoveButton->SetBalloonHelpString(this->RemoveButton->GetText());
  this->RemoveButton->SetImageToPredefinedIcon(
    vtkKWIcon::IconTransportPlayBackward);
  this->RemoveButton->SetWidth(width);
  this->RemoveButton->SetCommand(this, "RemoveCallback");

  this->RemoveAllButton->SetParent(frame);
  this->RemoveAllButton->Create();
  this->RemoveAllButton->SetText(
    ks_("List Box To List Box|Button|Remove All"));
  this->RemoveAllButton->SetBalloonHelpString(
    this->RemoveAllButton->GetText());
  this->RemoveAllButton->SetImageToPredefinedIcon(
    vtkKWIcon::IconTransportRewind);
  this->RemoveAllButton->SetWidth(width);
  this->RemoveAllButton->SetCommand(this, "RemoveAllCallback");

  this->Script("pack %s %s %s %s -side top -fill x -padx 4 -pady 0",
               this->AddButton->GetWidgetName(),
               this->AddAllButton->GetWidgetName(),
               this->RemoveButton->GetWidgetName(),
               this->RemoveAllButton->GetWidgetName());

  this->Script("pack %s %s -pady 2",
               this->AddAllButton->GetWidgetName(),
               this->RemoveAllButton->GetWidgetName());

  this->Script("pack %s -side left -expand false -fill y -pady 40",
    frame->GetWidgetName());
  frame->Delete();

  frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create();

  this->FinalList->SetParent(frame);
  this->FinalList->SetLabelVisibility(0);
  this->FinalList->SetLabelPositionToTop();
  this->FinalList->Create();
  this->Script("pack %s -side top -expand true -fill both",
    this->FinalList->GetWidgetName());

  listbox = this->FinalList->GetWidget()->GetWidget();
  listbox->SetSelectionModeToExtended();
  listbox->SetSelectionCommand(this, "FinalSelectionChangedCallback");

  this->ButtonFrame->SetParent(frame);
  this->ButtonFrame->Create();

  this->UpButton->SetParent(this->ButtonFrame);
  this->UpButton->Create();
  this->UpButton->SetText(ks_("List Box To List Box|Button|Up"));
  this->UpButton->SetBalloonHelpString(this->UpButton->GetText());
  this->UpButton->SetImageToPredefinedIcon(vtkKWIcon::IconSpinUp);
  this->UpButton->SetHeight(16);
  this->UpButton->SetCommand(this, "UpCallback");

  this->DownButton->SetParent(this->ButtonFrame);
  this->DownButton->Create();
  this->DownButton->SetText(ks_("List Box To List Box|Button|Down"));
  this->DownButton->SetBalloonHelpString(this->DownButton->GetText());
  this->DownButton->SetImageToPredefinedIcon(vtkKWIcon::IconSpinDown);
  this->DownButton->SetHeight(16);
  this->DownButton->SetCommand(this, "DownCallback");

  this->Script("grid %s -column 0 -row 0 -stick ew  -padx 1 -pady 2",
    this->UpButton->GetWidgetName());

  this->Script("grid %s -column 1 -row 0 -stick ew  -padx 1 -pady 2",
    this->DownButton->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1 -uniform col",
               this->ButtonFrame->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 1 -uniform col",
               this->ButtonFrame->GetWidgetName());

  this->Script("pack %s %s -side left -fill x -expand y -padx 1 -pady 2",
    this->UpButton->GetWidgetName(),
    this->DownButton->GetWidgetName());

  this->Pack();

  this->Script("pack %s -side left -expand true -fill both",
    frame->GetWidgetName());
  frame->Delete();
  this->DisplayEllipsis();
  this->Update();
}
  
//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::Pack() 
{
  if(this->AllowReordering)
    {
    this->Script("pack %s -side top -expand false -fill x",
      this->ButtonFrame->GetWidgetName());
    }
  else if(this->ButtonFrame->IsPacked())
    {
    this->ButtonFrame->Unpack();
    }
}  
//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddElement(vtkKWListBox* l1, vtkKWListBox* l2, 
  const char* element, int force)
{
  int idx = l2->GetItemIndex(element);
  if ( idx >= 0 )
    {
    if ( force )
      {
      l2->DeleteRange(idx, idx);
      l1->AppendUnique(element);
      }
    }
  else
    {
    l1->AppendUnique(element);
    }
  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddSourceElement(const char* element, int force /* = 0 */)
{
  this->RemoveEllipsis();
  this->AddElement(this->SourceList->GetWidget()->GetWidget(), 
                   this->FinalList->GetWidget()->GetWidget(), element, force);
  this->DisplayEllipsis();
  this->Update();
}
//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddFinalElement(const char* element, int force /* = 0 */)
{
  this->RemoveEllipsis();
  this->AddElement(this->FinalList->GetWidget()->GetWidget(), 
                   this->SourceList->GetWidget()->GetWidget(), element, force);
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveSourceElement(int index)
{
  this->RemoveEllipsis();
  if(index<this->SourceList->GetWidget()->GetWidget()->GetNumberOfItems())
    {
    this->SourceList->GetWidget()->GetWidget()->DeleteRange(index, index);
    }
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveFinalElement(int index)
{
  this->RemoveEllipsis();
  if(index<this->FinalList->GetWidget()->GetWidget()->GetNumberOfItems())
    {
    this->FinalList->GetWidget()->GetWidget()->DeleteRange(index, index);
    }
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddCallback()
{
  this->RemoveEllipsis();
  this->MoveSelectedList(this->SourceList->GetWidget()->GetWidget(), 
                         this->FinalList->GetWidget()->GetWidget());
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddAllCallback()
{
  this->RemoveEllipsis();
  this->MoveWholeList(this->SourceList->GetWidget()->GetWidget(), 
                      this->FinalList->GetWidget()->GetWidget());
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveCallback() 
{
  this->RemoveEllipsis();
  this->MoveSelectedList(this->FinalList->GetWidget()->GetWidget(), 
                         this->SourceList->GetWidget()->GetWidget());
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveAllCallback()
{
  this->RemoveEllipsis();
  this->MoveWholeList(this->FinalList->GetWidget()->GetWidget(), 
                      this->SourceList->GetWidget()->GetWidget());
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::ShiftItems(vtkKWListBox* l1, int down)
{
  const char* list = this->Script("lsort -integer %s [ %s curselection ]", 
    down?"-decreasing":"",
    l1->GetWidgetName());
  char* selection = new char[ strlen(list) + 1 ];
  strcpy(selection, list);
  int idx = -1;
  int size = l1->GetNumberOfItems();
  istrstream sel(selection);
  vtksys_stl::string item;
  while(sel >> idx)
    {
    if ( idx < 0 )
      {
      break;
      }
    int newidx = idx - 1;
    if ( down )
      {
      newidx = idx + 1;
      }
    if ( newidx >= 0 && newidx < size )
      {
      item = l1->GetItem(idx);
      l1->DeleteRange(idx, idx);
      l1->InsertEntry(newidx, item.c_str());
      this->Script("%s selection set %d %d",
        l1->GetWidgetName(), newidx, newidx);
      }

    idx = -1;
    }
  delete [] selection;
  this->Modified();
  this->InvokeFinalListChangedCommand();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::UpCallback()
{
  this->ShiftItems(this->FinalList->GetWidget()->GetWidget(), 0);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::DownCallback()
{
  this->ShiftItems(this->FinalList->GetWidget()->GetWidget(), 1);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::MoveWholeList(vtkKWListBox* l1, vtkKWListBox* l2)
{
  this->Script("%s selection set 0 end",
    l1->GetWidgetName());
  this->MoveSelectedList(l1, l2);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::MoveSelectedList(vtkKWListBox* l1, vtkKWListBox* l2)
{
  const char* cselected = this->Script("%s curselection", 
    l1->GetWidgetName());
  this->MoveList(l1, l2, cselected);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::MoveList(vtkKWListBox* l1, vtkKWListBox* l2, 
  const char* list)
{
  char* selection = new char[ strlen(list) + 1 ];
  strcpy(selection, list);
  int idx = -1;
  vtksys_stl::string str;
  vtksys_stl::vector<int> vec;
  istrstream sel(selection);
  while(sel >> idx)
    {
    if ( idx < 0 )
      {
      break;
      }
    str = l1->GetItem(idx);
    l2->AppendUnique(str.c_str());
    vec.push_back(idx);
    idx = -1;
    }
  while( vec.size() )
    {
    idx = vec[vec.size() - 1];
    l1->DeleteRange(idx,idx);
    vec.erase(vec.end()-1);
    }
  delete [] selection;
  
  this->Modified();
  this->InvokeFinalListChangedCommand();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetNumberOfElementsOnSourceList()
{
  if ( this->EllipsisDisplayed )
    {
    return 0;
    }
  return this->SourceList->GetWidget()->GetWidget()->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetNumberOfElementsOnFinalList()
{
  return this->FinalList->GetWidget()->GetWidget()->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkKWListBoxToListBoxSelectionEditor::GetElementFromSourceList(int idx)
{
  if ( this->EllipsisDisplayed )
    {
    return 0;
    }
  return this->SourceList->GetWidget()->GetWidget()->GetItem(idx);
}

//----------------------------------------------------------------------------
const char* vtkKWListBoxToListBoxSelectionEditor::GetElementFromFinalList(int idx)
{
  return this->FinalList->GetWidget()->GetWidget()->GetItem(idx);
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetElementIndexFromSourceList(const char* element)
{
  if ( this->EllipsisDisplayed )
    {
    return -1;
    }
  return this->SourceList->GetWidget()->GetWidget()->GetItemIndex(element);
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetElementIndexFromFinalList(const char* element)
{
  return this->FinalList->GetWidget()->GetWidget()->GetItemIndex(element);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveItemsFromSourceList()
{
  this->SourceList->GetWidget()->GetWidget()->DeleteAll();
  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
  this->DisplayEllipsis();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveItemsFromFinalList()
{
  this->FinalList->GetWidget()->GetWidget()->DeleteAll();
  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::DisplayEllipsis()
{
  if ( this->SourceList->GetWidget()->GetWidget()->GetNumberOfItems() > 0 )
    {
    return;
    }
  this->SourceList->GetWidget()->GetWidget()->InsertEntry(
    0, ks_("List Box To List Box|Ellipsis|..."));
  this->SourceList->GetWidget()->GetWidget()->SetBinding(
    "<Double-1>", this, "EllipsisCallback");
  this->EllipsisDisplayed = 1;
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveEllipsis()
{
  if ( !this->EllipsisDisplayed )
    {
    return;
    }
  this->SourceList->GetWidget()->GetWidget()->DeleteAll();
  this->SourceList->GetWidget()->GetWidget()->RemoveBinding("<Double-1>");
  this->EllipsisDisplayed = 0;
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::FinalSelectionChangedCallback()
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::SourceSelectionChangedCallback()
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::EllipsisCallback()
{
  this->InvokeEllipsisCommand();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::SetEllipsisCommand(
  vtkObject *obj, const char *method)
{
  this->SetObjectMethodCommand(&this->EllipsisCommand, obj, method);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::SetFinalListChangedCommand(
  vtkObject *obj, const char *method)
{
  this->SetObjectMethodCommand(&this->FinalListChangedCommand, obj, method);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::InvokeEllipsisCommand()
{
  this->InvokeObjectMethodCommand(this->EllipsisCommand);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::InvokeFinalListChangedCommand()
{
  if(this->FinalListChangedCommand &&
    *this->FinalListChangedCommand)
  {
  this->InvokeObjectMethodCommand(this->FinalListChangedCommand);
  }
}

//-----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->SourceList);
  this->PropagateEnableState(this->FinalList);

  this->PropagateEnableState(this->AddButton);
  this->PropagateEnableState(this->AddAllButton);
  this->PropagateEnableState(this->RemoveButton);
  this->PropagateEnableState(this->RemoveAllButton);
  this->PropagateEnableState(this->UpButton);
  this->PropagateEnableState(this->DownButton);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::Update()
{
  this->UpdateEnableState();

  // Add and AddAll buttons

  if ( this->SourceList->GetWidget()->GetWidget()->GetNumberOfItems() == 0 
    || this->EllipsisDisplayed)
    {
    this->AddButton->SetEnabled(0);
    this->AddAllButton->SetEnabled(0);
    }
  else if(this->SourceList->GetWidget()->GetWidget()->GetSelectionIndex()<0)
    {
    this->AddButton->SetEnabled(0);
    }

  // Remove and RemoveAll buttons

  if ( this->FinalList->GetWidget()->GetWidget()->GetNumberOfItems() == 0)
    {
    this->RemoveButton->SetEnabled(0);
    this->RemoveAllButton->SetEnabled(0);
    }
  else if(this->FinalList->GetWidget()->GetWidget()->GetSelectionIndex()<0)
    {
    this->RemoveButton->SetEnabled(0);
    }

  // Up and Down buttons

  if ( this->FinalList->GetWidget()->GetWidget()->GetNumberOfItems() <= 1
    || this->FinalList->GetWidget()->GetWidget()->GetSelectionIndex()<0)
    {
    this->UpButton->SetEnabled(0);
    this->DownButton->SetEnabled(0);
    }
  else if(this->FinalList->GetWidget()->GetWidget()->GetSelectionIndex() ==0)
    {
    this->UpButton->SetEnabled(0);
    }
  else if(this->FinalList->GetWidget()->GetWidget()->GetSelectionIndex() ==
    (this->FinalList->GetWidget()->GetWidget()->GetNumberOfItems()-1))
    {
    this->DownButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::SetAllowReordering(
  int allowed)
{
  if(this->AllowReordering == allowed)
    {
    return;
    }
  this->AllowReordering = allowed;

  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

