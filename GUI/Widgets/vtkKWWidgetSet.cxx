/*=========================================================================

  Module:    vtkKWWidgetSet.cxx,v

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidgetSet.h"

#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/stl/list>
#include <vtksys/stl/vector>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWWidgetSet, "1.16");

//----------------------------------------------------------------------------
class vtkKWWidgetSetInternals
{
public:

  struct WidgetSlot
  {
    int Id;
    int Visibility;
    vtkKWWidget *Widget;
  };

  typedef vtksys_stl::vector<WidgetSlot> WidgetsContainer;
  typedef vtksys_stl::vector<WidgetSlot>::iterator WidgetsContainerIterator;

  WidgetsContainer Widgets;
};

//----------------------------------------------------------------------------
vtkKWWidgetSet::vtkKWWidgetSet()
{
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetsInPackingDirection = 0;
  this->WidgetsPadX = 0;
  this->WidgetsPadY = 0;
  this->WidgetsInternalPadX = 0;
  this->WidgetsInternalPadY = 0;
  this->ExpandWidgets = 0;
  this->UniformColumns = 0;
  this->UniformRows = 0;

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

  this->Pack();
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
int vtkKWWidgetSet::GetIdOfNthWidget(int rank)
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
int vtkKWWidgetSet::GetWidgetPosition(int id)
{
  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (it->Id == id)
      {
      return it - this->Internals->Widgets.begin();
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
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
vtkKWWidget* vtkKWWidgetSet::InsertWidgetInternal(int id, int pos)
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
  widget_slot.Visibility = 1;
  widget_slot.Widget = this->AllocateAndCreateWidget();
  this->PropagateEnableState(widget_slot.Widget);
 
  if (pos < 0 || (unsigned int)pos >= this->Internals->Widgets.size())
    {
    this->Internals->Widgets.push_back(widget_slot);
    }
  else
    {
    this->Internals->Widgets.insert(
      this->Internals->Widgets.begin() + (unsigned int)pos, widget_slot);
    }

  // Pack the set

  this->Pack();

  return widget_slot.Widget;
}

// ----------------------------------------------------------------------------
void vtkKWWidgetSet::Pack()
{
  if (!this->IsAlive())
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

  int nb_widgets = this->GetNumberOfWidgets();

  int old_nb_cols = 10, old_nb_rows = 10;
  vtkKWTkUtilities::GetGridSize(this, &old_nb_cols, &old_nb_rows);

  const char *sticky = 
    (this->ExpandWidgets ? "news" : (this->PackHorizontally ? "ews" : "nsw"));

  vtksys_stl::vector<int> col_used;
  col_used.assign(nb_widgets, 0);

  vtksys_stl::vector<int> row_used;
  row_used.assign(nb_widgets, 0);

  int col = 0;
  int row = 0;

  for (; it != end; ++it)
    {
    if (it->Visibility)
      {
      tk_cmd 
        << "grid " << it->Widget->GetWidgetName() 
        << " -sticky " << sticky
        << " -column " << (this->PackHorizontally ? col : row)
        << " -row " << (this->PackHorizontally ? row : col)
        << " -padx " << this->WidgetsPadX
        << " -pady " << this->WidgetsPadY
        << " -ipadx " << this->WidgetsInternalPadX
        << " -ipady " << this->WidgetsInternalPadY
        << endl;
      if (this->PackHorizontally)
        {
        col_used[col] = 1;
        row_used[row] = 1;
        }
      else
        {
        col_used[row] = 1;
        row_used[col] = 1;
        }
      }
    col++;
    if (this->MaximumNumberOfWidgetsInPackingDirection &&
        col >= this->MaximumNumberOfWidgetsInPackingDirection)
      {
      col = 0;
      row++;
      }
    }

  // Weights
  
  int max_col = 
    (row > 0) ? this->MaximumNumberOfWidgetsInPackingDirection : col;

  int nb_cols = this->PackHorizontally ? max_col : row;
  int nb_rows = this->PackHorizontally ? row + 1 : max_col;

  int i;
  if (nb_widgets)
    {
    for (i = 0; i < nb_cols; i++)
      {
      tk_cmd 
        << "grid columnconfigure " << this->GetWidgetName() << " " << i 
        << " -weight " << col_used[i]
        << " -uniform " << (this->UniformColumns && col_used[i] ? "col" : "{}")
        << endl;
      }
    }
  for (i = nb_cols; i < old_nb_cols; i++)
    {
    tk_cmd << "grid columnconfigure " << this->GetWidgetName() << " " << i
           << " -weight 0 -uniform {}" << endl;
    }
  
  if (nb_widgets)
    {
    for (i = 0; i < nb_rows; i++)
      {
      tk_cmd 
        << "grid rowconfigure " << this->GetWidgetName() << " " << i 
        << " -weight " << row_used[i]
        << " -uniform " << (this->UniformRows && row_used[i] ? "row" : "{}")
        << endl;
      }
    }
  for (i = nb_rows; i < old_nb_rows; i++)
    {
    tk_cmd << "grid rowconfigure " << this->GetWidgetName() << " " << i
           << " -weight 0 -uniform {}" << endl;
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
void vtkKWWidgetSet::SetUniformColumns(int _arg)
{
  if (this->UniformColumns == _arg)
    {
    return;
    }
  this->UniformColumns = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWWidgetSet::SetUniformRows(int _arg)
{
  if (this->UniformRows == _arg)
    {
    return;
    }
  this->UniformRows = _arg;
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

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetsPadX(int arg)
{
  if (arg == this->WidgetsPadX)
    {
    return;
    }

  this->WidgetsPadX = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetsPadY(int arg)
{
  if (arg == this->WidgetsPadY)
    {
    return;
    }

  this->WidgetsPadY = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetsInternalPadX(int arg)
{
  if (arg == this->WidgetsInternalPadX)
    {
    return;
    }

  this->WidgetsInternalPadX = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetsInternalPadY(int arg)
{
  if (arg == this->WidgetsInternalPadY)
    {
    return;
    }

  this->WidgetsInternalPadY = arg;
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
  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (it->Id == id)
      {
      return it->Visibility;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::SetWidgetVisibility(int id, int flag)
{
  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (it->Id == id)
      {
      if (it->Visibility != flag)
        {
        it->Visibility = flag;
        this->Pack();
        break;
        }
      }
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
int vtkKWWidgetSet::GetIdOfNthVisibleWidget(int rank)
{
  vtkKWWidgetSetInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWWidgetSetInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (it->Visibility && !rank--)
      {
      return it->Id;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkKWWidgetSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WidgetsPadX: " << this->WidgetsPadX << endl;
  os << indent << "WidgetsPadY: " << this->WidgetsPadY << endl;

  os << indent << "WidgetsInternalPadX: " << this->WidgetsInternalPadX << endl;
  os << indent << "WidgetsInternalPadY: " << this->WidgetsInternalPadY << endl;

  os << indent << "ExpandWidgets: " 
     << (this->ExpandWidgets ? "On" : "Off") << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "UniformColumns: " 
     << (this->UniformColumns ? "On" : "Off") << endl;

  os << indent << "UniformRows: " 
     << (this->UniformRows ? "On" : "Off") << endl;

  os << indent << "MaximumNumberOfWidgetsInPackingDirection: " 
     << (this->MaximumNumberOfWidgetsInPackingDirection ? "On" : "Off") << endl;
}
