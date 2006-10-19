/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

// we need all of windows.h
#define VTK_WINDOWS_FULL

#include "vtkKWSelectionFrameLayoutManager.h"

#include "vtkKWOptions.h"
#include "vtkImageData.h"
#include "vtkKWApplication.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWSaveImageDialog.h"
#include "vtkKWSelectionFrame.h"
#include "vtkKWSimpleEntryDialog.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWWindowBase.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWindows.h"

// Readers / Writers

#include "vtkErrorCode.h"
#include "vtkBMPWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkTIFFWriter.h"

#include "vtkWindowToImageFilter.h"
#include "vtkImageAppend.h"
#include "vtkImageConstantPad.h"

#include "vtkStringArray.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/list>
#include <vtksys/stl/string>

#include "Resources/vtkKWWindowLayoutResources.h"

#define VTK_KW_SFLMGR_LABEL_PATTERN "%d x %d"
#define VTK_KW_SFLMGR_ICON_PATTERN "KWWindowLayout%dx%d"
#define VTK_KW_SFLMGR_RESOLUTIONS {{ 1, 1}, { 1, 2}, { 2, 1}, { 2, 2}, { 2, 3}, { 3, 2}}
#define VTK_KW_SFLMGR_MAX_SIZE 100

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWSelectionFrameLayoutManager);
vtkCxxRevisionMacro(vtkKWSelectionFrameLayoutManager, "1.66");

//----------------------------------------------------------------------------
class vtkKWSelectionFrameLayoutManagerInternals
{
public:
  struct PoolNode
  {
    vtksys_stl::string Tag;
    vtksys_stl::string Group;
    vtkKWSelectionFrame *Widget;
    int Position[2];
  };

  typedef vtksys_stl::vector<PoolNode> PoolType;
  typedef vtksys_stl::vector<PoolNode>::iterator PoolIterator;

  PoolType Pool;

  struct CellCoordinate
  {
    int Col;
    int Row;
  };
  typedef vtksys_stl::list<CellCoordinate> CellCoordinatePoolType;

  CellCoordinatePoolType ResolutionStack;
  CellCoordinatePoolType PositionStack;

  vtksys_stl::string ScheduleNumberOfWidgetsHasChangedTimerId;
};

//----------------------------------------------------------------------------
vtkKWSelectionFrameLayoutManager::vtkKWSelectionFrameLayoutManager()
{
  this->Internals = new vtkKWSelectionFrameLayoutManagerInternals;

  this->Resolution[0] = 1;
  this->Resolution[1] = 1;

  this->Origin[0] = 0;
  this->Origin[1] = 0;

  this->ResolutionEntriesMenu    = NULL;
  this->ResolutionEntriesToolbar = NULL;

  this->SelectionChangedCommand = NULL;

  this->ReorganizeWidgetPositionsAutomatically = 1;

  this->LayoutFrame = vtkKWFrame::New();
}

