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

#include "vtkKWWindowLevelPresetSelector.h"

#include "vtkImageData.h"
#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkKWBalloonHelpManager.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWSpinButtons.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWindowToImageFilter.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/string>

#include <time.h>

#define VTK_KW_WLPS_ID_COL         0
#define VTK_KW_WLPS_ICON_COL       1
#define VTK_KW_WLPS_WINDOW_COL     2
#define VTK_KW_WLPS_LEVEL_COL      3
#define VTK_KW_WLPS_COMMENT_COL    4

#define VTK_KW_WLPS_BUTTON_ADD_ID    0
#define VTK_KW_WLPS_BUTTON_REMOVE_ID 1

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindowLevelPresetSelector);
vtkCxxRevisionMacro(vtkKWWindowLevelPresetSelector, "1.7");

//----------------------------------------------------------------------------
class vtkKWWindowLevelPresetSelectorInternals
{
public:
  struct PoolNode
  {
    int Id;
    double Window;
    double Level;
    vtksys_stl::string Group;
    vtksys_stl::string Comment;
    clock_t CreationTime;
    vtkKWIcon *Thumbnail;
    vtkKWIcon *Screenshot;

    PoolNode();
    ~PoolNode();
  };

  static int PoolNodeCounter;

  typedef vtksys_stl::vector<PoolNode*> PoolType;
  typedef vtksys_stl::vector<PoolNode*>::iterator PoolIterator;

  PoolType Pool;

  PoolIterator GetPoolNode(int id);
};

int vtkKWWindowLevelPresetSelectorInternals::PoolNodeCounter = 0;

//---------------------------------------------------------------------------
vtkKWWindowLevelPresetSelectorInternals::PoolIterator 
vtkKWWindowLevelPresetSelectorInternals::GetPoolNode(int id)
{
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Pool.begin();
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
    this->Pool.end();
  for (; it != end; ++it)
    {
    if ((*it)->Id == id)
      {
      return it;
      }
    }
  return end;
}

//---------------------------------------------------------------------------
vtkKWWindowLevelPresetSelectorInternals::PoolNode::PoolNode()
{
  this->Thumbnail = NULL;
  this->Screenshot = NULL;
}

