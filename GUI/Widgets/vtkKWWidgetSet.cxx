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
#include "vtkObjectFactory.h"

#include <kwsys/stl/list>

//----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkKWWidgetSet, "1.5");

int vtkKWWidgetSetCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWWidgetSetInternals
{
public:

  struct WidgetSlot
  {
    int Id;
    vtkKWWidget *Widget;
  };

  typedef kwsys_stl::list<WidgetSlot> WidgetsContainer;
  typedef kwsys_stl::list<WidgetSlot>::iterator WidgetsContainerIterator;

  WidgetsContainer Widgets;
};

//----------------------------------------------------------------------------
vtkKWWidgetSet::vtkKWWidgetSet()
{
  this->CommandFunction = vtkKWWidgetSetCommand;
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetsInPackingDirection = 0;
  this->PadX = 0;
  this->PadY = 0;
  this->ExpandWidgets = 0;

  // Internal structs

  this->Internals = new vtkKWWidgetSetInternals;
}

//----------------------------------------------------------------------------
vtkKWWidgetSet::~vtkKWWidgetSet()
{
  // Delete all widgets

  this->DeleteAllWidgets();

  // Delete the container

  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::DeleteAllWidgets()
{
  // Delete all widgets

  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (it->Widget)
      {
      it->Widget->Delete();
      it->Widget = NULL;
      }
    }

  this->Internals->Widgets.clear();
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWWidgetSet::GetWidgetInternal(int id)
{
  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (it->Id == id)
      {
      return it->Widget;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::HasWidget(int id)
{
  return this->GetWidgetInternal(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::GetNumberOfWidgets()
{
  return this->Internals ? this->Internals->Widgets.size() : 0;
}

//----------------------------------------------------------------------------
int vtkKWWidgetSet::GetNthWidgetId(int rank)
{
  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (!rank--)
      {
      return it->Id;
      }
    }

  return -1;
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

  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    this->PropagateEnableState(it->Widget);
    }
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

  vtkKWWidgetSetInternals::WidgetSlot widget_slot;
  widget_slot.Id = id;
  widget_slot.Widget = this->AllocateAndCreateWidget();
  this->PropagateEnableState(widget_slot.Widget);

  this->Internals->Widgets.push_back(widget_slot);

  // Pack the set

  this->Pack();

  return widget_slot.Widget;
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

  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();

  int col = 0;
  int row = 0;
  const char *sticky = 
    (this->ExpandWidgets ? "news" : (this->PackHorizontally ? "ews" : "nsw"));

  for (; it != end; ++it)
    {
    tk_cmd 
      << "grid " << it->Widget->GetWidgetName() 
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

  // Weights
  
  int i;
  int maxcol = 
    (row > 0) ? this->MaximumNumberOfWidgetsInPackingDirection : col;
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
  vtkKWWidget *widget = this->GetWidgetInternal(id);
  return (widget && 
          widget->IsCreated() &&
          !widget->GetApplication()->EvaluateBooleanExpression(
            "catch {grid info %s}", widget->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetVisibility(int id, int flag)
{
  vtkKWWidget *widget = this->GetWidgetInternal(id);
  if (widget && widget->IsCreated())
    {
    this->Script("grid %s %s", (flag ? "" : "remove"),widget->GetWidgetName());
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
