/*=========================================================================

  Module:    vtkKWWidgetSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidgetSet.h"

#include "vtkKWApplication.h"
#include "vtkKWWidget.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkKWWidgetSet, "1.1");

int vtkKWWidgetSetCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWWidgetSet::vtkKWWidgetSet()
{
  this->CommandFunction = vtkKWWidgetSetCommand;
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetsInPackingDirection = 0;
  this->PadX = 0;
  this->PadY = 0;
  this->ExpandWidgets = 0;
  this->Widgets = vtkKWWidgetSet::WidgetsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWWidgetSet::~vtkKWWidgetSet()
{
  // Delete all widgets

  this->DeleteAllWidgets();

  // Delete the container

  this->Widgets->Delete();
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::DeleteAllWidgets()
{
  // Delete all widgets

  vtkKWWidgetSet::WidgetSlot *widget_slot = NULL;
  vtkKWWidgetSet::WidgetsContainerIterator *it = 
    this->Widgets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(widget_slot) == VTK_OK)
      {
      if (widget_slot->Widget)
        {
        widget_slot->Widget->Delete();
        widget_slot->Widget = NULL;
        }
      delete widget_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->Widgets->RemoveAllItems();
}

//----------------------------------------------------------------------------
vtkKWWidgetSet::WidgetSlot* 
vtkKWWidgetSet::GetWidgetSlot(int id)
{
  vtkKWWidgetSet::WidgetSlot *widget_slot = NULL;
  vtkKWWidgetSet::WidgetSlot *found = NULL;
  vtkKWWidgetSet::WidgetsContainerIterator *it = 
    this->Widgets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(widget_slot) == VTK_OK && widget_slot->Id == id)
      {
      found = widget_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWWidgetSet::WidgetSlot* 
vtkKWWidgetSet::GetNthWidgetSlot(int rank)
{
  vtkKWWidgetSet::WidgetSlot *widget_slot = NULL;
  vtkKWWidgetSet::WidgetSlot *found = NULL;
  vtkKWWidgetSet::WidgetsContainerIterator *it = 
    this->Widgets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(widget_slot) == VTK_OK && !rank--)
      {
      found = widget_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::HasWidget(int id)
{
  return this->GetWidgetSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::GetNumberOfWidgets()
{
  return this->Widgets ? this->Widgets->GetNumberOfItems() : 0;
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::GetNthWidgetId(int rank)
{
  vtkKWWidgetSet::WidgetSlot *widget_slot = 
    this->GetNthWidgetSlot(rank);

  if (!widget_slot)
    {
    return -1;
    }

  return widget_slot->Id;
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::Create(vtkKWApplication *app, const char *args)
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
void vtkKWWidgetSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWWidgetSet::WidgetSlot *widget_slot = NULL;
  vtkKWWidgetSet::WidgetsContainerIterator *it = 
    this->Widgets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(widget_slot) == VTK_OK)
      {
      widget_slot->Widget->SetEnabled(this->Enabled);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWWidgetSet::AddWidgetInternal(int id)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The vtkKWWidgetSet set must be created before any "
                  "Widget can be added.");
    return NULL;
    }

  // Check if the new widget has a unique id

  if (this->HasWidget(id))
    {
    vtkErrorMacro("A Widget with that id (" << id << ") already exists "
                  "in the set.");
    return NULL;
    }

  // Add the widget slot to the manager

  vtkKWWidgetSet::WidgetSlot *widget_slot = 
    new vtkKWWidgetSet::WidgetSlot;

  if (this->Widgets->AppendItem(widget_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a Widget to the set.");
    delete widget_slot;
    return NULL;
    }
  
  // Create the widget

  widget_slot->Id = id;
  widget_slot->Widget = this->AllocateAndCreateWidget();
  widget_slot->Widget->SetEnabled(this->Enabled);

  // Pack the set

  this->Pack();

  return widget_slot->Widget;
}

// ----------------------------------------------------------------------------
void vtkKWWidgetSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWWidgetSet::WidgetSlot *widget_slot = NULL;
  vtkKWWidgetSet::WidgetsContainerIterator *it = 
    this->Widgets->NewIterator();

  int col = 0;
  int row = 0;
  const char *sticky = 
    (this->ExpandWidgets ? "news" : (this->PackHorizontally ? "ews" : "nsw"));

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(widget_slot) == VTK_OK)
      {
      tk_cmd 
        << "grid " << widget_slot->Widget->GetWidgetName() 
        << " -sticky " << sticky
        << " -column " << (this->PackHorizontally ? col : row)
        << " -row " << (this->PackHorizontally ? row : col)
        << " -padx " << this->PadX
        << " -pady " << this->PadY
        << endl;
      col++;
      if (this->MaximumNumberOfWidgetsInPackingDirection &&
          col >= this->MaximumNumberOfWidgetsInPackingDirection)
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
  int maxcol = (row > 0) ? this->MaximumNumberOfWidgetsInPackingDirection : col;
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
void vtkKWWidgetSet::SetPackHorizontally(int _arg)
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
void vtkKWWidgetSet::SetMaximumNumberOfWidgetsInPackingDirection(int _arg)
{
  if (this->MaximumNumberOfWidgetsInPackingDirection == _arg)
    {
    return;
    }
  this->MaximumNumberOfWidgetsInPackingDirection = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWWidgetSet::SetPadding(int x, int y)
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

// ----------------------------------------------------------------------------
void vtkKWWidgetSet::SetExpandWidgets(int _arg)
{
  if (this->ExpandWidgets == _arg)
    {
    return;
    }

  this->ExpandWidgets = _arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::HideWidget(int id)
{
  this->SetWidgetVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::ShowWidget(int id)
{
  this->SetWidgetVisibility(id, 1);
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::GetWidgetVisibility(int id)
{
  vtkKWWidgetSet::WidgetSlot *widget_slot = 
    this->GetWidgetSlot(id);

  return (widget_slot && 
          widget_slot->Widget &&
          widget_slot->Widget->IsCreated() &&
          !widget_slot->Widget->GetApplication()->EvaluateBooleanExpression(
            "catch {grid info %s}", widget_slot->Widget->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetVisibility(int id, int flag)
{
  vtkKWWidgetSet::WidgetSlot *widget_slot = 
    this->GetWidgetSlot(id);

  if (widget_slot && widget_slot->Widget && widget_slot->Widget->IsCreated())
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 widget_slot->Widget->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::GetNumberOfVisibleWidgets()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ExpandWidgets: " 
     << (this->ExpandWidgets ? "On" : "Off") << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "MaximumNumberOfWidgetsInPackingDirection: " 
     << (this->MaximumNumberOfWidgetsInPackingDirection ? "On" : "Off") << endl;
}
