/*=========================================================================

  Module:    vtkKWEntrySet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWEntrySet.h"

#include "vtkKWApplication.h"
#include "vtkKWEntry.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWEntrySet);
vtkCxxRevisionMacro(vtkKWEntrySet, "1.1");

int vtkvtkKWEntrySetCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWEntrySet::vtkKWEntrySet()
{
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetInPackingDirection = 0;
  this->PadX = 0;
  this->PadY = 0;
  this->Entries = vtkKWEntrySet::EntriesContainer::New();
}

//----------------------------------------------------------------------------
vtkKWEntrySet::~vtkKWEntrySet()
{
  // Delete all entries

  vtkKWEntrySet::EntrySlot *entry_slot = NULL;
  vtkKWEntrySet::EntriesContainerIterator *it = 
    this->Entries->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(entry_slot) == VTK_OK)
      {
      if (entry_slot->Entry)
        {
        entry_slot->Entry->Delete();
        entry_slot->Entry = NULL;
        }
      delete entry_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->Entries->Delete();
}

//----------------------------------------------------------------------------
vtkKWEntrySet::EntrySlot* 
vtkKWEntrySet::GetEntrySlot(int id)
{
  vtkKWEntrySet::EntrySlot *entry_slot = NULL;
  vtkKWEntrySet::EntrySlot *found = NULL;
  vtkKWEntrySet::EntriesContainerIterator *it = 
    this->Entries->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(entry_slot) == VTK_OK && entry_slot->Id == id)
      {
      found = entry_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWEntry* vtkKWEntrySet::GetEntry(int id)
{
  vtkKWEntrySet::EntrySlot *entry_slot = 
    this->GetEntrySlot(id);

  if (!entry_slot)
    {
    return NULL;
    }

  return entry_slot->Entry;
}

//----------------------------------------------------------------------------
int vtkKWEntrySet::HasEntry(int id)
{
  return this->GetEntrySlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWEntrySet::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWEntrySet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWEntrySet::EntrySlot *entry_slot = NULL;
  vtkKWEntrySet::EntriesContainerIterator *it = 
    this->Entries->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(entry_slot) == VTK_OK)
      {
      entry_slot->Entry->SetEnabled(this->Enabled);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWEntrySet::AddEntry(int id, 
                            vtkKWObject *object, 
                            const char *method_and_arg_string,
                            const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The entry set must be created before any entry "
                  "is added.");
    return 0;
    }

  // Check if the new entry has a unique id

  if (this->HasEntry(id))
    {
    vtkErrorMacro("A entry with that id (" << id << ") already exists "
                  "in the entry set.");
    return 0;
    }

  // Add the entry slot to the manager

  vtkKWEntrySet::EntrySlot *entry_slot = 
    new vtkKWEntrySet::EntrySlot;

  if (this->Entries->AppendItem(entry_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a entry to the set.");
    delete entry_slot;
    return 0;
    }
  
  // Create the entry

  entry_slot->Entry = vtkKWEntry::New();
  entry_slot->Id = id;

  entry_slot->Entry->SetParent(this);
  entry_slot->Entry->Create(this->GetApplication(), 0);
  entry_slot->Entry->SetEnabled(this->Enabled);

  // Set balloon help, if any

  if (object && method_and_arg_string)
    {
    entry_slot->Entry->BindCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    entry_slot->Entry->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the entry

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWEntrySet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWEntrySet::EntrySlot *entry_slot = NULL;
  vtkKWEntrySet::EntriesContainerIterator *it = 
    this->Entries->NewIterator();

  int col = 0;
  int row = 0;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(entry_slot) == VTK_OK)
      {
      tk_cmd << "grid " << entry_slot->Entry->GetWidgetName() 
             << " -sticky news"
             << " -column " << (this->PackHorizontally ? col : row)
             << " -row " << (this->PackHorizontally ? row : col)
             << " -padx " << this->PadX
             << " -pady " << this->PadY
             << endl;
      col++;
      if (this->MaximumNumberOfWidgetInPackingDirection &&
          col >= this->MaximumNumberOfWidgetInPackingDirection)
        {
        col = 0;
        row++;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Weights
  
  int i;
  int maxcol = (row > 0) ? this->MaximumNumberOfWidgetInPackingDirection : col;
  for (i = 0; i < maxcol; i++)
    {
    tk_cmd << "grid " << (this->PackHorizontally ? "column" : "row") 
           << "configure " << this->GetWidgetName() << " " 
           << i << " -weight 1" << endl;
    }

  for (i = 0; i <= row; i++)
    {
    tk_cmd << "grid " << (this->PackHorizontally ? "row" : "column") 
           << "configure " << this->GetWidgetName() << " " 
           << i << " -weight 1" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWEntrySet::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWEntrySet::SetMaximumNumberOfWidgetInPackingDirection(int _arg)
{
  if (this->MaximumNumberOfWidgetInPackingDirection == _arg)
    {
    return;
    }
  this->MaximumNumberOfWidgetInPackingDirection = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWEntrySet::SetPadding(int x, int y)
{
  if (this->PadX == x && this->PadY == y)
    {
    return;
    }

  this->PadX = x;
  this->PadY = y;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWEntrySet::HideEntry(int id)
{
  this->SetEntryVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWEntrySet::ShowEntry(int id)
{
  this->SetEntryVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWEntrySet::SetEntryVisibility(int id, int flag)
{
  vtkKWEntrySet::EntrySlot *entry_slot = 
    this->GetEntrySlot(id);

  if (entry_slot && entry_slot->Entry)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 entry_slot->Entry->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWEntrySet::GetNumberOfVisibleEntries()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWEntrySet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "MaximumNumberOfWidgetInPackingDirection: " 
     << (this->MaximumNumberOfWidgetInPackingDirection ? "On" : "Off") << endl;
}


