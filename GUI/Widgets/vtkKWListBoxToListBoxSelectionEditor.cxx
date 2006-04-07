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
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkKWFrame.h"
#include "vtkKWEvent.h"

#include <vtksys/stl/string>
#include <vtksys/stl/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWListBoxToListBoxSelectionEditor );
vtkCxxRevisionMacro(vtkKWListBoxToListBoxSelectionEditor, "1.15");

//----------------------------------------------------------------------------
vtkKWListBoxToListBoxSelectionEditor::vtkKWListBoxToListBoxSelectionEditor()
{
  this->SourceList = vtkKWListBoxWithScrollbars::New();
  this->FinalList = vtkKWListBoxWithScrollbars::New();

  this->AddButton = vtkKWPushButton::New();
  this->AddAllButton = vtkKWPushButton::New();
  this->RemoveButton = vtkKWPushButton::New();
  this->RemoveAllButton = vtkKWPushButton::New();
  this->UpButton = vtkKWPushButton::New();
  this->DownButton = vtkKWPushButton::New();
  this->EllipsisCommand = 0;
  this->EllipsisDisplayed = 0;
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

  if (this->EllipsisCommand)
    {
    delete [] this->EllipsisCommand;
    this->EllipsisCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  this->SourceList->SetParent(this);
  this->SourceList->Create();
  this->SourceList->GetWidget()->SetSelectionModeToExtended();
  this->Script("pack %s -side left -expand true -fill both",
    this->SourceList->GetWidgetName());

  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create();

  this->AddButton->SetParent(frame);
  this->AddButton->Create();
  this->AddButton->SetText(ks_("List Box To List Box|Button|Add"));
  this->AddButton->SetCommand(this, "AddCallback");

  this->AddAllButton->SetParent(frame);
  this->AddAllButton->Create();
  this->AddAllButton->SetText(ks_("List Box To List Box|Button|Add All"));
  this->AddAllButton->SetCommand(this, "AddAllCallback");

  this->RemoveButton->SetParent(frame);
  this->RemoveButton->Create();
  this->RemoveButton->SetText(ks_("List Box To List Box|Button|Remove"));
  this->RemoveButton->SetCommand(this, "RemoveCallback");

  this->RemoveAllButton->SetParent(frame);
  this->RemoveAllButton->Create();
  this->RemoveAllButton->SetText(
    ks_("List Box To List Box|Button|Remove All"));
  this->RemoveAllButton->SetCommand(this, "RemoveAllCallback");

  this->Script("pack %s %s %s %s -side top -fill x -padx 2 -pady 1",
    this->AddButton->GetWidgetName(),
    this->AddAllButton->GetWidgetName(),
    this->RemoveButton->GetWidgetName(),
    this->RemoveAllButton->GetWidgetName());

  this->Script("pack %s -side left -expand false -fill y",
    frame->GetWidgetName());
  frame->Delete();

  frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create();

  this->FinalList->SetParent(frame);
  this->FinalList->Create();
  this->FinalList->GetWidget()->SetSelectionModeToExtended();

  this->Script("pack %s -side top -expand true -fill both",
    this->FinalList->GetWidgetName());

  vtkKWFrame* btframe = vtkKWFrame::New();
  btframe->SetParent(frame);
  btframe->Create();

  this->UpButton->SetParent(btframe);
  this->UpButton->Create();
  this->UpButton->SetText(ks_("List Box To List Box|Button|Up"));
  this->UpButton->SetCommand(this, "UpCallback");

  this->DownButton->SetParent(btframe);
  this->DownButton->Create();
  this->DownButton->SetText(ks_("List Box To List Box|Button|Down"));
  this->DownButton->SetCommand(this, "DownCallback");

  this->Script("pack %s %s -side left -fill x -expand y -padx 1 -pady 2",
    this->UpButton->GetWidgetName(),
    this->DownButton->GetWidgetName());

  this->Script("pack %s -side top -expand false -fill x",
    btframe->GetWidgetName());
  btframe->Delete();
  
  this->Script("pack %s -side left -expand true -fill both",
    frame->GetWidgetName());
  frame->Delete();
  this->DisplayEllipsis();
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
  this->AddElement(this->SourceList->GetWidget(), 
                   this->FinalList->GetWidget(), element, force);
  this->DisplayEllipsis();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddFinalElement(const char* element, int force /* = 0 */)
{
  this->RemoveEllipsis();
  this->AddElement(this->FinalList->GetWidget(), 
                   this->SourceList->GetWidget(), element, force);
  this->DisplayEllipsis();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddCallback()
{
  this->RemoveEllipsis();
  this->MoveSelectedList(this->SourceList->GetWidget(), 
                         this->FinalList->GetWidget());
  this->DisplayEllipsis();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::AddAllCallback()
{
  this->RemoveEllipsis();
  this->MoveWholeList(this->SourceList->GetWidget(), 
                      this->FinalList->GetWidget());
  this->DisplayEllipsis();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveCallback() 
{
  this->RemoveEllipsis();
  this->MoveSelectedList(this->FinalList->GetWidget(), 
                         this->SourceList->GetWidget());
  this->DisplayEllipsis();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveAllCallback()
{
  this->RemoveEllipsis();
  this->MoveWholeList(this->FinalList->GetWidget(), 
                      this->SourceList->GetWidget());
  this->DisplayEllipsis();
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
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::UpCallback()
{
  this->ShiftItems(this->FinalList->GetWidget(), 0);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::DownCallback()
{
  this->ShiftItems(this->FinalList->GetWidget(), 1);
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
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetNumberOfElementsOnSourceList()
{
  if ( this->EllipsisDisplayed )
    {
    return 0;
    }
  return this->SourceList->GetWidget()->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetNumberOfElementsOnFinalList()
{
  return this->FinalList->GetWidget()->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkKWListBoxToListBoxSelectionEditor::GetElementFromSourceList(int idx)
{
  if ( this->EllipsisDisplayed )
    {
    return 0;
    }
  return this->SourceList->GetWidget()->GetItem(idx);
}

//----------------------------------------------------------------------------
const char* vtkKWListBoxToListBoxSelectionEditor::GetElementFromFinalList(int idx)
{
  return this->FinalList->GetWidget()->GetItem(idx);
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetElementIndexFromSourceList(const char* element)
{
  if ( this->EllipsisDisplayed )
    {
    return -1;
    }
  return this->SourceList->GetWidget()->GetItemIndex(element);
}

//----------------------------------------------------------------------------
int vtkKWListBoxToListBoxSelectionEditor::GetElementIndexFromFinalList(const char* element)
{
  return this->FinalList->GetWidget()->GetItemIndex(element);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveItemsFromSourceList()
{
  this->SourceList->GetWidget()->DeleteAll();
  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
  this->DisplayEllipsis();
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::RemoveItemsFromFinalList()
{
  this->FinalList->GetWidget()->DeleteAll();
  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWListBoxToListBoxSelectionEditor::DisplayEllipsis()
{
  if ( this->SourceList->GetWidget()->GetNumberOfItems() > 0 )
    {
    return;
    }
  this->SourceList->GetWidget()->InsertEntry(
    0, ks_("List Box To List Box|Ellipsis|..."));
  this->SourceList->GetWidget()->SetBinding(
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
  this->SourceList->GetWidget()->DeleteAll();
  this->SourceList->GetWidget()->RemoveBinding("<Double-1>");
  this->EllipsisDisplayed = 0;
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
void vtkKWListBoxToListBoxSelectionEditor::InvokeEllipsisCommand()
{
  this->InvokeObjectMethodCommand(this->EllipsisCommand);
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
void vtkKWListBoxToListBoxSelectionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

