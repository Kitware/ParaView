/*=========================================================================

  Module:    vtkPVListBoxToListBoxSelectionEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVListBoxToListBoxSelectionEditor.h"

#include "vtkCommand.h"
#include "vtkKWListBox.h"
#include "vtkKWListBoxWithScrollbars.h"
#include "vtkObjectFactory.h"
#include "vtkStringList.h"

#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVListBoxToListBoxSelectionEditor);
vtkCxxRevisionMacro(vtkPVListBoxToListBoxSelectionEditor, "1.1");
//-----------------------------------------------------------------------------
vtkPVListBoxToListBoxSelectionEditor::vtkPVListBoxToListBoxSelectionEditor()
{
}

//-----------------------------------------------------------------------------
vtkPVListBoxToListBoxSelectionEditor::~vtkPVListBoxToListBoxSelectionEditor()
{
}

//-----------------------------------------------------------------------------
void vtkPVListBoxToListBoxSelectionEditor::SetList(vtkStringList* list,
  vtkKWListBox* toAdd, vtkKWListBox* toComp, int force)
{
  this->RemoveEllipsis();

  toAdd->DeleteAll();

  int maxToAdd = list->GetNumberOfStrings();
  int cc;
  if (force)
    {
    vtkstd::set <vtkstd::string> setToAdd;
    for (cc=0; cc < maxToAdd; cc++)
      {
      setToAdd.insert(list->GetString(cc));
      }
    vtkstd::vector <vtkstd::string> vecCompare;
    int max_comp = toComp->GetNumberOfItems();
    for (cc=0; cc <max_comp; cc++)
      {
      vecCompare.push_back(toComp->GetItem(cc));
      }
    
    // push in items in list.
    for (cc=0; cc < maxToAdd; cc++)
      {
      toAdd->InsertEntry(cc, list->GetString(cc));
      }

    // Add only those strings in toComp that are no in toAdd.
    toComp->DeleteAll();
    vtkstd::vector<vtkstd::string>::iterator iter;
    cc = 0;
    for (iter=vecCompare.begin(); iter!=vecCompare.end(); ++iter)
      {
      const char* str = iter->c_str();
      if (setToAdd.find(str) == setToAdd.end())
        {
        toComp->InsertEntry(cc++, str);
        }
      }
    }
  else
    {
    vtkstd::set <vtkstd::string> setCompare;
    int max_comp = toComp->GetNumberOfItems();
    for (cc=0; cc < max_comp; cc++)
      {
      setCompare.insert(toComp->GetItem(cc));
      }
    int count=0;
    for (cc=0; cc < maxToAdd; cc++)
      {
      const char* str = list->GetString(cc);
      if (setCompare.find(str) == setCompare.end())
        {
        toAdd->InsertEntry(count++, str);
        }
      }
    }
  this->DisplayEllipsis();
}

//-----------------------------------------------------------------------------
void vtkPVListBoxToListBoxSelectionEditor::SetSourceList(vtkStringList* list,
  int force)
{
  this->SetList(list, this->SourceList->GetWidget(),
    this->FinalList->GetWidget(), force);

  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkPVListBoxToListBoxSelectionEditor::SetFinalList(vtkStringList* list,
  int force)
{
  this->SetList(list, this->FinalList->GetWidget(),
    this->SourceList->GetWidget(), force);

  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkPVListBoxToListBoxSelectionEditor::MoveWholeList(vtkKWListBox* l1, 
  vtkKWListBox* l2)
{
  vtkStringList* list = vtkStringList::New();
  int cc;
  int maxL1 = l1->GetNumberOfItems();
  
  for (cc=0; cc < maxL1; cc++)
    {
    list->AddString(l1->GetItem(cc));
    }
  l1->DeleteAll();
  this->SetList(list, l2, l1, 0);
  list->Delete();

  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkPVListBoxToListBoxSelectionEditor::PrintSelf(ostream& os, 
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