//---------------------------------------------------------------------------
vtkKWWindowLevelPresetSelectorInternals::PoolNode::~PoolNode()
{
  if (this->Thumbnail)
    {
    this->Thumbnail->Delete();
    this->Thumbnail = NULL;
    }
  if (this->Screenshot)
    {
    this->Screenshot->Delete();
    this->Screenshot = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWWindowLevelPresetSelector::vtkKWWindowLevelPresetSelector()
{
  this->Internals = new vtkKWWindowLevelPresetSelectorInternals;

  this->AddWindowLevelPresetCommand    = NULL;
  this->ApplyWindowLevelPresetCommand  = NULL;
  this->RemoveWindowLevelPresetCommand = NULL;

  this->PresetList        = NULL;
  this->ControlFrame      = NULL;
  this->PresetSpinButtons = NULL;
  this->PresetButtons     = NULL;

  this->ApplyPresetOnSingleClick = 1;

  this->ThumbnailSize = 32;
  this->ScreenshotSize = 144;

  this->GroupFilter = NULL;
}

//----------------------------------------------------------------------------
vtkKWWindowLevelPresetSelector::~vtkKWWindowLevelPresetSelector()
{
  if (this->PresetList)
    {
    this->PresetList->Delete();
    this->PresetList = NULL;
    }

  if (this->ControlFrame)
    {
    this->ControlFrame->Delete();
    this->ControlFrame = NULL;
    }

  if (this->PresetSpinButtons)
    {
    this->PresetSpinButtons->Delete();
    this->PresetSpinButtons = NULL;
    }

  if (this->PresetButtons)
    {
    this->PresetButtons->Delete();
    this->PresetButtons = NULL;
    }

  if (this->AddWindowLevelPresetCommand)
    {
    delete [] this->AddWindowLevelPresetCommand;
    this->AddWindowLevelPresetCommand = NULL;
    }

  if (this->ApplyWindowLevelPresetCommand)
    {
    delete [] this->ApplyWindowLevelPresetCommand;
    this->ApplyWindowLevelPresetCommand = NULL;
    }

  if (this->RemoveWindowLevelPresetCommand)
    {
    delete [] this->RemoveWindowLevelPresetCommand;
    this->RemoveWindowLevelPresetCommand = NULL;
    }

  // Remove all presets

  this->RemoveAllWindowLevelPresets();

  // Delete our pool

  delete this->Internals;
  this->Internals = NULL;

  this->SetGroupFilter(NULL);
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  // --------------------------------------------------------------
  // Preset : preset list

  if (!this->PresetList)
    {
    this->PresetList = vtkKWMultiColumnListWithScrollbars::New();
    }

  this->PresetList->SetParent(this);
  this->PresetList->Create(app);
  this->PresetList->HorizontalScrollbarVisibilityOff();

  this->Script(
    "pack %s -side top -anchor nw -fill both -expand t -padx 2 -pady 2",
    this->PresetList->GetWidgetName());

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  if (this->ApplyPresetOnSingleClick)
    {
    list->SetSelectionModeToSingle();
    }
  else
    {
    list->SetSelectionModeToExtended();
    }
  list->SetSelectionCommand(
    this, "PresetSelectionCallback");
  list->SetPotentialCellBackgroundColorChangedCommand(
    list, "RefreshBackgroundColorOfAllCellsWithWindowCommand");
  // list->SetSelectionBackgroundColor(0.988, 1.0, 0.725);
  // list->ColumnLabelsVisibilityOff();
  list->ColumnSeparatorsVisibilityOn();
  list->SetEditStartCommand(this, "PresetCellEditStartCallback");
  list->SetEditEndCommand(this, "PresetCellEditEndCallback");

  int col = 0;

  // We need that column to retrieve the Id

  list->AddColumn("Id");
  list->ColumnVisibilityOff(col);
  col++;

  list->AddColumn("Image");
  list->SetColumnWidth(col, -this->ThumbnailSize);
  list->SetColumnResizable(col, 0);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 0);
  list->SetColumnSortModeToReal(col);
  list->SetColumnFormatCommandToEmptyOutput(col);
  col++;

  list->AddColumn("Window");
  list->SetColumnWidth(col, 8);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 1);
  list->SetColumnSortModeToReal(col);
  col++;

  list->AddColumn("Level");
  list->SetColumnWidth(col, 8);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 1);
  list->SetColumnSortModeToReal(col);
  col++;

  list->AddColumn("Comment");
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 1);
  list->SetColumnEditable(col, 1);
  col++;

  // --------------------------------------------------------------
  // Preset : control frame

  if (!this->ControlFrame)
    {
    this->ControlFrame = vtkKWFrame::New();
    }

  this->ControlFrame->SetParent(this);
  this->ControlFrame->Create(app);

  this->Script("pack %s -side top -anchor nw -fill both -expand t",
               this->ControlFrame->GetWidgetName());

  // --------------------------------------------------------------
  // Preset : spin buttons

  if (!this->PresetSpinButtons)
    {
    this->PresetSpinButtons = vtkKWSpinButtons::New();
    }

  this->PresetSpinButtons->SetParent(this->ControlFrame);
  this->PresetSpinButtons->Create(app);
  this->PresetSpinButtons->SetLayoutOrientationToHorizontal();
  this->PresetSpinButtons->SetArrowOrientationToVertical();
  this->PresetSpinButtons->SetButtonsPadX(2);
  this->PresetSpinButtons->SetButtonsPadY(2);
  this->PresetSpinButtons->GetPreviousButton()->SetBalloonHelpString(
    "Select and apply previous preset");
  this->PresetSpinButtons->GetNextButton()->SetBalloonHelpString(
    "Select and apply next preset");
  this->PresetSpinButtons->SetPreviousCommand(
    this, "PresetSelectAndApplyPreviousCallback");
  this->PresetSpinButtons->SetNextCommand(
    this, "PresetSelectAndApplyNextCallback");

  this->Script("pack %s -side left -anchor nw -fill both -expand t",
               this->PresetSpinButtons->GetWidgetName());

  // --------------------------------------------------------------
  // Preset : buttons

  if (!this->PresetButtons)
    {
    this->PresetButtons = vtkKWPushButtonSet::New();
    }

  this->PresetButtons->SetParent(this->ControlFrame);
  this->PresetButtons->PackHorizontallyOn();
  this->PresetButtons->SetWidgetsPadX(2);
  this->PresetButtons->SetWidgetsPadY(2);
  this->PresetButtons->SetWidgetsInternalPadY(1);
  this->PresetButtons->Create(app);

  this->Script("pack %s -side left -anchor nw -fill x -expand t",
               this->PresetButtons->GetWidgetName());

  vtkKWPushButton *pb = NULL;

  // add preset

  pb = this->PresetButtons->AddWidget(VTK_KW_WLPS_BUTTON_ADD_ID);
  pb->SetImageToPredefinedIcon(vtkKWIcon::IconPlus);
  pb->SetCommand(this, "PresetAddCallback");
  pb->SetBalloonHelpString("Add a window/level preset");

  // remove preset

  pb = this->PresetButtons->AddWidget(VTK_KW_WLPS_BUTTON_REMOVE_ID);
  pb->SetImageToPredefinedIcon(vtkKWIcon::IconMinus);
  pb->SetCommand(this, "PresetRemoveCallback");
  pb->SetBalloonHelpString(
    "Remove the selected preset(s) from the list of presets");

  // Update enable state

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetApplyPresetOnSingleClick(int arg)
{
  if (this->ApplyPresetOnSingleClick == arg)
    {
    return;
    }

  this->ApplyPresetOnSingleClick = arg;
  this->Modified();

  if (this->PresetList)
    {
    if (this->ApplyPresetOnSingleClick)
      {
      this->PresetList->GetWidget()->SetSelectionModeToSingle();
      }
    else
      {
      this->PresetList->GetWidget()->SetSelectionModeToExtended();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetListHeight(int h)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetHeight(h);
    }
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetListHeight()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetHeight();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetImageColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      VTK_KW_WLPS_ICON_COL, arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetImageColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      VTK_KW_WLPS_ICON_COL);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetCommentColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      VTK_KW_WLPS_COMMENT_COL, arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetCommentColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      VTK_KW_WLPS_COMMENT_COL);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::AddWindowLevelPresetWithGroup(
  double window, double level, const char *group)
{
  // Do we have that preset already ? If not, create a new node

  int id;
  if (group && *group)
    {
    id = this->GetWindowLevelPresetIdWithGroup(window, level, group);
    }
  else
    {
    id = this->GetWindowLevelPresetId(window, level);
    }
  vtkKWWindowLevelPresetSelectorInternals::PoolNode *node = NULL;
  if (id < 0)
    {
    id =  vtkKWWindowLevelPresetSelectorInternals::PoolNodeCounter++;

    node = new vtkKWWindowLevelPresetSelectorInternals::PoolNode;

    node->Id = id;
    node->Window = window;
    node->Level = level;
    node->CreationTime = clock();
    if (group && *group)
      {
      node->Group = group;
      }

    this->Internals->Pool.push_back(node);

    this->UpdateRowInPresetList(node);
    }

  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();
    int row = list->FindCellTextAsIntInColumn(VTK_KW_WLPS_ID_COL, id);
    if (row >= 0)
      {
      list->SeeRow(row);
      }
    }

  if (node)
    {
    this->NumberOfWindowLevelPresetsHasChanged();
    }

  return id;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::AddWindowLevelPreset(
  double window, double level)
{
  return this->AddWindowLevelPresetWithGroup(window, level, NULL);
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetWindowLevelPresetGroup(
  int id, const char *group)
{
  // Check if there is already a preset corresponding to that group
  // If there is, remove it first

  double window, level;
  if (this->GetWindowLevelPreset(id, &window, &level))
    {
    int existing_id = 
      this->GetWindowLevelPresetIdWithGroup(window, level, group);
    if (existing_id >= 0)
      {
      if (existing_id == id)
        {
        return 1;
        }
      else
        {
        this->RemoveWindowLevelPreset(existing_id);
        }
      }
    }

  // Update the group

  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    (*it)->Group = group ? group : "";
    this->UpdateRowInPresetList(*it);
    this->Update(); // the number of visible widgets may have changed
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWWindowLevelPresetSelector::GetWindowLevelPresetGroup(
  int id)
{
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    return (*it)->Group.c_str();
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetWindowLevelPresetComment(
  int id, const char *group)
{
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    (*it)->Comment = group ? group : "";
    this->UpdateRowInPresetList(*it);
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWWindowLevelPresetSelector::GetWindowLevelPresetComment(
  int id)
{
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    return (*it)->Comment.c_str();
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNumberOfWindowLevelPresets()
{
  return this->Internals->Pool.size();
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNumberOfWindowLevelPresetsWithGroup(
  const char *group)
{
  int count = 0;
  if (group && *group)
    {
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (!(*it)->Group.compare(group))
        {
        count++;
        }
      }
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNumberOfVisibleWindowLevelPresets()
{
  if (this->GroupFilter && *this->GroupFilter)
    {
    return this->GetNumberOfWindowLevelPresetsWithGroup(this->GroupFilter);
    }
  return this->GetNumberOfWindowLevelPresets();
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::HasWindowLevelPreset(
  double window, double level)
{
  int id = this->GetWindowLevelPresetId(window, level);
  return id >= 0 ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::HasWindowLevelPresetWithGroup(
  double window, double level, const char *group)
{
  int id = this->GetWindowLevelPresetIdWithGroup(window, level, group);
  return id >= 0 ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetWindowLevelPreset(
  int id, double *window, double *level)
{
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    *window = (*it)->Window;
    *level = (*it)->Level;
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
double* vtkKWWindowLevelPresetSelector::GetWindowLevelPreset(int id)
{
  static double wl[2];
  if (this->GetWindowLevelPreset(id, wl, wl + 1))
    {
    return wl;
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNthWindowLevelPreset(
  int index, double *window, double *level)
{
  if (index < 0 || index >= this->GetNumberOfWindowLevelPresets())
    {
    return 0;
    }

  *window = this->Internals->Pool[index]->Window;
  *level = this->Internals->Pool[index]->Level;
  return 1;
}

//----------------------------------------------------------------------------
double* vtkKWWindowLevelPresetSelector::GetNthWindowLevelPreset(int index)
{
  static double wl[2];
  if (this->GetNthWindowLevelPreset(index, wl, wl + 1))
    {
    return wl;
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNthWindowLevelPresetWithGroup(
  int index, const char *group, double *window, double *level)
{
  int rank = this->GetNthWindowLevelPresetRankWithGroup(index, group);
  if (rank >= 0)
    {
    return this->GetNthWindowLevelPreset(rank, window, level);
    }
  return -1;
}

//----------------------------------------------------------------------------
double* vtkKWWindowLevelPresetSelector::GetNthWindowLevelPresetWithGroup(
  int index, const char *group)
{
  static double wl[2];
  if (this->GetNthWindowLevelPresetWithGroup(index, group, wl, wl + 1))
    {
    return wl;
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetWindowLevelPresetId(
  double window, double level)
{
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if ((*it)->Window == window && (*it)->Level == level)
      {
      return (*it)->Id;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetWindowLevelPresetIdWithGroup(
  double window, double level, const char *group)
{
  if (group && *group)
    {
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if ((*it)->Window == window && (*it)->Level == level &&
          !(*it)->Group.compare(group))
        {
        return (*it)->Id;
        }
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNthWindowLevelPresetId(int index)
{
  if (index >= 0 && index < this->GetNumberOfWindowLevelPresets())
    {
    return this->Internals->Pool[index]->Id;
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNthWindowLevelPresetIdWithGroup(
  int index, const char *group)
{
  int rank = this->GetNthWindowLevelPresetRankWithGroup(index, group);
  if (rank >= 0)
    {
    return this->GetNthWindowLevelPresetId(rank);
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNthVisibleWindowLevelPresetId(
  int row_index)
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();
    if (row_index >= 0 && row_index < list->GetNumberOfRows())
      {
      return list->GetCellTextAsInt(row_index, VTK_KW_WLPS_ID_COL);
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetNthWindowLevelPresetRankWithGroup(
  int index, const char *group)
{
  if (index >= 0 && group && *group)
    {
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (int nth = 0; it != end; ++it, nth++)
      {
      if (!(*it)->Group.compare(group))
        {
        index--;
        if (index < 0)
          {
          return nth;
          }
        }
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::RemoveWindowLevelPreset(
  int id)
{
  if (this->Internals)
    {
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->GetPoolNode(id);
    if (it != this->Internals->Pool.end())
      {
      if (this->PresetList)
        {
        vtkKWMultiColumnList *list = this->PresetList->GetWidget();
        int row = list->FindCellTextAsIntInColumn(VTK_KW_WLPS_ID_COL, id);
        if (row >= 0)
          {
          list->DeleteRow(row);
          }
        }
      delete (*it);
      this->Internals->Pool.erase(it);
      this->NumberOfWindowLevelPresetsHasChanged();
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::RemoveAllWindowLevelPresets()
{
  // Is faster than calling RemoveWindowLevelPreset on each preset

  if (this->PresetList)
    {
    this->PresetList->GetWidget()->DeleteAllRows();
    }
  if (this->Internals)
    {
    int nb_deleted = this->GetNumberOfWindowLevelPresets();
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      delete (*it);
      }
    this->Internals->Pool.clear();
    vtkKWWindowLevelPresetSelectorInternals::PoolNodeCounter = 0;
    if (nb_deleted)
      {
      this->NumberOfWindowLevelPresetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::RemoveAllWindowLevelPresetsWithGroup(
  const char *group)
{
  // Is faster than calling RemoveWindowLevelPreset on each preset

  if (this->Internals && group && *group)
    {
    int nb_deleted = 0;
    int done = 0;
    while (!done)
      {
      done = 1;
      vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
        this->Internals->Pool.begin();
      vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
        this->Internals->Pool.end();
      for (; it != end; ++it)
        {
        if (!(*it)->Group.compare(group))
          {
          if (this->PresetList)
            {
            vtkKWMultiColumnList *list = this->PresetList->GetWidget();
            int row = list->FindCellTextAsIntInColumn(
              VTK_KW_WLPS_ID_COL, (*it)->Id);
            if (row >= 0)
              {
              list->DeleteRow(row);
              }
            }
          delete (*it);
          this->Internals->Pool.erase(it);
          nb_deleted++;
          done = 0;
          break;
          }
        }
      }
    
    if (nb_deleted)
      {
      this->NumberOfWindowLevelPresetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::NumberOfWindowLevelPresetsHasChanged()
{
  this->Update(); // enable/disable some buttons valid only if we have presets
}

//---------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetWindowLevelPresetImage(
  int id, vtkImageData *screenshot)
{
  if (!this->Internals || !screenshot)
    {
    return 0;
    }

  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it == this->Internals->Pool.end())
    {
    return 0;
    }

  double factor;
  vtkImageData *output;

  vtkImageResample *resample = vtkImageResample::New();
  resample->SetInput(screenshot);
  resample->SetInterpolationModeToCubic();
  resample->SetDimensionality(2);

  // Create the thumbnail

  factor = (double)this->ThumbnailSize / 
    (double)screenshot->GetDimensions()[0];
  resample->SetAxisMagnificationFactor(0, factor);
  resample->SetAxisMagnificationFactor(1, factor);
  resample->Update();
  output = resample->GetOutput();

  if (!(*it)->Thumbnail)
    {
    (*it)->Thumbnail = vtkKWIcon::New();
    }
  (*it)->Thumbnail->SetImage(
    (const unsigned char*)output->GetScalarPointer(),
    output->GetDimensions()[0],
    output->GetDimensions()[1],
    3,
    0,
    vtkKWIcon::ImageOptionFlipVertical);

  // Create the screenshot

  factor = (double)this->ScreenshotSize / 
    (double)screenshot->GetDimensions()[0];
  resample->SetAxisMagnificationFactor(0, factor);
  resample->SetAxisMagnificationFactor(1, factor);
  resample->Update();
  output = resample->GetOutput();

  if (!(*it)->Screenshot)
    {
    (*it)->Screenshot = vtkKWIcon::New();
    }
  (*it)->Screenshot->SetImage(
    (const unsigned char*)output->GetScalarPointer(),
    output->GetDimensions()[0],
    output->GetDimensions()[1],
    3,
    0,
    vtkKWIcon::ImageOptionFlipVertical);

  resample->Delete();

  // Update the icon cell

  this->UpdateRowInPresetList(*it);
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();
    int row = list->FindCellTextAsIntInColumn(VTK_KW_WLPS_ID_COL, id);
    if (row >= 0)
      {
      list->RefreshCellWithWindowCommand(row, VTK_KW_WLPS_ICON_COL);
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetWindowLevelPresetImageFromRenderWindow(
  int id, vtkRenderWindow *win)
{
  if (win)
    {
    vtkWindowToImageFilter *filter = vtkWindowToImageFilter::New();
    filter->ShouldRerenderOff();
    filter->SetInput(win);
    filter->Update();
    int res = this->SetWindowLevelPresetImage(id, filter->GetOutput());
    filter->Delete();
    return res;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::HasWindowLevelPresetImage(int id)
{
  if (this->Internals)
    {
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->GetPoolNode(id);
    return (it != this->Internals->Pool.end() && (*it)->Thumbnail) ? 1 : 0;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetGroupFilter(const char* _arg)
{
  if (this->GroupFilter == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->GroupFilter && _arg && (!strcmp(this->GroupFilter, _arg))) 
    { 
    return;
    }

  if (this->GroupFilter) 
    { 
    delete [] this->GroupFilter; 
    }

  if (_arg)
    {
    this->GroupFilter = new char[strlen(_arg)+1];
    strcpy(this->GroupFilter,_arg);
    }
   else
    {
    this->GroupFilter = NULL;
    }

  this->Modified();

  this->UpdateRowsInPresetList();
} 

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::UpdateRowsInPresetList()
{
  if (this->Internals)
    {
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWWindowLevelPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      this->UpdateRowInPresetList(*it);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::UpdateRowInPresetList(void *ptr)
{
  if (!ptr || !this->PresetList)
    {
    return;
    }

  vtkKWWindowLevelPresetSelectorInternals::PoolNode *node = 
    static_cast<vtkKWWindowLevelPresetSelectorInternals::PoolNode*>(ptr);

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  // Look for this node in the list

  int row = list->FindCellTextAsIntInColumn(VTK_KW_WLPS_ID_COL, node->Id);

  int group_filter_exclude =
    this->GroupFilter && *this->GroupFilter && 
    node->Group.compare(this->GroupFilter);

  // Not found ? Insert it, or ignore it if the group filter does not match

  if (row < 0)
    {
    if (group_filter_exclude)
      {
      return;
      }
    list->AddRow();
    row = list->GetNumberOfRows() - 1;
    }

  // Found ? Remove it if the group filter does not match

  else
    {
    if (group_filter_exclude)
      {
      list->DeleteRow(row);
      return;
      }
    }

  // Id (not shown, but useful to retrieve the id of a preset from
  // a cell position

  char buffer[256];
  sprintf(buffer, "%03d", node->Id);
  list->SetCellText(row, VTK_KW_WLPS_ID_COL, buffer);

  if (node->Thumbnail)
    {
    list->SetCellWindowCommand(
      row, VTK_KW_WLPS_ICON_COL, this, "PresetCellIconCallback");
    list->SetCellWindowDestroyCommandToRemoveChild(row, VTK_KW_WLPS_ICON_COL);
    }
  list->SetCellTextAsDouble(
    row, VTK_KW_WLPS_ICON_COL, (double)node->CreationTime);

  list->SetCellTextAsDouble(row, VTK_KW_WLPS_WINDOW_COL, node->Window);
  list->SetCellTextAsDouble(row, VTK_KW_WLPS_LEVEL_COL, node->Level);
  list->SetCellText(row, VTK_KW_WLPS_COMMENT_COL, node->Comment.c_str());
}

//---------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetCellIconCallback(
  const char *, int row, int, const char *widget)
{
  if (!this->Internals || !this->PresetList || !widget)
    {
    return;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  int id = list->GetCellTextAsInt(row, VTK_KW_WLPS_ID_COL);

  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    if ((*it)->Thumbnail)
      {
      vtkKWLabel *child = vtkKWLabel::New();
      child->SetWidgetName(widget);
      child->SetParent(list);

      if ((*it)->Screenshot)
        {
        // Create out own balloon help manager for this one, so that
        // we can set a much shorter delay
        vtkKWBalloonHelpManager *mgr = vtkKWBalloonHelpManager::New();
        mgr->SetApplication(list->GetApplication());
        child->SetBalloonHelpManager(mgr);
        mgr->SetDelay(10);
        mgr->Delete();
        child->SetBalloonHelpIcon((*it)->Screenshot);
        }

      child->Create(list->GetApplication());
      child->SetImageToIcon((*it)->Thumbnail);
      child->SetBorderWidth(0);
      child->SetHighlightThickness(0);

      list->AddBindingsToWidget(child);
      child->Delete();
      }
    }
}

//---------------------------------------------------------------------------
const char* vtkKWWindowLevelPresetSelector::PresetCellEditStartCallback(
  const char *, int row, int col, const char *text)
{
  if (!this->Internals || !this->PresetList)
    {
    return NULL;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  int id = list->GetCellTextAsInt(row, VTK_KW_WLPS_ID_COL);

  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    if (col != VTK_KW_WLPS_COMMENT_COL)
      {
      this->InvokeApplyWindowLevelPresetCommand(id);
      list->CancelEditing();
      }
    }
  return text;
}

//---------------------------------------------------------------------------
const char* vtkKWWindowLevelPresetSelector::PresetCellEditEndCallback(
  const char *, int row, int col, const char *text)
{
  if (!this->Internals || !this->PresetList)
    {
    return NULL;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  int id = list->GetCellTextAsInt(row, VTK_KW_WLPS_ID_COL);

  vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    if (col == VTK_KW_WLPS_COMMENT_COL)
      {
      this->SetWindowLevelPresetComment(id, text);
      }
    }
  return text;
}

//---------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetAddCallback()
{
  this->InvokeAddWindowLevelPresetCommand();
}

//---------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetRemoveCallback()
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();

    // First collect the indices of the presets to remove

    int *indices = new int [list->GetNumberOfRows()];
    int *ids = new int [list->GetNumberOfRows()];

    int nb_selected_rows = list->GetSelectedRows(indices);
    int i;
    for (i = 0; i < nb_selected_rows; i++)
      {
      ids[i] = list->GetCellTextAsInt(indices[i], VTK_KW_WLPS_ID_COL);
      }

    // Then remove them

    for (i = 0; i < nb_selected_rows; i++)
      {
      this->InvokeRemoveWindowLevelPresetCommand(ids[i]); // first
      this->RemoveWindowLevelPreset(ids[i]);
      }
    
    delete [] indices;
    delete [] ids;
    }
}

//---------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetSelectionCallback()
{
  this->Update(); // this enable/disable the remove button if no selection

  if (this->ApplyPresetOnSingleClick)
    {
    if (this->PresetList)
      {
      vtkKWMultiColumnList *list = this->PresetList->GetWidget();
      if (list->GetNumberOfSelectedRows())
        {
        int id = list->GetCellTextAsInt(
          list->GetIndexOfFirstSelectedRow(), VTK_KW_WLPS_ID_COL);
        vtkKWWindowLevelPresetSelectorInternals::PoolIterator it = 
          this->Internals->GetPoolNode(id);
        if (it != this->Internals->Pool.end())
          {
          this->InvokeApplyWindowLevelPresetCommand(id);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetSelectAndApplyPreviousCallback()
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();
    int nb_rows = list->GetNumberOfRows();
    if (nb_rows)
      {
      int prev_row; 
      if (!list->GetNumberOfSelectedRows())
        {
        prev_row = nb_rows - 1;
        }
      else
        {
        int sel_row = list->GetIndexOfFirstSelectedRow();
        prev_row = (nb_rows == 1 || sel_row == 0) ? nb_rows - 1 : sel_row - 1;
        }
      list->SelectSingleRow(prev_row);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetSelectAndApplyNextCallback()
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();
    int nb_rows = list->GetNumberOfRows();
    if (nb_rows)
      {
      int next_row; 
      if (!list->GetNumberOfSelectedRows())
        {
        next_row = 0;
        }
      else
        {
        int sel_row = list->GetIndexOfFirstSelectedRow();
        next_row = (nb_rows == 1 || sel_row == nb_rows - 1) ? 0 : sel_row + 1;
        }
      list->SelectSingleRow(next_row);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetAddWindowLevelPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->AddWindowLevelPresetCommand, object, method);
  this->Update(); // this show/hide the add button
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::InvokeAddWindowLevelPresetCommand()
{
  if (this->AddWindowLevelPresetCommand && 
      *this->AddWindowLevelPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s", this->AddWindowLevelPresetCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetApplyWindowLevelPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ApplyWindowLevelPresetCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::InvokeApplyWindowLevelPresetCommand(
  int id)
{
  if (this->ApplyWindowLevelPresetCommand && 
      *this->ApplyWindowLevelPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->ApplyWindowLevelPresetCommand, id);
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetRemoveWindowLevelPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->RemoveWindowLevelPresetCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::InvokeRemoveWindowLevelPresetCommand(
  int id)
{
  if (this->RemoveWindowLevelPresetCommand && 
      *this->RemoveWindowLevelPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->RemoveWindowLevelPresetCommand, id);
    }
}

//---------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::Update()
{
  this->UpdateEnableState();

  if (this->PresetButtons)
    {
    this->PresetButtons->SetWidgetVisibility(
      VTK_KW_WLPS_BUTTON_ADD_ID, 
      (this->AddWindowLevelPresetCommand && *this->AddWindowLevelPresetCommand)
      ? 1 : 0);

    int has_selection = 
      (this->PresetList && 
       this->PresetList->GetWidget()->GetNumberOfSelectedCells());
    
    this->PresetButtons->GetWidget(
      VTK_KW_WLPS_BUTTON_REMOVE_ID)->SetEnabled(
        has_selection ? this->PresetButtons->GetEnabled() : 0);
    }

  if (this->PresetSpinButtons)
    {
    if (!this->GetNumberOfVisibleWindowLevelPresets())
      {
      this->PresetSpinButtons->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PresetList)
    {
    this->PresetList->SetEnabled(this->GetEnabled());
    }

  if (this->ControlFrame)
    {
    this->ControlFrame->SetEnabled(this->GetEnabled());
    }

  if (this->PresetSpinButtons)
    {
    this->PresetSpinButtons->SetEnabled(this->GetEnabled());
    }

  if (this->PresetButtons)
    {
    this->PresetButtons->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