//----------------------------------------------------------------------------
vtkKWSelectionFrameLayoutManager::~vtkKWSelectionFrameLayoutManager()
{
  if (this->SelectionChangedCommand)
    {
    delete [] this->SelectionChangedCommand;
    this->SelectionChangedCommand = NULL;
    }

  // Remove all widgets

  this->RemoveAllWidgets();

  // Delete frame

  if (this->LayoutFrame)
    {
    this->LayoutFrame->Delete();
    this->LayoutFrame = NULL;
    }

  // Delete our pool

  delete this->Internals;

  // Delete the menu

  if (this->ResolutionEntriesMenu)
    {
    this->ResolutionEntriesMenu->Delete();
    this->ResolutionEntriesMenu = NULL;
    }

  // Delete the toolbar

  if (this->ResolutionEntriesToolbar)
    {
    this->ResolutionEntriesToolbar->Delete();
    this->ResolutionEntriesToolbar = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SetBackgroundColor(0.2, 0.2, 0.2);

  // Create the layout frame

  this->LayoutFrame->SetParent(this);
  this->LayoutFrame->Create();
  this->LayoutFrame->SetBackgroundColor(0.2, 0.2, 0.2);

  // Pack

  this->Pack();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::IsPositionInLayout(int col, int row)
{
  return (col >= this->Origin[0] && 
          col < this->Origin[0] + this->Resolution[0] && 
          row >= this->Origin[1] && 
          row < this->Origin[1] + this->Resolution[1]);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::Pack()
{
  if (!this->IsAlive())
    {
    return;
    }

  // Unpack everything

  this->UnpackChildren();

  ostrstream tk_cmd;

  // Pack layout

  tk_cmd << "pack " << this->LayoutFrame->GetWidgetName() 
         << " -side top -expand y -fill both -padx 0 -pady 0" << endl;

  this->LayoutFrame->UnpackChildren();
  
  // Pack each widgets, column first

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget)
      {
      this->CreateWidget(it->Widget);
      if (it->Widget->IsCreated() && this->IsPositionInLayout(it->Position))
        {
        tk_cmd << "grid " << it->Widget->GetWidgetName() 
               << " -sticky news "
               << " -column " << it->Position[0] - this->Origin[0]
               << " -row " << it->Position[1] - this->Origin[1] << endl;
        }
      }
    }

  // columns and rows can resize
  // Make sure we reset the columns/rows that are not used (even if we
  // unpacked the children, those settings are kept since they are set
  // on the master)

  int nb_of_cols = 10, nb_of_rows = 10;
  vtkKWTkUtilities::GetGridSize(this->LayoutFrame, &nb_of_cols, &nb_of_rows);

  int i, j;
  for (j = 0; j < this->Resolution[1]; j++)
    {
    tk_cmd << "grid rowconfigure " 
           << this->LayoutFrame->GetWidgetName() << " " << j 
           << " -weight 1 -uniform row" << endl;
    }
  for (j = this->Resolution[1]; j < nb_of_rows; j++)
    {
    tk_cmd << "grid rowconfigure " 
           << this->LayoutFrame->GetWidgetName() << " " << j 
           << " -weight 0 -uniform {}" << endl;
    }
  for (i = 0; i < this->Resolution[0]; i++)
    {
    tk_cmd << "grid columnconfigure " 
           << this->LayoutFrame->GetWidgetName() << " " << i
           << " -weight 1 -uniform col" << endl;
    }
  for (i = this->Resolution[0]; i < nb_of_cols; i++)
    {
    tk_cmd << "grid columnconfigure " 
           << this->LayoutFrame->GetWidgetName() << " " << i
           << " -weight 0 -uniform {}" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SetWidgetPositionInternal(
  vtkKWSelectionFrame *widget, int col, int row)
{
  if (widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && it->Widget == widget)
        {
        it->Position[0] = col;
        it->Position[1] = row;
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SetWidgetPosition(
  vtkKWSelectionFrame *widget, int col, int row)
{
  if (this->SetWidgetPositionInternal(widget, col, row))
    {
    this->Pack();
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetWidgetPosition(
  vtkKWSelectionFrame *widget, int *col, int *row)
{
  if (widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && it->Widget == widget)
        {
        *col = it->Position[0];
        *row = it->Position[1];
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* 
vtkKWSelectionFrameLayoutManager::GetWidgetAtPosition(int col, int row)
{
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && 
        it->Position[0] == col && it->Position[1] == row)
      {
      return it->Widget;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void 
vtkKWSelectionFrameLayoutManager::SetReorganizeWidgetPositionsAutomatically(
  int arg)
{
  if (this->ReorganizeWidgetPositionsAutomatically == arg)
    {
    return;
    }

  this->ReorganizeWidgetPositionsAutomatically = arg;
  this->Modified();

  if (this->ReorganizeWidgetPositionsAutomatically)
    {
    this->ReorganizeWidgetPositions();
    this->Pack();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::ReorganizeWidgetPositions()
{
  // Save selection, that could be affected by reorg

  int sel_pos[2];
  vtkKWSelectionFrame *sel = this->GetSelectedWidget();
  this->GetWidgetPosition(sel, sel_pos);

  // Given the resolution, fill in the corresponding grid with 
  // widgets that have a valid position inside that grid

  vtksys_stl::vector<int> grid;
  grid.assign(this->Resolution[0] * this->Resolution[1], 0);

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && this->IsPositionInLayout(it->Position))
      {
      grid[(it->Position[1] - this->Origin[1]) * this->Resolution[0] + 
           it->Position[0] - this->Origin[0]] = 1;
      }
    }

  // Fill the holes in the grid with whatever widgets
  // which positions were out of the grid

  it = this->Internals->Pool.begin();
  int i, j;
  for (j = 0; j < this->Resolution[1] && it != end; j++)
    {
    for (i = 0; i < this->Resolution[0] && it != end; i++)
      {
      if (grid[j * this->Resolution[0] + i] == 0)
        {
        while (it != end)
          {
          if (it->Widget &&  !this->IsPositionInLayout(it->Position))
            {
            it->Position[0] = this->Origin[0] + i;
            it->Position[1] = this->Origin[1] + j;
            ++it;
            break;
            }
          ++it;
          }
        }
      }
    }

  // Fix the selection, just in case it is not visible anymore for example

  if (sel)
    {
    this->MoveSelectionInsideVisibleLayout(sel_pos);
    }
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PushResolution(int nb_cols, int nb_rows)
{
  if (this->Internals)
    {
    vtkKWSelectionFrameLayoutManagerInternals::CellCoordinate res;
    res.Col = nb_cols;
    res.Row = nb_rows;
    this->Internals->ResolutionStack.push_back(res);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PopResolution(int *nb_cols, int *nb_rows)
{
  if (this->Internals && this->Internals->ResolutionStack.size())
    {
    vtkKWSelectionFrameLayoutManagerInternals::CellCoordinate &res =
      this->Internals->ResolutionStack.back();
    *nb_cols = res.Col;
    *nb_rows = res.Row;
    this->Internals->ResolutionStack.pop_back();
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SetResolution(int nb_cols, int nb_rows)
{
  this->SetResolutionAndOrigin(
    nb_cols, nb_rows, this->Origin[0], this->Origin[1]);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PushPosition(int col, int row)
{
  if (this->Internals)
    {
    vtkKWSelectionFrameLayoutManagerInternals::CellCoordinate res;
    res.Col = col;
    res.Row = row;
    this->Internals->PositionStack.push_back(res);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PopPosition(int *col, int *row)
{
  if (this->Internals && this->Internals->PositionStack.size())
    {
    vtkKWSelectionFrameLayoutManagerInternals::CellCoordinate &res =
      this->Internals->PositionStack.back();
    *col = res.Col;
    *row = res.Row;
    this->Internals->PositionStack.pop_back();
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SetOrigin(int col, int row)
{
  this->SetResolutionAndOrigin(
    this->Resolution[0], this->Resolution[1], col, row);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SetResolutionAndOrigin(
  int nb_cols, int nb_rows, int col, int row)
{
  if (nb_cols < 0 || nb_rows < 0 || col < 0 || row < 0)
    {
    return;
    }

  int res_has_changed = 
    (nb_cols != this->Resolution[0] || nb_rows != this->Resolution[1]);

  int origin_has_changed = 
    (col != this->Origin[0] || row != this->Origin[1]);

  if (!res_has_changed && !origin_has_changed)
    {
    return;
    }

  this->Resolution[0] = nb_cols;
  this->Resolution[1] = nb_rows;

  this->Origin[0] = col;
  this->Origin[1] = row;

  if (res_has_changed)
    {
    this->UpdateResolutionEntriesMenu();
    this->UpdateResolutionEntriesToolbar();
    }

  // Reorganize and pack

  if (this->ReorganizeWidgetPositionsAutomatically)
    {
    this->ReorganizeWidgetPositions();
    this->Pack();
    }

  if (res_has_changed)
    {
    this->InvokeEvent(
      vtkKWSelectionFrameLayoutManager::ResolutionChangedEvent, 
      NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::MoveSelectionInsideVisibleLayout(
  int *pos_hint)
{
  // If the selection disappeared, try to select
  // something else. Let's try the lower right corners first.
  
  vtkKWSelectionFrame *sel = this->GetSelectedWidget();
  if (sel && !this->GetWidgetVisibility(sel))
    {
    vtkKWSelectionFrame *try_selection = NULL;
    if (pos_hint)
      {
      try_selection = this->GetWidgetAtPosition(pos_hint);
      }
    for (int row = this->Origin[1] + this->Resolution[1] - 1; 
         row >= this->Origin[1] && !try_selection; row--)
      {
      for (int col = this->Origin[0] + this->Resolution[0] - 1; 
           col >= this->Origin[0] && !try_selection; col--)
        {
        try_selection = this->GetWidgetAtPosition(col, row);
        }
      }
    this->SelectWidget(try_selection);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::AdjustResolution()
{
  int i = this->Resolution[0];
  int j = this->Resolution[1];

  int pool_size = (int)this->Internals->Pool.size();

  // Increase the resolution so that all widgets can potentially be shown
  // If there is the same number of row/column, add a row first

  while (pool_size && 
         (i * j) < pool_size)
    {
    if (i < j)
      {
      i++;
      }
    else
      {
      j++;
      }
    }

  // Decrease the resolution so that all widgets can potentially be shown
  // without extra columns or holes.
  // If there is the same number of row/column, remove a row first

  while (pool_size &&
         (pool_size <= ((i - 1) * j)  ||
          pool_size <= (i * (j - 1))))
    {
    if (i > j)
      {
      i--;
      }
    else
      {
      j--;
      }
    }

  this->SetResolution(i, j);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateResolutionEntriesMenu(
  vtkKWMenu *parent)
{
  if (!parent)
    {
    return;
    }

  if (!this->ResolutionEntriesMenu)
    {
    this->ResolutionEntriesMenu = vtkKWMenu::New();
    }

  if (!this->ResolutionEntriesMenu->IsCreated())
    {
    this->ResolutionEntriesMenu->SetParent(parent);
    this->ResolutionEntriesMenu->Create();
    }

  // Allowed resolutions

  vtksys_stl::string varname(this->GetTclName());
  varname += "reschoice";

  char label[64], command[128], help[128];  

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (size_t idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(label, VTK_KW_SFLMGR_LABEL_PATTERN, 
            res[idx][0], res[idx][1]);
    sprintf(command, "ResolutionCallback %d %d", res[idx][0], res[idx][1]);
    sprintf(
      help, 
      ks_("Selection Frame Manager|Set window layout to %d column(s) by %d row(s)"), 
      res[idx][0], res[idx][1]);
    int value = 
      ((res[idx][0] - 1) * VTK_KW_SFLMGR_MAX_SIZE + res[idx][1] - 1);
    int index = this->ResolutionEntriesMenu->AddRadioButton(
      label, this, command);
    this->ResolutionEntriesMenu->SetItemVariable(
      index, varname.c_str());
    this->ResolutionEntriesMenu->SetItemSelectedValueAsInt(index, value);
    this->ResolutionEntriesMenu->SetItemHelpString(index, help);
    }

  this->UpdateResolutionEntriesMenu();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateResolutionEntriesMenu()
{
  if (!this->ResolutionEntriesMenu ||
      !this->ResolutionEntriesMenu->IsCreated())
    {
    return;
    }

  // Enabled/Disabled some resolutions

  int normal_state = 
    this->GetEnabled() ? 
    vtkKWOptions::StateNormal : vtkKWOptions::StateDisabled;
  size_t size = this->Internals->Pool.size();

  char label[64];

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (size_t idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(label, VTK_KW_SFLMGR_LABEL_PATTERN, res[idx][0], res[idx][1]);
    this->ResolutionEntriesMenu->SetItemState(
      label, 
      (size_t)(res[idx][0] * res[idx][1]) <= 
      (size + (res[idx][0] != 1 && res[idx][1] != 1 ? 1 : 0))
      ? normal_state : vtkKWOptions::StateDisabled);
    }

  // Select the right one

  int value = 
    (this->Resolution[0]-1) * VTK_KW_SFLMGR_MAX_SIZE + this->Resolution[1]-1;

  vtksys_stl::string rbv(this->GetTclName());
  rbv += "reschoice";
  if (atoi(this->Script("set %s", rbv.c_str())) != value)
    {
    this->Script("set %s %d", rbv.c_str(), value);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateResolutionEntriesToolbar(
  vtkKWWidget *parent)
{
  if (!parent)
    {
    return;
    }

  if (!this->ResolutionEntriesToolbar)
    {
    this->ResolutionEntriesToolbar = vtkKWToolbar::New();
    this->ResolutionEntriesToolbar->SetName(
      ks_("Toolbar|Window Layout"));
    }

  if (!this->ResolutionEntriesToolbar->IsCreated())
    {
    this->ResolutionEntriesToolbar->SetParent(parent);
    this->ResolutionEntriesToolbar->Create();
    }

  // Got to create the icons

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication(),
    "KWWindowLayout1x1",
    NULL,
    NULL,
    image_KWWindowLayout1x1, 
    image_KWWindowLayout1x1_width, 
    image_KWWindowLayout1x1_height,
    image_KWWindowLayout1x1_pixel_size,
    image_KWWindowLayout1x1_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication(),
    "KWWindowLayout1x2",
    NULL,
    NULL,
    image_KWWindowLayout1x2, 
    image_KWWindowLayout1x2_width, 
    image_KWWindowLayout1x2_height,
    image_KWWindowLayout1x2_pixel_size,
    image_KWWindowLayout1x2_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication(),
    "KWWindowLayout2x1",
    NULL,
    NULL,
    image_KWWindowLayout2x1, 
    image_KWWindowLayout2x1_width, 
    image_KWWindowLayout2x1_height,
    image_KWWindowLayout2x1_pixel_size,
    image_KWWindowLayout2x1_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication(),
    "KWWindowLayout2x2",
    NULL,
    NULL,
    image_KWWindowLayout2x2, 
    image_KWWindowLayout2x2_width, 
    image_KWWindowLayout2x2_height,
    image_KWWindowLayout2x2_pixel_size,
    image_KWWindowLayout2x2_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication(),
    "KWWindowLayout2x3",
    NULL,
    NULL,
    image_KWWindowLayout2x3, 
    image_KWWindowLayout2x3_width, 
    image_KWWindowLayout2x3_height,
    image_KWWindowLayout2x3_pixel_size,
    image_KWWindowLayout2x3_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication(),
    "KWWindowLayout3x2",
    NULL,
    NULL,
    image_KWWindowLayout3x2, 
    image_KWWindowLayout3x2_width, 
    image_KWWindowLayout3x2_height,
    image_KWWindowLayout3x2_pixel_size,
    image_KWWindowLayout3x2_length);

  // Allowed resolutions

  vtksys_stl::string rbv(this->GetTclName());
  rbv += "reschoice";

  char command[128], help[128], icon[128];  

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (size_t idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(command, "ResolutionCallback %d %d", 
            res[idx][0], res[idx][1]);
    sprintf(
      help, 
      ks_("Selection Frame Manager|Set window layout to %d column(s) by %d row(s)"), 
      res[idx][0], res[idx][1]);
    sprintf(icon, VTK_KW_SFLMGR_ICON_PATTERN, 
            res[idx][0], res[idx][1]);
    int value = 
      ((res[idx][0] - 1) * VTK_KW_SFLMGR_MAX_SIZE + res[idx][1] - 1);
    this->ResolutionEntriesToolbar->AddRadioButtonImage(
      value, icon, icon, rbv.c_str(), this, command, help);
    }

  this->UpdateResolutionEntriesToolbar();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateResolutionEntriesToolbar()
{
  if (!this->ResolutionEntriesToolbar ||
      !this->ResolutionEntriesToolbar->IsCreated())
    {
    return;
    }

  // Enabled/Disabled some resolutions

  size_t size = this->Internals->Pool.size();
  char icon[128];

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (size_t idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(icon, VTK_KW_SFLMGR_ICON_PATTERN, res[idx][0], res[idx][1]);
    vtkKWWidget *w = this->ResolutionEntriesToolbar->GetWidget(icon);
    if (w)
      {
      w->SetEnabled(
        (size_t)(res[idx][0] * res[idx][1]) <= 
        (size + (res[idx][0] != 1 && res[idx][1] != 1 ? 1 : 0)) 
        ? this->GetEnabled() : 0);
      }
    }

  // Select the right one

  int value = 
    (this->Resolution[0]-1) * VTK_KW_SFLMGR_MAX_SIZE + this->Resolution[1]-1;

  vtksys_stl::string rbv(this->GetTclName());
  rbv += "reschoice";
  if (atoi(this->Script("set %s", rbv.c_str())) != value)
    {
    this->Script("set %s %d", rbv.c_str(), value);
    }
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetWidgetWithTag(
  const char *tag)
{
  if (tag && *tag)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && !it->Tag.compare(tag))
        {
        return it->Widget;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* 
vtkKWSelectionFrameLayoutManager::GetWidgetWithTagAndGroup(
  const char *tag, const char *group)
{
  if (tag && *tag && group && *group)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && !it->Tag.compare(tag)  && !it->Group.compare(group))
        {
        return it->Widget;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetNthWidget(
  int index)
{
  if (index < 0 || index >= (int)this->Internals->Pool.size())
    {
    return NULL;
    }

#if 0  
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  while (index != 0)
    {
    ++it;
    index--;
    }
  return it->Widget;
#else
  return this->Internals->Pool[index].Widget;
#endif
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetNthWidgetNotMatching(
  int index, vtkKWSelectionFrame *avoid)
{
  if (index >= 0)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && it->Widget != avoid)
        {
        index--;
        if (index < 0)
          {
          return it->Widget;
          }
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetNthWidgetWithGroup(
  int index, const char *group)
{
  if (index >= 0 && group && *group)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && !it->Group.compare(group))
        {
        index--;
        if (index < 0)
          {
          return it->Widget;
          }
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetWidgetWithTitle(
  const char *title)
{
  if (title)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && 
          it->Widget->GetTitle() && 
          !strcmp(title, it->Widget->GetTitle()))
        {
        return it->Widget;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetNumberOfWidgets()
{
  return this->Internals->Pool.size();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetNumberOfWidgetsWithTag(
  const char *tag)
{
  int count = 0;
  if (tag && *tag)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && !it->Tag.compare(tag))
        {
        count++;
        }
      }
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetNumberOfWidgetsWithGroup(
  const char *group)
{
  int count = 0;
  if (group && *group)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && !it->Group.compare(group))
        {
        count++;
        }
      }
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::HasWidget(
  vtkKWSelectionFrame *widget)
{
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget == widget)
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::HasWidgetWithTag(const char *tag)
{
  return this->GetWidgetWithTag(tag) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::HasWidgetWithTagAndGroup(
  const char *tag, const char *group)
{
  return this->GetWidgetWithTagAndGroup(tag, group) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AddWidget(
  vtkKWSelectionFrame *widget)
{
  return this->AddWidgetWithTagAndGroup(widget, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AddWidgetWithTagAndGroup(
  vtkKWSelectionFrame *widget, const char *tag, const char *group)
{
  if (!widget)
    {
    return 0;
    }

  // If we have that widget already

  if (this->HasWidget(widget))
    {
    return 0;
    }

  // Create a new node

  vtkKWSelectionFrameLayoutManagerInternals::PoolNode node;
  node.Widget = widget;
  if (tag)
    {
    node.Tag = tag;
    }
  if (group)
    {
    node.Group = group;
    }
  node.Widget->Register(this);

  // Create the widget (if needed), configure the callbacks

  if (!node.Widget->IsCreated())
    {
    this->CreateWidget(node.Widget);
    }
  else
    {
    this->ConfigureWidget(node.Widget);
    }

  // Unitialize its position. It will be updated automatically the first
  // time this widget is packed.

  node.Position[0] = node.Position[1] = -1;

  // Add it to the pool

  this->Internals->Pool.push_back(node);

  this->ScheduleNumberOfWidgetsHasChanged();

  // If we just added a widget, and there was nothing else before, let's
  // select it for convenience purposes

  if (this->GetNumberOfWidgets() == 1 && !this->GetSelectedWidget())
    {
    this->SelectWidget(this->GetNthWidget(0));
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::AllocateAndAddWidget()
{
  // Allocate a widget and add it

  vtkKWSelectionFrame *widget = this->AllocateWidget();
  if (widget)
    {
    int ok =  this->AddWidget(widget); // this will Register() the widget
    widget->Delete();
    if (!ok)
      {
      widget = NULL;
      }
    }

  return widget;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::AllocateWidget()
{
  vtkKWSelectionFrame *widget = vtkKWSelectionFrame::New();
  widget->AllowChangeTitleOn();
  widget->AllowCloseOn();
  return widget;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateWidget(
  vtkKWSelectionFrame *widget)
{
  if (this->IsCreated() && widget && !widget->IsCreated())
    {
    widget->SetParent(this->LayoutFrame);
    widget->Create();
    widget->SetWidth(350);
    widget->SetHeight(350);
    this->ConfigureWidget(widget);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::ConfigureWidget(
  vtkKWSelectionFrame *widget)
{
  this->PropagateEnableState(widget);
  this->AddCallbacksToWidget(widget);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::AddCallbacksToWidget(
  vtkKWSelectionFrame *widget)
{
  if (widget)
    {
    widget->SetCloseCommand(this, "CloseWidgetCallback");
    widget->SetTitleChangedCommand(this, "WidgetTitleChangedCallback");
    widget->SetChangeTitleCommand(this, "ChangeWidgetTitleCallback");
    widget->SetSelectCommand(this, "SelectWidgetCallback");
    widget->SetDoubleClickCommand(this, "SelectAndMaximizeWidgetCallback");
    widget->SetSelectionListCommand(this, "SwitchWidgetCallback");
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::RemoveCallbacksFromWidget(
  vtkKWSelectionFrame *widget)
{
  if (widget)
    {
    widget->SetCloseCommand(NULL, NULL);
    widget->SetChangeTitleCommand(NULL, NULL);
    widget->SetSelectCommand(NULL, NULL);
    widget->SetDoubleClickCommand(NULL, NULL);
    widget->SetSelectionListCommand(NULL, NULL);
    }
}

//----------------------------------------------------------------------------
vtkKWRenderWidget* vtkKWSelectionFrameLayoutManager::GetRenderWidget(
  vtkKWSelectionFrame *widget)
{
  vtkKWRenderWidget *rw = NULL;
  if (widget)
    {
    vtkKWFrame *frame = widget->GetBodyFrame();
    if (frame)
      {
      int nb_children = frame->GetNumberOfChildren();
      for (int i = 0; i < nb_children; i++)
        {
        vtkKWWidget *child = frame->GetNthChild(i);
        if (child)
          {
          rw = vtkKWRenderWidget::SafeDownCast(child);
          if (rw)
            {
            return rw;
            }
          int nb_grand_children = child->GetNumberOfChildren();
          for (int j = 0; j < nb_grand_children; j++)
            {
            vtkKWWidget *grand_child = child->GetNthChild(j);
            if (grand_child)
              {
              rw = vtkKWRenderWidget::SafeDownCast(grand_child);
              if (rw)
                {
                return rw;
                }
              }
            }
          }
        }
      }
    }
  return rw;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::NumberOfWidgetsHasChanged()
{
  // Update all selection lists, so that this new widget can be selected

  this->UpdateSelectionLists();

  // Adjust the resolution

  this->AdjustResolution();
  this->UpdateEnableState();

  // Pack

  if (this->ReorganizeWidgetPositionsAutomatically)
    {
    this->ReorganizeWidgetPositions();
    this->Pack();
    }
}

//----------------------------------------------------------------------------
void 
vtkKWSelectionFrameLayoutManager::ScheduleNumberOfWidgetsHasChanged()
{
  // Already scheduled

  if (!this->GetApplication() ||
      this->Internals->ScheduleNumberOfWidgetsHasChangedTimerId.size())
    {
    return;
    }

  this->Internals->ScheduleNumberOfWidgetsHasChangedTimerId =
    this->Script("after idle {catch {%s NumberOfWidgetsHasChangedCallback}}", 
                 this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::NumberOfWidgetsHasChangedCallback()
{
  if (!this->GetApplication() || this->GetApplication()->GetInExit() ||
      !this->IsAlive())
    {
    return;
    }

  this->NumberOfWidgetsHasChanged();

  this->Internals->ScheduleNumberOfWidgetsHasChangedTimerId = "";
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::DeleteWidget(
  vtkKWSelectionFrame *widget)
{
  if (widget)
    {
    this->RemoveCallbacksFromWidget(widget);
    widget->Close();
    widget->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::RemoveWidget(
  vtkKWSelectionFrame *widget)
{
  if (this->Internals && widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget == widget)
        {
        // If we are removing the selectiong, make sure we select another one
        // instead
        vtkKWSelectionFrame *sel = this->GetSelectedWidget();
        this->Internals->Pool.erase(it);
        if (sel == widget)
          {
          this->SelectWidget(this->GetNthWidget(0));
          }
        this->DeleteWidget(widget);
        this->ScheduleNumberOfWidgetsHasChanged();
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::RemoveAllWidgets()
{
  // Is faster than calling RemoveWidget on each widget
  // since the selection is set to NULL first, and no callbacks is going
  // to be invoked until every widget is cleared.

  if (this->Internals)
    {
    this->SelectWidget((vtkKWSelectionFrame*)NULL);

    int nb_deleted = 0;
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget)
        {
        this->DeleteWidget(it->Widget);
        nb_deleted++;
        }
      }
    
    this->Internals->Pool.clear();
    if (nb_deleted)
      {
      this->ScheduleNumberOfWidgetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::RemoveAllWidgetsWithGroup(
  const char *group)
{
  // Is faster than calling RemoveWidget on each widget
  // since the selection is saved first, and no callbacks is going
  // to be invoked until every widget is cleared.

  if (this->Internals && group && *group)
    {
    vtkKWSelectionFrame *sel = this->GetSelectedWidget();
    
    int nb_deleted = 0;
    int done = 0;
    while (!done)
      {
      done = 1;
      vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
        this->Internals->Pool.begin();
      vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
        this->Internals->Pool.end();
      for (; it != end; ++it)
        {
        if (it->Widget && !it->Group.compare(group))
          {
          vtkKWSelectionFrame *widget = it->Widget;
          this->Internals->Pool.erase(it);
          this->DeleteWidget(widget);
          nb_deleted++;
          done = 0;
          break;
          }
        }
      }
    
    if (nb_deleted)
      {
      if (!this->HasWidget(sel))
        {
        this->SelectWidget(this->GetNthWidget(0));
        }
      this->ScheduleNumberOfWidgetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SetWidgetTag(
  vtkKWSelectionFrame *widget, 
  const char *tag)
{
  // Valid tag ?

  if (!widget || !tag || !*tag)
    {
    return 0;
    }

  // OK, tag it

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget == widget)
      {
      it->Tag = tag;
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrameLayoutManager::GetWidgetTag(
  vtkKWSelectionFrame *widget)
{
  if (widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget == widget)
        {
        return it->Tag.c_str();
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SetWidgetGroup(
  vtkKWSelectionFrame *widget, 
  const char *group)
{
  // Valid group ?

  if (!widget || !group || !*group)
    {
    return 0;
    }

  // OK, group it

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget == widget && strcmp(it->Group.c_str(), group))
      {
      it->Group = group;
      this->UpdateSelectionLists();
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrameLayoutManager::GetWidgetGroup(
  vtkKWSelectionFrame *widget)
{
  if (widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget == widget)
        {
        return it->Group.c_str();
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::ShowWidgetsWithGroup(const char *group)
{
  if (!group || !*group)
    {
    return 0;
    }

  int nb_widgets_in_group = this->GetNumberOfWidgetsWithGroup(group);
  int row, col, i;

  // Save selection

  int old_sel_pos[2];
  vtkKWSelectionFrame *old_sel = this->GetSelectedWidget();
  this->GetWidgetPosition(old_sel, old_sel_pos);

  // Inspect all selection frame, and check if they already display the group
  // we want to make visible

  for (row = this->Origin[1]; 
       row < this->Origin[1] + this->Resolution[1]; row++)
    {
    for (col = this->Origin[0]; 
         col < this->Origin[0] + this->Resolution[0]; col++)
      {
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(col, row);
      if (widget)
        {
        const char *widget_group = this->GetWidgetGroup(widget);
        if (widget_group && strcmp(widget_group, group))
          {
          // The selection frame is not the right group, look for another one
          // with the right group, and exchange both

          for (i = 0; i < nb_widgets_in_group; i++)
            {
            vtkKWSelectionFrame *new_widget = 
              this->GetNthWidgetWithGroup(i, group);
            if (new_widget)
              {
              int new_row, new_col;
              this->GetWidgetPosition(new_widget, &new_col, &new_row);
              if (new_col < this->Origin[0] || new_row < this->Origin[1] || 
                  new_row > row || (new_row == row && new_col > col))
                {
                this->SetWidgetPosition(new_widget, col, row);
                this->SetWidgetPosition(widget, new_col, new_row);
                break;
                }
              }
            }
          }
        }
      }
    }

  // Restore the selection

  if (old_sel)
    {
    vtkKWSelectionFrame *atpos = this->GetWidgetAtPosition(old_sel_pos);
    if (atpos && atpos != old_sel)
      {
      this->SelectWidget(atpos);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetSelectedWidget()
{
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget->GetSelected())
      {
      return it->Widget;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectWidget(
  vtkKWSelectionFrame *widget)
{
  // Deselect all widgets and select the right one (if any)

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget != widget)
      {
      it->Widget->SetSelected(0);
      }
    }
  if (widget)
    {
    widget->SetSelected(1);
    this->InvokeSelectionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SetSelectionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::InvokeSelectionChangedCommand()
{
  this->InvokeObjectMethodCommand(this->SelectionChangedCommand);
  this->InvokeEvent(
    vtkKWSelectionFrameLayoutManager::SelectionChangedEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectWidgetCallback(
  vtkKWSelectionFrame *selection)
{
  this->SelectWidget(selection);
}

//---------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetWidgetVisibility(
  vtkKWSelectionFrame *widget)
{
  int col, row;
  return (this->GetWidgetPosition(widget, &col, &row) &&
          this->IsPositionInLayout(col, row));
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SaveLayoutBeforeMaximize()
{
  this->PushResolution(this->GetResolution());
  this->PushPosition(this->GetOrigin());
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::RestoreLayoutBeforeMaximize()
{
  int res[2], origin[2];
  if (this->PopResolution(res) && this->PopPosition(origin))
    {
    this->SetResolutionAndOrigin(res, origin);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::IsWidgetMaximized(
  vtkKWSelectionFrame *widget)
{
  return (this->GetWidgetVisibility(widget) &&
          this->Resolution[0] == 1 && this->Resolution[1] == 1);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::MaximizeWidget(
  vtkKWSelectionFrame *widget)
{
  // Save the resolution and the position so that both can
  // be restored on UndoMaximize

  int pos[2];
  if (!this->IsWidgetMaximized(widget) &&
      this->GetWidgetPosition(widget, pos))
    {
    this->SaveLayoutBeforeMaximize();

    // Set the resolution to full (1, 1) and origin to the widget pos

    this->SetResolutionAndOrigin(1, 1, pos[0], pos[1]);
    
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::UndoMaximizeWidget()
{
  if (this->Resolution[0] == 1 && this->Resolution[1] == 1 && this->Internals)
    {
    return this->RestoreLayoutBeforeMaximize();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::ToggleMaximizeWidget(
  vtkKWSelectionFrame *widget)
{
  if (this->IsWidgetMaximized(widget))
    {
    return this->UndoMaximizeWidget();
    }
  else
    {
    return this->MaximizeWidget(widget);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectAndMaximizeWidgetCallback(
  vtkKWSelectionFrame *selection)
{
  this->SelectWidget(selection);
  this->ToggleMaximizeWidget(selection);
}

//---------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SwitchWidgetsPosition(
  vtkKWSelectionFrame *w1, vtkKWSelectionFrame *w2)
{
  if (!w1 || !w2 || w1 == w2)
    {
    return 0;
    }

  int pos1[2], pos2[2];
  if (!this->GetWidgetPosition(w1, pos1) ||
      !this->GetWidgetPosition(w2, pos2))
    {
    return 0;
    }
  
  this->SetWidgetPositionInternal(w1, -1, -1);
  this->SetWidgetPositionInternal(w2, -1, -1);

  this->SetWidgetPosition(w1, pos2);
  this->SetWidgetPosition(w2, pos1);
  
  return 1;
}

//---------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SwitchWidgetCallback(
  const char *title, vtkKWSelectionFrame *widget)
{
  // Get the widget we want to see in place of the current
  // widget

  vtkKWSelectionFrame *new_widget = this->GetWidgetWithTitle(title);
  if (!new_widget || new_widget == widget)
    {
    return;
    }

  // Switch both

  this->SwitchWidgetsPosition(widget, new_widget);

  // Select the new one

  new_widget->SelectCallback();

  // Make sure each selection list is updated to point at the right title
  // (since this callback was most likely triggered by selecting a 
  // *different* title in the list
  
  if (widget->GetSelectionListMenuButton() && widget->GetTitle())
    {
    widget->GetSelectionListMenuButton()->SetValue(widget->GetTitle());
    }
  if (new_widget->GetSelectionListMenuButton() && new_widget->GetTitle())
    {
    new_widget->GetSelectionListMenuButton()->SetValue(new_widget->GetTitle());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CloseWidgetCallback(
  vtkKWSelectionFrame *widget)
{
  this->RemoveWidget(widget);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::WidgetTitleChangedCallback(
  vtkKWSelectionFrame *)
{
  this->UpdateSelectionLists();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::ChangeWidgetTitleCallback(
  vtkKWSelectionFrame *widget)
{
  if (!widget)
    {
    return 0;
    }

  // Create a dialog to ask for a new title

  vtkKWSimpleEntryDialog *dlg = vtkKWSimpleEntryDialog::New();
  dlg->SetMasterWindow(this->GetParentTopLevel());
  dlg->SetDisplayPositionToPointer();
  dlg->SetTitle(
    ks_("Selection Frame Manager|Dialog|Title|Change frame title"));
  dlg->SetStyleToOkCancel();
  dlg->Create();
  dlg->GetEntry()->GetLabel()->SetText(
    ks_("Selection Frame Manager|Dialog|Name:"));
  dlg->SetText(
    ks_("Selection Frame Manager|Dialog|Enter a new title for this frame"));

  int ok = dlg->Invoke();
  if (ok)
    {
    vtksys_stl::string new_title(dlg->GetEntry()->GetWidget()->GetValue());
    ok = this->CanWidgetTitleBeChanged(widget, new_title.c_str());
    if (!ok)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this->GetParentTopLevel(), 
        ks_("Selection Frame Manager|Dialog|Title|Change frame title - Error!"),
        ks_("Selection Frame Manager|There is a problem with the new title you provided."),
        vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      widget->SetTitle(new_title.c_str());
      this->UpdateSelectionLists();
      }
    }

  dlg->Delete();
  return ok; 
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::CanWidgetTitleBeChanged(
    vtkKWSelectionFrame *widget, const char *new_title)
{
  return (widget && 
          new_title && 
          *new_title && 
          (!widget->GetTitle() || strcmp(widget->GetTitle(), new_title)));
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::ResolutionCallback(
  int nb_cols, int nb_rows)
{
  // If something is selected, let's try to center the origin around it
  // If 1x1 is requested, use maximize code instead, so that the previous
  // resolution is remembered and we can minimize later on.

  vtkKWSelectionFrame *sel = this->GetSelectedWidget();
  if (!sel)
    {
    this->SetResolutionAndOrigin(nb_cols, nb_rows, 0, 0);
    }
  else
    {
    if (nb_cols == 1 && nb_rows == 1)
      {
      this->ToggleMaximizeWidget(sel);
      }
    else
      {
      int origin[2];
      this->GetWidgetPosition(sel, origin);
      origin[0] = origin[0] - nb_cols + 1;
      if (origin[0] < 0)
        {
        origin[0] = 0;
        }
      origin[1] = origin[1] - nb_rows + 1;
      if (origin[1] < 0)
        {
        origin[1] = 0;
        }
      this->SetResolutionAndOrigin(nb_cols, nb_rows, origin[0], origin[1]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    this->PropagateEnableState(it->Widget);
    }

  this->PropagateEnableState(this->ResolutionEntriesMenu);
  this->PropagateEnableState(this->ResolutionEntriesToolbar);

  // Enable/Disable some entries

  this->UpdateResolutionEntriesMenu();
  this->UpdateResolutionEntriesToolbar();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateSelectionLists()
{
  if (!this->Internals ||
      !this->Internals->Pool.size())
    {
    return;
    }
  
  // Allocate array of titles
  // Separate each group

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator begin = 
    this->Internals->Pool.begin();

  vtkStringArray *str_array = vtkStringArray::New();

  const char *separator = "--";
  const char *prev_group = (begin != end ? begin->Group.c_str() : NULL);

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = begin;
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget->GetTitle())
      {
      if (strcmp(it->Group.c_str(), prev_group))
        {
        str_array->InsertNextValue(separator);
        prev_group = it->Group.c_str();
        }
      str_array->InsertNextValue(it->Widget->GetTitle());
      }
    }

  it = begin;
  for (; it != end; ++it)
    {
    if (it->Widget)
      {
      it->Widget->SetSelectionList(str_array);
      if (it->Widget->GetSelectionListMenuButton() && it->Widget->GetTitle())
        {
        it->Widget->GetSelectionListMenuButton()->SetValue(
          it->Widget->GetTitle());
        }
      }
    }

  str_array->Delete();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendWidgetsToImageData(
  vtkImageData *image, int selection_only, int direct, 
  int ForceUpdateOnScreenRendering)
{
  int nb_slots = this->Resolution[0] * this->Resolution[1];

  // We need a window to image filter for each widget in the grid

  vtksys_stl::vector<vtkWindowToImageFilter*> w2i_filters;
  w2i_filters.assign(nb_slots, (vtkWindowToImageFilter*)NULL);

  // We also need a pad filter to add a small margin for each widget 

  vtksys_stl::vector<vtkImageConstantPad*> pad_filters;
  pad_filters.assign(nb_slots, (vtkImageConstantPad*)NULL);

  // We need an append filter for each row in the grid, to append
  // widgets horizontally

  vtksys_stl::vector<vtkImageAppend*> append_filters;
  append_filters.assign(this->Resolution[1], (vtkImageAppend*)NULL);

  // We need an append filter to append each rows (see above) and form
  // the final picture

  vtkImageAppend *append_all = vtkImageAppend::New();
  append_all->SetAppendAxis(1);

  int spacing = 4;

  // Build the whole pipeline

  int i, j;
  int pos[2]; 
  for (j = this->Resolution[1] - 1; j >= 0; j--)
    {
    append_filters[j] = vtkImageAppend::New();
    append_filters[j]->SetAppendAxis(0);
    for (i = 0; i < this->Resolution[0]; i++)
      {
      pos[0] = this->Origin[0] + i; 
      pos[1] = this->Origin[1] + j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetRenderWidget(widget);
        if (rwwidget)
          {
          int idx = j * this->Resolution[0] + i;
          w2i_filters[idx] = vtkWindowToImageFilter::New();
          int offscreen = rwwidget->GetOffScreenRendering();
          if (direct)
            {
            if (!ForceUpdateOnScreenRendering) // true by default.
              {
              w2i_filters[idx]->ShouldRerenderOff();
              }
            }
          else
            {
            rwwidget->SetOffScreenRendering(1);
            }
          w2i_filters[idx]->SetInput(rwwidget->GetRenderWindow());
          w2i_filters[idx]->Update();
          rwwidget->SetOffScreenRendering(offscreen);

          int ext[6];
          w2i_filters[idx]->GetOutput()->GetWholeExtent(ext);
          pad_filters[idx] = vtkImageConstantPad::New();
          pad_filters[idx]->SetInput(w2i_filters[idx]->GetOutput());
          pad_filters[idx]->SetConstant(255);
          pad_filters[idx]->SetOutputWholeExtent(
            ext[0] - spacing, ext[1] + spacing,
            ext[2] - spacing, ext[3] + spacing,
            ext[4], ext[5]);
          pad_filters[idx]->Update();

          append_filters[j]->AddInput(pad_filters[idx]->GetOutput());
          }
        }
      }

    if (append_filters[j]->GetNumberOfInputConnections(0))
      {
      append_all->AddInput(append_filters[j]->GetOutput());
      append_filters[j]->Update();
      }
    }

  // Create the final output

  if (append_all->GetNumberOfInputConnections(0))
    {
    append_all->Update();
    image->ShallowCopy(append_all->GetOutput());
    }

  // Deallocate

  append_all->Delete();

  for (j = 0; j < this->Resolution[1]; j++)
    {
    append_filters[j]->Delete();
    for (i = 0; i < this->Resolution[0]; i++)
      {
      pos[0] = this->Origin[0] + i; 
      pos[1] = this->Origin[1] + j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetRenderWidget(widget);
        if (rwwidget && !direct)
          {
          rwwidget->Render();
          }
        }
      int idx = j * this->Resolution[0] + i;
      if (w2i_filters[idx])
        {
        w2i_filters[idx]->Delete();
        }
      if (pad_filters[idx])
        {
        pad_filters[idx]->Delete();
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendAllWidgetsToImageData(
  vtkImageData *image, int OnScreenRendering)
{
  if (OnScreenRendering)
    {
    return this->AppendWidgetsToImageData(image, 0, 1, OnScreenRendering);
    }
  return this->AppendWidgetsToImageData(image, 0, 0, OnScreenRendering);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendAllWidgetsToImageDataFast(
  vtkImageData *image)
{
  return this->AppendWidgetsToImageData(image, 0, 1);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendSelectedWidgetToImageData(
  vtkImageData *image)
{
  return this->AppendWidgetsToImageData(image, 1, 0);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendSelectedWidgetToImageDataFast(
  vtkImageData *image)
{
  return this->AppendWidgetsToImageData(image, 1, 1);
}

//---------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SaveScreenshotAllWidgets()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  vtkKWSaveImageDialog *save_dialog = vtkKWSaveImageDialog::New();
  save_dialog->SetParent(this->GetParentTopLevel());
  save_dialog->Create();
  save_dialog->SetTitle(
    ks_("Selection Frame Manager|Dialog|Title|Save Screenshot"));
  save_dialog->RetrieveLastPathFromRegistry("SavePath");
  
  int res = 0;
  if (save_dialog->Invoke() && 
      this->SaveScreenshotAllWidgetsToFile(save_dialog->GetFileName()))
    {
    save_dialog->SaveLastPathToRegistry("SavePath");
    res = 1;
    }

  save_dialog->Delete();

  return res;
}

//---------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SaveScreenshotAllWidgetsToFile(
  const char* fname)
{
  if (!fname)
    {
    return 0;
    }

  // Append all widgets to an image

  vtkImageData *iData = vtkImageData::New();
  if (!this->AppendAllWidgetsToImageData(iData))
    {
    iData->Delete();
    return 0;
    }

  int extent[6];
  iData->GetExtent(extent);
  if (extent[0] > extent[1] && extent[2] > extent[3] && extent[4] > extent[5])
    {
    iData->Delete();
    return 0;
    }

  // Now save it

  const char *ext = fname + strlen(fname) - 4;
  
  int success = 1;

  if (!strcmp(ext, ".bmp"))
    {
    vtkBMPWriter *bmp = vtkBMPWriter::New();
    bmp->SetInput(iData);
    bmp->SetFileName(fname);
    bmp->Write();
    if (bmp->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    bmp->Delete();
    }
  else if (!strcmp(ext, ".tif"))
    {
    vtkTIFFWriter *tif = vtkTIFFWriter::New();
    tif->SetInput(iData);
    tif->SetFileName(fname);
    tif->Write();
    if (tif->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    tif->Delete();
    }
  else if (!strcmp(ext, ".ppm"))
    {
    vtkPNMWriter *pnm = vtkPNMWriter::New();
    pnm->SetInput(iData);
    pnm->SetFileName(fname);
    pnm->Write();
    if (pnm->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    pnm->Delete();
    }
  else if (!strcmp(ext, ".png"))
    {
    vtkPNGWriter *png = vtkPNGWriter::New();
    png->SetInput(iData);
    png->SetFileName(fname);
    png->Write();
    if (png->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    png->Delete();
    }
  else if (!strcmp(ext, ".jpg"))
    {
    vtkJPEGWriter *jpg = vtkJPEGWriter::New();
    jpg->SetInput(iData);
    jpg->SetFileName(fname);
    jpg->Write();
    if (jpg->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    jpg->Delete();
    }
  
  if (!success)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->GetParentTopLevel(), 
      ks_("Selection Frame Manager|Dialog|Title|Save Screenshot - Error!"),
      k_("There was a problem writing the image file.\n"
         "Please check the location and make sure you have write\n"
         "permissions and enough disk space."),
      vtkKWMessageDialog::ErrorIcon);
    }
  iData->Delete();

  return success;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::CopyScreenshotAllWidgetsToClipboard()
{
  // Append all widgets to an image

  vtkImageData *iData = vtkImageData::New();
  if (!this->AppendAllWidgetsToImageData(iData))
    {
    iData->Delete();
    return 0;
    }

  int *extent = iData->GetExtent();
  if (extent[0] > extent[1] && extent[2] > extent[3] && extent[4] > extent[5])
    {
    iData->Delete();
    return 0;
    }

  // Save to clipboard

#ifdef _WIN32

  vtkKWSelectionFrame *widget = this->GetSelectedWidget();
  if (!widget)
    {
    return 0;
    }

  vtkKWRenderWidget *rwwidget = this->GetRenderWidget(widget);
  if (!rwwidget)
    {
    return 0;
    }

  if (::OpenClipboard((HWND)rwwidget->GetRenderWindow()->GetGenericWindowId()))
    {
    extent = iData->GetWholeExtent();
    
    int size[2];
    size[0] = extent[1] - extent[0] + 1;
    size[1] = extent[3] - extent[2] + 1;

    int data_width = ((size[0] * 3 + 3) / 4) * 4;
    int src_width = size[0] * 3;
  
    EmptyClipboard();

    DWORD dwLen = sizeof(BITMAPINFOHEADER) + data_width * size[1];
    HANDLE hDIB = ::GlobalAlloc(GHND, dwLen);
    LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
    
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth = size[0];
    lpbi->biHeight = size[1];
    lpbi->biPlanes = 1;
    lpbi->biBitCount = 24;
    lpbi->biCompression = BI_RGB;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
    lpbi->biSizeImage = data_width * size[1];
    
    // Copy the data to the clipboard

    unsigned char *ptr = (unsigned char *)(iData->GetScalarPointer());
    unsigned char *dest = (unsigned char *)lpbi + lpbi->biSize;

    int i,j;
    for (i = 0; i < size[1]; i++)
      {
      for (j = 0; j < size[0]; j++)
        {
        *dest++ = ptr[2];
        *dest++ = ptr[1];
        *dest++ = *ptr;
        ptr += 3;
        }
      dest = dest + (data_width - src_width);
      }
    
    SetClipboardData (CF_DIB, hDIB);
    ::GlobalUnlock(hDIB);
    CloseClipboard();
    }           
#endif

  iData->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintWidgets(
  double dpi, int selection_only)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkKWSelectionFrame *first_widget = this->GetNthWidget(0);
  if (!first_widget)
    {
    }
  
  PRINTDLG pd;
  DOCINFO di;
  RECT rcDest = { 0, 0, 0, 0};
  
  memset((void *)&pd, 0, sizeof(PRINTDLG));

  pd.lStructSize = sizeof(PRINTDLG);
  vtkKWRenderWidget *first_rwwidget = 
    this->GetRenderWidget(first_widget);
  if (first_rwwidget)
    {
    pd.hwndOwner = (HWND)first_rwwidget->GetRenderWindow()->GetGenericWindowId();
    }
  pd.Flags = PD_RETURNDC;
  pd.hInstance = NULL;
  
  PrintDlg(&pd);
  HDC ghdc = pd.hDC;

  if (!ghdc)
    {
    return 0;
    }

  if (pd.hDevMode)
    {
    GlobalFree(pd.hDevMode);
    }
  if (pd.hDevNames)
    {
    GlobalFree(pd.hDevNames);
    }
  
  if (this->IsCreated())
    {
    vtkKWTkUtilities::SetTopLevelMouseCursor(this, "watch");
    this->GetApplication()->ProcessPendingEvents();
    }
  
  di.cbSize = sizeof(DOCINFO);
  di.lpszDocName = "Kitware Test";
  di.lpszOutput = NULL;
  
  StartDoc(ghdc, &di);
  StartPage(ghdc);

  // Get size of printer page in pixels

  int cxPage = GetDeviceCaps(ghdc, HORZRES);
  int cyPage = GetDeviceCaps(ghdc, VERTRES);

  // Get printer DPI

  int cxInch = GetDeviceCaps(ghdc, LOGPIXELSX);
  int cyInch = GetDeviceCaps(ghdc, LOGPIXELSY);

  double scale = (double)cxInch / dpi;
  
  SetStretchBltMode(ghdc, HALFTONE);
 
  // If only the selection is to be printed, set the res to 1, 1

  int res[2];
  if (selection_only)
    {
    res[0] = res[1] = 1;
    }
  else
    {
    res[0] = this->Resolution[0];
    res[1] = this->Resolution[1];
    }

  // First pass to compute the total size (i.e. the resolution * biggest win)

  int max_size[2] = { -1, -1 };

  int i, j;
  int pos[2]; 
  for (j = 0; j < this->Resolution[1]; j++)
    {
    for (i = 0; i < this->Resolution[0]; i++)
      {
      pos[0] = this->Origin[0] + i; 
      pos[1] = this->Origin[1] + j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetRenderWidget(widget);
        if (rwwidget)
          {
          int *size = rwwidget->GetRenderWindow()->GetSize();
          if (max_size[0] < size[0])
            {
            max_size[0] = size[0];
            }
          if (max_size[1] < size[1])
            {
            max_size[1] = size[1];
            }
          }
        }
      }
    }

  int spacing = 4;

  int total_size[2];
  total_size[0] = res[0] * (max_size[0] + 2 * spacing);
  total_size[1] = res[1] * (max_size[1] + 2 * spacing);

  double ratio[2];
  ratio[0] = (double)max_size[0] / (double)total_size[0];
  ratio[1] = (double)max_size[1] / (double)total_size[1];

  // Print each widget (or the selection only)

  for (j = 0; j < this->Resolution[1]; j++)
    {
    for (i = 0; i < this->Resolution[0]; i++)
      {
      pos[0] = this->Origin[0] + i; 
      pos[1] = this->Origin[1] + j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetRenderWidget(widget);
        if (rwwidget)
          {
          int i2, j2;
          if (selection_only)
            {
            i2 = j2 = 0;
            }
          else
            {
            i2 = i;
            j2 = j;
            }
          int printing = rwwidget->GetPrinting();
          rwwidget->SetPrinting(1);
          rwwidget->SetupPrint(
            rcDest, ghdc, cxPage, cyPage, cxInch, cyInch,
            ratio[0], ratio[1], total_size[0], total_size[1]);
          rwwidget->Render();

          StretchBlt(
            ghdc, 
            (double)rcDest.right * 
            (spacing + i2 * (max_size[0] + 2 * spacing))/(double)total_size[0],
            (double)rcDest.top * 
            (spacing + j2 * (max_size[1] + 2 * spacing))/(double)total_size[1],
            (double)rcDest.right * ratio[0], 
            (double)rcDest.top * ratio[1],
            (HDC)rwwidget->GetMemoryDC(), 
            0, 
            0,
            (double)rcDest.right / scale * ratio[0], 
            (double)rcDest.top / scale * ratio[1], 
            SRCCOPY);

          rwwidget->SetPrinting(printing);
          }
        }
      }
    }
  
  // Close the page

  EndPage(ghdc);
  EndDoc(ghdc);
  DeleteDC(ghdc);

  if (this->IsCreated())
    {
    vtkKWTkUtilities::SetTopLevelMouseCursor(this, NULL);
    }

  // At that point the Print Dialog does not seem to disappear.
  // Let's Render()

  if (this->IsCreated())
    {
    this->GetApplication()->ProcessPendingEvents();
    }
#else

  (void)dpi;
  (void)selection_only;

#endif

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintAllWidgets()
{
  if (this->GetApplication())
    {
    return this->PrintAllWidgetsAtResolution(
      this->GetApplication()->GetPrintTargetDPI());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintAllWidgetsAtResolution(double dpi)
{
  return this->PrintWidgets(dpi, 0);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintSelectedWidget()
{
  if (this->GetApplication())
    {
    return this->PrintSelectedWidgetAtResolution(
      this->GetApplication()->GetPrintTargetDPI());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintSelectedWidgetAtResolution(
  double dpi)
{
  return this->PrintWidgets(dpi, 1);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution[0] << " x " 
     << this->Resolution[1] << endl;

  os << indent << "ResolutionEntriesMenu: " << this->ResolutionEntriesMenu << endl;
  os << indent << "ResolutionEntriesToolbar: " << this->ResolutionEntriesToolbar << endl;
  os << indent << "ReorganizeWidgetPositionsAutomatically: " << this->ReorganizeWidgetPositionsAutomatically << endl;
}
