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

#include "vtkKWAbstractPresetSelector.h"

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
#define VTK_KW_WLPS_COMMENT_COL    4

#define VTK_KW_WLPS_BUTTON_ADD_ID    0
#define VTK_KW_WLPS_BUTTON_REMOVE_ID 1

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWAbstractPresetSelector);
vtkCxxRevisionMacro(vtkKWAbstractPresetSelector, "1.1");

//----------------------------------------------------------------------------
class vtkKWAbstractPresetSelectorInternals
{
public:
  struct PoolNode
  {
    int Id;
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

int vtkKWAbstractPresetSelectorInternals::PoolNodeCounter = 0;

//---------------------------------------------------------------------------
vtkKWAbstractPresetSelectorInternals::PoolIterator 
vtkKWAbstractPresetSelectorInternals::GetPoolNode(int id)
{
  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
    this->Pool.begin();
  vtkKWAbstractPresetSelectorInternals::PoolIterator end = 
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
vtkKWAbstractPresetSelectorInternals::PoolNode::PoolNode()
{
  this->Thumbnail = NULL;
  this->Screenshot = NULL;
}

//---------------------------------------------------------------------------
vtkKWAbstractPresetSelectorInternals::PoolNode::~PoolNode()
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
vtkKWAbstractPresetSelector::vtkKWAbstractPresetSelector()
{
  this->Internals = new vtkKWAbstractPresetSelectorInternals;

  this->AddPresetCommand    = NULL;
  this->ApplyPresetCommand  = NULL;
  this->RemovePresetCommand = NULL;

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
vtkKWAbstractPresetSelector::~vtkKWAbstractPresetSelector()
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

  if (this->AddPresetCommand)
    {
    delete [] this->AddPresetCommand;
    this->AddPresetCommand = NULL;
    }

  if (this->ApplyPresetCommand)
    {
    delete [] this->ApplyPresetCommand;
    this->ApplyPresetCommand = NULL;
    }

  if (this->RemovePresetCommand)
    {
    delete [] this->RemovePresetCommand;
    this->RemovePresetCommand = NULL;
    }

  // Remove all presets

  this->RemoveAllPresets();

  // Delete our pool

  delete this->Internals;
  this->Internals = NULL;

  this->SetGroupFilter(NULL);
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::Create(vtkKWApplication *app)
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

  // ask for W/L

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
  pb->SetBalloonHelpString("Add a preset");

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
void vtkKWAbstractPresetSelector::SetApplyPresetOnSingleClick(int arg)
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
void vtkKWAbstractPresetSelector::SetListHeight(int h)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetHeight(h);
    }
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetListHeight()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetHeight();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::SetImageColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      VTK_KW_WLPS_ICON_COL, arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetImageColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      VTK_KW_WLPS_ICON_COL);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::SetCommentColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      VTK_KW_WLPS_COMMENT_COL, arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetCommentColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      VTK_KW_WLPS_COMMENT_COL);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::SetPresetGroup(
  int id, const char *group)
{
  // Update the group

  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
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
const char* vtkKWAbstractPresetSelector::GetPresetGroup(
  int id)
{
  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    return (*it)->Group.c_str();
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::SetPresetComment(
  int id, const char *group)
{
  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
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
const char* vtkKWAbstractPresetSelector::GetPresetComment(
  int id)
{
  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    return (*it)->Comment.c_str();
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetNumberOfPresets()
{
  return this->Internals->Pool.size();
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetNumberOfPresetsWithGroup(
  const char *group)
{
  int count = 0;
  if (group && *group)
    {
    vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWAbstractPresetSelectorInternals::PoolIterator end = 
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
int vtkKWAbstractPresetSelector::GetNumberOfVisiblePresets()
{
  if (this->GroupFilter && *this->GroupFilter)
    {
    return this->GetNumberOfPresetsWithGroup(this->GroupFilter);
    }
  return this->GetNumberOfPresets();
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetNthPresetId(int index)
{
  if (index >= 0 && index < this->GetNumberOfPresets())
    {
    return this->Internals->Pool[index]->Id;
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetNthPresetIdWithGroup(
  int index, const char *group)
{
  int rank = this->GetNthPresetRankWithGroup(index, group);
  if (rank >= 0)
    {
    return this->GetNthPresetId(rank);
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::GetNthVisiblePresetId(
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
int vtkKWAbstractPresetSelector::GetNthPresetRankWithGroup(
  int index, const char *group)
{
  if (index >= 0 && group && *group)
    {
    vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWAbstractPresetSelectorInternals::PoolIterator end = 
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
int vtkKWAbstractPresetSelector::RemovePreset(
  int id)
{
  if (this->Internals)
    {
    vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
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
      this->NumberOfPresetsHasChanged();
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::RemoveAllPresets()
{
  // Is faster than calling RemovePreset on each preset

  if (this->PresetList)
    {
    this->PresetList->GetWidget()->DeleteAllRows();
    }
  if (this->Internals)
    {
    int nb_deleted = this->GetNumberOfPresets();
    vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWAbstractPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      delete (*it);
      }
    this->Internals->Pool.clear();
    vtkKWAbstractPresetSelectorInternals::PoolNodeCounter = 0;
    if (nb_deleted)
      {
      this->NumberOfPresetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::RemoveAllPresetsWithGroup(
  const char *group)
{
  // Is faster than calling RemovePreset on each preset

  if (this->Internals && group && *group)
    {
    int nb_deleted = 0;
    int done = 0;
    while (!done)
      {
      done = 1;
      vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
        this->Internals->Pool.begin();
      vtkKWAbstractPresetSelectorInternals::PoolIterator end = 
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
      this->NumberOfPresetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::NumberOfPresetsHasChanged()
{
  this->Update(); // enable/disable some buttons valid only if we have presets
}

//---------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::SetPresetImage(
  int id, vtkImageData *screenshot)
{
  if (!this->Internals || !screenshot)
    {
    return 0;
    }

  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
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
int vtkKWAbstractPresetSelector::SetPresetImageFromRenderWindow(
  int id, vtkRenderWindow *win)
{
  if (win)
    {
    vtkWindowToImageFilter *filter = vtkWindowToImageFilter::New();
    filter->ShouldRerenderOff();
    filter->SetInput(win);
    filter->Update();
    int res = this->SetPresetImage(id, filter->GetOutput());
    filter->Delete();
    return res;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkKWAbstractPresetSelector::HasPresetImage(int id)
{
  if (this->Internals)
    {
    vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
      this->Internals->GetPoolNode(id);
    return (it != this->Internals->Pool.end() && (*it)->Thumbnail) ? 1 : 0;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::SetGroupFilter(const char* _arg)
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
void vtkKWAbstractPresetSelector::UpdateRowsInPresetList()
{
  if (this->Internals)
    {
    vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWAbstractPresetSelectorInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      this->UpdateRowInPresetList(*it);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::UpdateRowInPresetList(void *ptr)
{
  if (!ptr || !this->PresetList)
    {
    return;
    }

  vtkKWAbstractPresetSelectorInternals::PoolNode *node = 
    static_cast<vtkKWAbstractPresetSelectorInternals::PoolNode*>(ptr);

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

  list->SetCellText(row, VTK_KW_WLPS_COMMENT_COL, node->Comment.c_str());
}

//---------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::PresetCellIconCallback(
  const char *, int row, int, const char *widget)
{
  if (!this->Internals || !this->PresetList || !widget)
    {
    return;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  int id = list->GetCellTextAsInt(row, VTK_KW_WLPS_ID_COL);

  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
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
const char* vtkKWAbstractPresetSelector::PresetCellEditStartCallback(
  const char *, int row, int col, const char *text)
{
  if (!this->Internals || !this->PresetList)
    {
    return NULL;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  int id = list->GetCellTextAsInt(row, VTK_KW_WLPS_ID_COL);

  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    if (col != VTK_KW_WLPS_COMMENT_COL)
      {
      this->InvokeApplyPresetCommand(id);
      list->CancelEditing();
      }
    }
  return text;
}

//---------------------------------------------------------------------------
const char* vtkKWAbstractPresetSelector::PresetCellEditEndCallback(
  const char *, int row, int col, const char *text)
{
  if (!this->Internals || !this->PresetList)
    {
    return NULL;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();
  int id = list->GetCellTextAsInt(row, VTK_KW_WLPS_ID_COL);

  vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
    this->Internals->GetPoolNode(id);
  if (it != this->Internals->Pool.end())
    {
    if (col == VTK_KW_WLPS_COMMENT_COL)
      {
      this->SetPresetComment(id, text);
      }
    }
  return text;
}

//---------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::PresetAddCallback()
{
  this->InvokeAddPresetCommand();
}

//---------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::PresetRemoveCallback()
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
      this->InvokeRemovePresetCommand(ids[i]); // first
      this->RemovePreset(ids[i]);
      }
    
    delete [] indices;
    delete [] ids;
    }
}

//---------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::PresetSelectionCallback()
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
        vtkKWAbstractPresetSelectorInternals::PoolIterator it = 
          this->Internals->GetPoolNode(id);
        if (it != this->Internals->Pool.end())
          {
          this->InvokeApplyPresetCommand(id);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::PresetSelectAndApplyPreviousCallback()
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
void vtkKWAbstractPresetSelector::PresetSelectAndApplyNextCallback()
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
void vtkKWAbstractPresetSelector::SetAddPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->AddPresetCommand, object, method);
  this->Update(); // this show/hide the add button
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::InvokeAddPresetCommand()
{
  if (this->AddPresetCommand && 
      *this->AddPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s", this->AddPresetCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::SetApplyPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ApplyPresetCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::InvokeApplyPresetCommand(
  int id)
{
  if (this->ApplyPresetCommand && 
      *this->ApplyPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->ApplyPresetCommand, id);
    }
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::SetRemovePresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->RemovePresetCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::InvokeRemovePresetCommand(
  int id)
{
  if (this->RemovePresetCommand && 
      *this->RemovePresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->RemovePresetCommand, id);
    }
}

//---------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::Update()
{
  this->UpdateEnableState();

  if (this->PresetButtons)
    {
    this->PresetButtons->SetWidgetVisibility(
      VTK_KW_WLPS_BUTTON_ADD_ID, 
      (this->AddPresetCommand && *this->AddPresetCommand)
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
    if (!this->GetNumberOfVisiblePresets())
      {
      this->PresetSpinButtons->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWAbstractPresetSelector::UpdateEnableState()
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
void vtkKWAbstractPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
