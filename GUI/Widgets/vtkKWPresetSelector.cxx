/*=========================================================================

  Module:    vtkKWPresetSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWPresetSelector.h"

#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkKWBalloonHelpManager.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWSpinButtons.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWindowToImageFilter.h"
#include "vtkKWApplication.h"
#include "vtkImagePermute.h"
#include "vtkImageClip.h"
#include "vtkKWIcon.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/vector>
#include <vtksys/stl/string>
#include <vtksys/stl/map>

#include <time.h>

#define VTK_KW_WLPS_BUTTON_ADD_ID    0
#define VTK_KW_WLPS_BUTTON_REMOVE_ID 1
#define VTK_KW_WLPS_BUTTON_UPDATE_ID 2

const char *vtkKWPresetSelector::IdColumnName      = "Id";
const char *vtkKWPresetSelector::ImageColumnName   = "Image";
const char *vtkKWPresetSelector::GroupColumnName   = "Group";
const char *vtkKWPresetSelector::CommentColumnName = "Comment";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWPresetSelector);
vtkCxxRevisionMacro(vtkKWPresetSelector, "1.2");

//----------------------------------------------------------------------------
class vtkKWPresetSelectorInternals
{
public:
  
  class UserSlotType
  {
  public:
    double DoubleValue;
    int IntValue;
    vtksys_stl::string StringValue;
    void *PointerValue;
  };

  typedef vtksys_stl::map<vtksys_stl::string, UserSlotType> UserSlotPoolType;
  typedef vtksys_stl::map<vtksys_stl::string, UserSlotType>::iterator UserSlotPoolIterator;

  class PresetNode
  {
  public:
    int Id;
    vtksys_stl::string Group;
    vtksys_stl::string Comment;
    vtksys_stl::string FileName;
    double CreationTime;
    vtkKWIcon *Thumbnail;
    vtkKWIcon *Screenshot;

    UserSlotPoolType UserSlotPool;

    PresetNode();
    ~PresetNode();
  };

  static int PresetNodeCounter;

  typedef vtksys_stl::vector<PresetNode*> PresetPoolType;
  typedef vtksys_stl::vector<PresetNode*>::iterator PresetPoolIterator;

  PresetPoolType PresetPool;

  PresetPoolIterator GetPresetNode(int id);
};

int vtkKWPresetSelectorInternals::PresetNodeCounter = 0;

//---------------------------------------------------------------------------
vtkKWPresetSelectorInternals::PresetPoolIterator 
vtkKWPresetSelectorInternals::GetPresetNode(int id)
{
  vtkKWPresetSelectorInternals::PresetPoolIterator it = 
    this->PresetPool.begin();
  vtkKWPresetSelectorInternals::PresetPoolIterator end = 
    this->PresetPool.end();
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
vtkKWPresetSelectorInternals::PresetNode::PresetNode()
{
  this->Thumbnail = NULL;
  this->Screenshot = NULL;
}

//---------------------------------------------------------------------------
vtkKWPresetSelectorInternals::PresetNode::~PresetNode()
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
vtkKWPresetSelector::vtkKWPresetSelector()
{
  this->Internals = new vtkKWPresetSelectorInternals;

  this->AddPresetCommand    = NULL;
  this->UpdatePresetCommand    = NULL;
  this->ApplyPresetCommand  = NULL;
  this->RemovePresetCommand = NULL;
  this->PresetHasChangedCommand    = NULL;

  this->PresetList        = NULL;
  this->PresetControlFrame      = NULL;
  this->PresetSelectSpinButtons = NULL;
  this->PresetButtons     = NULL;

  this->ApplyPresetOnSelection = 1;
  this->SelectSpinButtonsVisibility = 1;

  this->ThumbnailSize = 32;
  this->ScreenshotSize = 144;
  this->PromptBeforeRemovePreset = 0;

  this->GroupFilter = NULL;
}

//----------------------------------------------------------------------------
vtkKWPresetSelector::~vtkKWPresetSelector()
{
  if (this->PresetList)
    {
    this->PresetList->Delete();
    this->PresetList = NULL;
    }

  if (this->PresetControlFrame)
    {
    this->PresetControlFrame->Delete();
    this->PresetControlFrame = NULL;
    }

  if (this->PresetSelectSpinButtons)
    {
    this->PresetSelectSpinButtons->Delete();
    this->PresetSelectSpinButtons = NULL;
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

  if (this->UpdatePresetCommand)
    {
    delete [] this->UpdatePresetCommand;
    this->UpdatePresetCommand = NULL;
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

  if (this->PresetHasChangedCommand)
    {
    delete [] this->PresetHasChangedCommand;
    this->PresetHasChangedCommand = NULL;
    }

  // Remove all presets

  this->RemoveAllPresets();

  // Delete our pool

  delete this->Internals;
  this->Internals = NULL;

  this->SetGroupFilter(NULL);
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::Create(vtkKWApplication *app)
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
  if (this->ApplyPresetOnSelection)
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
  list->SetCellUpdatedCommand(this, "PresetCellUpdatedCallback");

  this->CreateColumns();

  // --------------------------------------------------------------
  // Preset : control frame

  if (!this->PresetControlFrame)
    {
    this->PresetControlFrame = vtkKWFrame::New();
    }

  this->PresetControlFrame->SetParent(this);
  this->PresetControlFrame->Create(app);

  this->Script("pack %s -side top -anchor nw -fill both -expand t",
               this->PresetControlFrame->GetWidgetName());

  // --------------------------------------------------------------
  // Preset : spin buttons

  if (!this->PresetSelectSpinButtons)
    {
    this->PresetSelectSpinButtons = vtkKWSpinButtons::New();
    }

  this->PresetSelectSpinButtons->SetParent(this->PresetControlFrame);
  this->PresetSelectSpinButtons->Create(app);
  this->PresetSelectSpinButtons->SetLayoutOrientationToHorizontal();
  this->PresetSelectSpinButtons->SetArrowOrientationToVertical();
  this->PresetSelectSpinButtons->SetButtonsPadX(2);
  this->PresetSelectSpinButtons->SetButtonsPadY(2);
  this->PresetSelectSpinButtons->GetPreviousButton()->SetBalloonHelpString(
    "Select and apply previous preset");
  this->PresetSelectSpinButtons->GetNextButton()->SetBalloonHelpString(
    "Select and apply next preset");
  this->PresetSelectSpinButtons->SetPreviousCommand(
    this, "PresetSelectPreviousCallback");
  this->PresetSelectSpinButtons->SetNextCommand(
    this, "PresetSelectNextCallback");

  // --------------------------------------------------------------
  // Preset : buttons

  if (!this->PresetButtons)
    {
    this->PresetButtons = vtkKWPushButtonSet::New();
    }

  this->PresetButtons->SetParent(this->PresetControlFrame);
  this->PresetButtons->PackHorizontallyOn();
  this->PresetButtons->SetWidgetsPadX(2);
  this->PresetButtons->SetWidgetsPadY(2);
  this->PresetButtons->SetWidgetsInternalPadY(1);
  this->PresetButtons->ExpandWidgetsOn();
  this->PresetButtons->Create(app);

  vtkKWPushButton *pb = NULL;

  // add preset

  pb = this->PresetButtons->AddWidget(VTK_KW_WLPS_BUTTON_ADD_ID);
  pb->SetImageToPredefinedIcon(vtkKWIcon::IconDocument);
  pb->SetCommand(this, "PresetAddCallback");
  pb->SetBalloonHelpString("Add a preset");

  // update preset

  pb = this->PresetButtons->AddWidget(VTK_KW_WLPS_BUTTON_UPDATE_ID);
  pb->SetImageToPredefinedIcon(vtkKWIcon::IconPointFinger);
  pb->SetCommand(this, "PresetUpdateCallback");
  pb->SetBalloonHelpString("Update the selected preset(s)");

  // remove preset

  pb = this->PresetButtons->AddWidget(VTK_KW_WLPS_BUTTON_REMOVE_ID);
  pb->SetImageToPredefinedIcon(vtkKWIcon::IconTrashcan);
  pb->SetCommand(this, "PresetRemoveCallback");
  pb->SetBalloonHelpString("Remove the selected preset(s)");

  // Pack

  this->Pack();

  // Update enable state

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->PresetControlFrame)
    {
    this->PresetControlFrame->UnpackChildren();
    }

  if (this->PresetSelectSpinButtons && 
      this->PresetSelectSpinButtons->IsCreated() &&
      this->SelectSpinButtonsVisibility)
    {
    this->Script("pack %s -side left -anchor nw -fill both -expand t",
                 this->PresetSelectSpinButtons->GetWidgetName());
    }

  if (this->PresetButtons && this->PresetButtons->IsCreated())
    {
    this->Script("pack %s -side left -anchor nw -fill x -expand t",
                 this->PresetButtons->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::CreateColumns()
{
  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  int col;

  // We need that column to retrieve the Id

  col = list->AddColumn(vtkKWPresetSelector::IdColumnName);
  list->SetColumnName(col, vtkKWPresetSelector::IdColumnName);
  list->ColumnVisibilityOff(col);

  col = list->AddColumn(vtkKWPresetSelector::ImageColumnName);
  list->SetColumnName(col, vtkKWPresetSelector::ImageColumnName);
  list->SetColumnWidth(col, -this->ThumbnailSize);
  list->SetColumnResizable(col, 0);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 0);
  list->SetColumnSortModeToReal(col);
  list->SetColumnFormatCommandToEmptyOutput(col);

  col = list->AddColumn(vtkKWPresetSelector::GroupColumnName);
  list->SetColumnName(col, vtkKWPresetSelector::GroupColumnName);
  list->SetColumnWidth(col, 5);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 0);
  list->ColumnVisibilityOff(col);

  col = list->AddColumn(vtkKWPresetSelector::CommentColumnName);
  list->SetColumnName(col, vtkKWPresetSelector::CommentColumnName);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 1);
  list->SetColumnEditable(col, 1);
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetIdColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWPresetSelector::IdColumnName) : -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetImageColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWPresetSelector::ImageColumnName) : -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetGroupColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWPresetSelector::GroupColumnName) : -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetCommentColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWPresetSelector::CommentColumnName) : -1;
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetApplyPresetOnSelection(int arg)
{
  if (this->ApplyPresetOnSelection == arg)
    {
    return;
    }

  this->ApplyPresetOnSelection = arg;
  this->Modified();

  if (this->PresetList)
    {
    if (this->ApplyPresetOnSelection)
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
void vtkKWPresetSelector::SetSelectSpinButtonsVisibility(int arg)
{
  if (this->SelectSpinButtonsVisibility == arg)
    {
    return;
    }

  this->SelectSpinButtonsVisibility = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetListHeight(int h)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetHeight(h);
    }
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetListHeight()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetHeight();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetImageColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      this->GetImageColumnIndex(), arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetImageColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      this->GetImageColumnIndex());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetCommentColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      this->GetCommentColumnIndex(), arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetCommentColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      this->GetCommentColumnIndex());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetGroupColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      this->GetGroupColumnIndex(), arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetGroupColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      this->GetGroupColumnIndex());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::AddPreset()
{
  if (!this->Internals)
    {
    return -1;
    }

  int id =  vtkKWPresetSelectorInternals::PresetNodeCounter++;

  vtkKWPresetSelectorInternals::PresetNode *node = 
    new vtkKWPresetSelectorInternals::PresetNode;

  node->Id = id;
  node->CreationTime = vtksys::SystemTools::GetTime();

  this->Internals->PresetPool.push_back(node);
  
  this->UpdatePresetRow(id);

  if (this->PresetList)
    {
    int row = this->GetPresetRow(id);
    if (row >= 0)
      {
      this->PresetList->GetWidget()->SeeRow(row);
      }
    }

  this->NumberOfPresetsHasChanged();

  return id;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::HasPreset(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    return (it != this->Internals->PresetPool.end()) ? 1 : 0;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetGroup(int id, const char *group)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->Group = group ? group : "";
      this->UpdatePresetRow(id);
      this->Update(); // the number of visible widgets may have changed
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWPresetSelector::GetPresetGroup(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      return (*it)->Group.c_str();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetComment(int id, const char *comment)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->Comment = comment ? comment : "";
      this->UpdatePresetRow(id);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWPresetSelector::GetPresetComment(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      return (*it)->Comment.c_str();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetFileName(int id, const char *filename)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->FileName = filename ? filename : "";
      this->UpdatePresetRow(id);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWPresetSelector::GetPresetFileName(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      return (*it)->FileName.c_str();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
double vtkKWPresetSelector::GetPresetCreationTime(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      return (*it)->CreationTime;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::HasPresetUserSlot(int id, const char *slot_name)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      vtkKWPresetSelectorInternals::UserSlotPoolIterator s_it =
        (*it)->UserSlotPool.find(slot_name);
      if (s_it != (*it)->UserSlotPool.end())
        {
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetUserSlotAsDouble(
  int id, const char *slot_name, double value)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->UserSlotPool[slot_name].DoubleValue = value;
      this->UpdatePresetRow(id);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
double vtkKWPresetSelector::GetPresetUserSlotAsDouble(
  int id, const char *slot_name)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      vtkKWPresetSelectorInternals::UserSlotPoolIterator s_it =
        (*it)->UserSlotPool.find(slot_name);
      if (s_it != (*it)->UserSlotPool.end())
        {
        return s_it->second.DoubleValue;
        }
      }
    }

  return 0.0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetUserSlotAsInt(
  int id, const char *slot_name, int value)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->UserSlotPool[slot_name].IntValue = value;
      this->UpdatePresetRow(id);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetPresetUserSlotAsInt(
  int id, const char *slot_name)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      vtkKWPresetSelectorInternals::UserSlotPoolIterator s_it =
        (*it)->UserSlotPool.find(slot_name);
      if (s_it != (*it)->UserSlotPool.end())
        {
        return s_it->second.IntValue;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetUserSlotAsString(
  int id, const char *slot_name, const char* value)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->UserSlotPool[slot_name].StringValue = value;
      this->UpdatePresetRow(id);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWPresetSelector::GetPresetUserSlotAsString(
  int id, const char *slot_name)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      vtkKWPresetSelectorInternals::UserSlotPoolIterator s_it =
        (*it)->UserSlotPool.find(slot_name);
      if (s_it != (*it)->UserSlotPool.end())
        {
        return s_it->second.StringValue.c_str();
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetUserSlotAsPointer(
  int id, const char *slot_name, void *value)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      (*it)->UserSlotPool[slot_name].PointerValue = value;
      this->UpdatePresetRow(id);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void* vtkKWPresetSelector::GetPresetUserSlotAsPointer(
  int id, const char *slot_name)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      vtkKWPresetSelectorInternals::UserSlotPoolIterator s_it =
        (*it)->UserSlotPool.find(slot_name);
      if (s_it != (*it)->UserSlotPool.end())
        {
        return s_it->second.PointerValue;
        }
      }
    }

  return NULL;
}

//---------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetImage(int id, vtkImageData *screenshot)
{
  if (!this->Internals)
    {
    return 0;
    }

  vtkKWPresetSelectorInternals::PresetPoolIterator it = 
    this->Internals->GetPresetNode(id);
  if (it == this->Internals->PresetPool.end())
    {
    return 0;
    }

  int *screenshot_dims = screenshot ? screenshot->GetDimensions() : NULL;
  if (!screenshot_dims ||
      screenshot_dims[0] == 0 || 
      screenshot_dims[1] == 0 || 
      screenshot_dims[2] == 0)
    {
    if ((*it)->Thumbnail)
      {
      (*it)->Thumbnail->Delete();
      (*it)->Thumbnail = NULL;
      }
    if ((*it)->Screenshot)
      {
      (*it)->Screenshot->Delete();
      (*it)->Screenshot = NULL;
      }
    return 1;
    }

  double factor;
  vtkImageData *resample_input;

  // First, let's make sure we are processing the screenshot as it
  // is by clipping its UpdateExtent. By doing so, we prevent our resample
  // and permute filter the process the screenshot's *whole* extent.

  vtkImageClip *clip = vtkImageClip::New();
  clip->SetInput(screenshot);
  clip->SetOutputWholeExtent(screenshot->GetUpdateExtent());
  clip->Update();

  // Permute, as a convenience

  int *clip_dims = clip->GetOutput()->GetDimensions();

  vtkImagePermute *permute = NULL;
  if (clip_dims[2] != 1)
    {
    permute = vtkImagePermute::New();
    permute->SetInput(clip->GetOutput());
    if (clip_dims[0] == 1)
      {
      permute->SetFilteredAxes(1, 2, 0);
      }
    else
      {
      permute->SetFilteredAxes(0, 2, 1);
      }
    resample_input = permute->GetOutput();
    }
  else
    {
    resample_input = clip->GetOutput();
    }

  // Create the thumbnail

  resample_input->Update();
  int *resample_input_dims = resample_input->GetDimensions();
  double *resample_input_spacing = resample_input->GetSpacing();

  int large_dim = 0, small_dim = 1;
  if (resample_input_dims[0] < resample_input_dims[1])
    {
    large_dim = 1; small_dim = 0;
    }

  vtkImageResample *resample = vtkImageResample::New();
  resample->SetInput(resample_input);
  resample->SetInterpolationModeToCubic();
  factor = 
    (double)this->ThumbnailSize / (double)resample_input_dims[large_dim];
  resample->SetAxisMagnificationFactor(large_dim, factor);
  resample->SetAxisMagnificationFactor(
    small_dim, factor * (resample_input_spacing[small_dim] / 
                         resample_input_spacing[large_dim]));
  resample->SetDimensionality(2);
  resample->Update();
  vtkImageData *resample_output = resample->GetOutput();

  if (!(*it)->Thumbnail)
    {
    (*it)->Thumbnail = vtkKWIcon::New();
    }
  (*it)->Thumbnail->SetImage(
    (const unsigned char*)resample_output->GetScalarPointer(),
    resample_output->GetDimensions()[0],
    resample_output->GetDimensions()[1],
    3,
    0,
    vtkKWIcon::ImageOptionFlipVertical);

  // Create the screenshot

  factor = 
    (double)this->ScreenshotSize / (double)resample_input_dims[large_dim];
  resample->SetAxisMagnificationFactor(large_dim, factor);
  resample->SetAxisMagnificationFactor(
    small_dim, factor * (resample_input_spacing[small_dim] / 
                         resample_input_spacing[large_dim]));
  resample->Update();
  resample_output = resample->GetOutput();

  if (!(*it)->Screenshot)
    {
    (*it)->Screenshot = vtkKWIcon::New();
    }
  (*it)->Screenshot->SetImage(
    (const unsigned char*)resample_output->GetScalarPointer(),
    resample_output->GetDimensions()[0],
    resample_output->GetDimensions()[1],
    3,
    0,
    vtkKWIcon::ImageOptionFlipVertical);

  clip->Delete();
  resample->Delete();
  if (permute)
    {
    permute->Delete();
    }

  // Update the icon cell

  this->UpdatePresetRow(id);

  return 1;
}

//---------------------------------------------------------------------------
int vtkKWPresetSelector::SetPresetImageFromRenderWindow(
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
int vtkKWPresetSelector::HasPresetImage(int id)
{
  return this->GetPresetThumbnail(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWIcon* vtkKWPresetSelector::GetPresetThumbnail(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      return (*it)->Thumbnail;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWIcon* vtkKWPresetSelector::GetPresetScreenshot(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      return (*it)->Screenshot;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetNumberOfPresets()
{
  if (this->Internals)
    {
    return this->Internals->PresetPool.size();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetNumberOfPresetsWithGroup(const char *group)
{
  int count = 0;
  if (this->Internals && group && *group)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->PresetPool.begin();
    vtkKWPresetSelectorInternals::PresetPoolIterator end = 
      this->Internals->PresetPool.end();
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
int vtkKWPresetSelector::GetNumberOfVisiblePresets()
{
  if (this->GroupFilter && *this->GroupFilter)
    {
    return this->GetNumberOfPresetsWithGroup(this->GroupFilter);
    }
  return this->GetNumberOfPresets();
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetNthPresetId(int index)
{
  if (this->Internals && index >= 0 && index < this->GetNumberOfPresets())
    {
    return this->Internals->PresetPool[index]->Id;
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetNthPresetWithGroupId(int index, const char *group)
{
  int rank = this->GetNthPresetWithGroupRank(index, group);
  if (rank >= 0)
    {
    return this->GetNthPresetId(rank);
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetPresetAtRowId(int row_index)
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();
    if (row_index >= 0 && row_index < list->GetNumberOfRows())
      {
      return list->GetCellTextAsInt(row_index, this->GetIdColumnIndex());
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetPresetRow(int id)
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->FindCellTextAsIntInColumn(
      this->GetIdColumnIndex(), id);
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::GetNthPresetWithGroupRank(
  int index, const char *group)
{
  if (this->Internals && index >= 0 && group && *group)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->PresetPool.begin();
    vtkKWPresetSelectorInternals::PresetPoolIterator end = 
      this->Internals->PresetPool.end();
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
int vtkKWPresetSelector::RemovePreset(int id)
{
  if (this->Internals)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it = 
      this->Internals->GetPresetNode(id);
    if (it != this->Internals->PresetPool.end())
      {
      if (this->PresetList)
        {
        int row = this->GetPresetRow(id);
        if (row >= 0)
          {
          this->PresetList->GetWidget()->DeleteRow(row);
          }
        }
      this->DeAllocatePreset(id);
      delete (*it);
      this->Internals->PresetPool.erase(it);
      this->NumberOfPresetsHasChanged();
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::RemoveAllPresets()
{
  // Is faster than calling RemovePreset on each preset

  if (this->PresetList)
    {
    this->PresetList->GetWidget()->DeleteAllRows();
    }
  if (this->Internals)
    {
    int nb_deleted = this->GetNumberOfPresets();
    vtkKWPresetSelectorInternals::PresetPoolIterator end = 
      this->Internals->PresetPool.end();
    vtkKWPresetSelectorInternals::PresetPoolIterator it;

    // First give a chance to third-party or subclasses to deallocate
    // the presets cleanly (without messing up the iterator framework)

    it = this->Internals->PresetPool.begin();
    for (; it != end; ++it)
      {
      this->DeAllocatePreset((*it)->Id);
      }

    // Then remove the presets

    it = this->Internals->PresetPool.begin();
    for (; it != end; ++it)
      {
      delete (*it);
      }
    this->Internals->PresetPool.clear();
    if (nb_deleted)
      {
      this->NumberOfPresetsHasChanged();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::RemoveAllPresetsWithGroup(const char *group)
{
  // Is faster than calling RemovePreset on each preset

  if (this->Internals && group && *group)
    {
    vtkKWPresetSelectorInternals::PresetPoolIterator it;
    vtkKWPresetSelectorInternals::PresetPoolIterator end = 
      this->Internals->PresetPool.end();

    // First give a chance to third-party or subclasses to deallocate
    // the presets cleanly (without messing up the iterator framework)

    it = this->Internals->PresetPool.begin();
    for (; it != end; ++it)
      {
      if (!(*it)->Group.compare(group))
        {
        if (this->PresetList)
          {
          int row = this->GetPresetRow((*it)->Id);
          if (row >= 0)
            {
            this->PresetList->GetWidget()->DeleteRow(row);
            }
          }
        this->DeAllocatePreset((*it)->Id);
        }
      }

    // Then remove the presets

    int nb_deleted = 0;
    int done = 0;
    while (!done)
      {
      done = 1;
      it = this->Internals->PresetPool.begin();
      for (; it != end; ++it)
        {
        if (!(*it)->Group.compare(group))
          {
          delete (*it);
          this->Internals->PresetPool.erase(it);
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
void vtkKWPresetSelector::NumberOfPresetsHasChanged()
{
  this->Update(); // enable/disable some buttons valid only if we have presets
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetGroupFilter(const char* _arg)
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
void vtkKWPresetSelector::UpdateRowsInPresetList()
{
  int i, nb_presets = this->GetNumberOfPresets();
  for (i = 0; i < nb_presets; i++)
    {
    this->UpdatePresetRow(this->GetNthPresetId(i));
    }
}

//----------------------------------------------------------------------------
int vtkKWPresetSelector::UpdatePresetRow(int id)
{
  if (!this->HasPreset(id))
    {
    return 0;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  // Look for this node in the list

  int row = this->GetPresetRow(id);

  const char *group = this->GetPresetGroup(id);
  
  int group_filter_exclude =
    this->GroupFilter && *this->GroupFilter && 
    strcmp(group, this->GroupFilter);

  // Not found ? Insert it, or ignore it if the group filter does not match

  if (row < 0)
    {
    if (group_filter_exclude)
      {
      return 0;
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
      return 0;
      }
    }

  // Id (not shown, but useful to retrieve the id of a preset from
  // a cell position

  char buffer[256];
  sprintf(buffer, "%03d", id);
  list->SetCellText(row, this->GetIdColumnIndex(), buffer);

  int image_col_index = this->GetImageColumnIndex();
  if (this->HasPresetImage(id))
    {
    list->SetCellWindowCommand(
      row, image_col_index, this, "PresetCellImageCallback");
    list->SetCellWindowDestroyCommandToRemoveChild(row, image_col_index);
    list->RefreshCellWithWindowCommand(row, image_col_index);
    }
  else
    {
    list->SetCellWindowCommand(row, image_col_index, NULL, NULL);
    }

  list->SetCellTextAsDouble(
    row, image_col_index, this->GetPresetCreationTime(id));

  list->SetCellText(
    row, this->GetGroupColumnIndex(), group);

  list->SetCellText(
    row, this->GetCommentColumnIndex(), this->GetPresetComment(id));

  return 1;
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::PresetCellImageCallback(
  const char *, int row, int, const char *widget)
{
  if (!this->PresetList || !widget)
    {
    return;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  int id = this->GetPresetAtRowId(row);
  if (this->HasPreset(id))
    {
    vtkKWIcon *thumbnail = this->GetPresetThumbnail(id);
    if (thumbnail)
      {
      vtkKWLabel *child = vtkKWLabel::New();
      child->SetWidgetName(widget);
      child->SetParent(list);

      vtkKWIcon *screenshot = this->GetPresetScreenshot(id);
      if (screenshot)
        {
        // Create out own balloon help manager for this one, so that
        // we can set a much shorter delay
        vtkKWBalloonHelpManager *mgr = vtkKWBalloonHelpManager::New();
        mgr->SetApplication(list->GetApplication());
        child->SetBalloonHelpManager(mgr);
        mgr->SetDelay(10);
        mgr->Delete();
        child->SetBalloonHelpIcon(screenshot);
        }

      child->Create(list->GetApplication());
      child->SetBorderWidth(0);
      child->SetHighlightThickness(0);
      child->SetWidth(this->ThumbnailSize);
      child->SetHeight(this->ThumbnailSize);
      child->SetBackgroundColor(
        list->GetCellCurrentBackgroundColor(row, this->GetImageColumnIndex()));
      child->SetImageToIcon(thumbnail);

      list->AddBindingsToWidget(child);
      child->Delete();
      }
    }
}

//---------------------------------------------------------------------------
const char* vtkKWPresetSelector::PresetCellEditStartCallback(
  int, int, const char *text)
{
  return text;
}

//---------------------------------------------------------------------------
const char* vtkKWPresetSelector::PresetCellEditEndCallback(
  int, int, const char *text)
{
  return text;
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::PresetCellUpdatedCallback(
  int row, int col, const char *text)
{
  int id = this->GetPresetAtRowId(row);
  if (this->HasPreset(id))
    {
    if (col == this->GetCommentColumnIndex())
      {
      this->SetPresetComment(id, text);
      this->InvokePresetHasChangedCommand(id);
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::PresetAddCallback()
{
  this->InvokeAddPresetCommand();
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::PresetUpdateCallback()
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();

    // First collect the indices of the presets to update
    // Then update them

    int *indices = new int [list->GetNumberOfRows()];
    int *ids = new int [list->GetNumberOfRows()];

    int i, nb_selected_rows = list->GetSelectedRows(indices);
    for (i = 0; i < nb_selected_rows; i++)
      {
      ids[i] = this->GetPresetAtRowId(indices[i]);
      }

    for (i = 0; i < nb_selected_rows; i++)
      {
      this->InvokeUpdatePresetCommand(ids[i]);
      }

    delete [] indices;
    delete [] ids;
    }
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::PresetRemoveCallback()
{
  if (this->PresetList)
    {
    vtkKWMultiColumnList *list = this->PresetList->GetWidget();

    // First collect the indices of the presets to remove

    int *indices = new int [list->GetNumberOfRows()];
    int *ids = new int [list->GetNumberOfRows()];

    int i, nb_selected_rows = list->GetSelectedRows(indices);
    for (i = 0; i < nb_selected_rows; i++)
      {
      ids[i] = this->GetPresetAtRowId(indices[i]);
      }

    // Then remove them

    if (nb_selected_rows)
      {
      if (!this->PromptBeforeRemovePreset ||
          vtkKWMessageDialog::PopupYesNo( 
            this->GetApplication(), 
            this->GetApplication()->GetNthWindow(0), 
            "Delete Preset",
            "Are you sure you want to delete this preset?", 
            vtkKWMessageDialog::WarningIcon | 
            vtkKWMessageDialog::InvokeAtPointer))
        {
        for (i = 0; i < nb_selected_rows; i++)
          {
          this->InvokeRemovePresetCommand(ids[i]); // first
          this->RemovePreset(ids[i]);
          }
        }
      }
    
    delete [] indices;
    delete [] ids;
    }
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::PresetSelectionCallback()
{
  this->Update(); // this enable/disable the remove button if no selection

  if (this->ApplyPresetOnSelection)
    {
    if (this->PresetList)
      {
      vtkKWMultiColumnList *list = this->PresetList->GetWidget();
      if (list->GetNumberOfSelectedRows())
        {
        int id = this->GetPresetAtRowId(list->GetIndexOfFirstSelectedRow());
        if (this->HasPreset(id))
          {
          this->InvokeApplyPresetCommand(id);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::PresetSelectPreviousCallback()
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
void vtkKWPresetSelector::PresetSelectNextCallback()
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
void vtkKWPresetSelector::SetAddPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->AddPresetCommand, object, method);
  this->Update(); // this show/hide the add button
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::InvokeAddPresetCommand()
{
  if (this->AddPresetCommand && 
      *this->AddPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s", this->AddPresetCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetUpdatePresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->UpdatePresetCommand, object, method);
  this->Update(); // this show/hide the update button
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::InvokeUpdatePresetCommand(int id)
{
  if (this->UpdatePresetCommand && 
      *this->UpdatePresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->UpdatePresetCommand, id);
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetApplyPresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ApplyPresetCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::InvokeApplyPresetCommand(int id)
{
  if (this->ApplyPresetCommand && 
      *this->ApplyPresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->ApplyPresetCommand, id);
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetRemovePresetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->RemovePresetCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::InvokeRemovePresetCommand(int id)
{
  if (this->RemovePresetCommand && 
      *this->RemovePresetCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->RemovePresetCommand, id);
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::SetPresetHasChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->PresetHasChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::InvokePresetHasChangedCommand(int id)
{
  if (this->PresetHasChangedCommand && 
      *this->PresetHasChangedCommand && 
      this->IsCreated())
    {
    this->Script("eval %s %d", this->PresetHasChangedCommand, id);
    }
}

//---------------------------------------------------------------------------
void vtkKWPresetSelector::Update()
{
  this->UpdateEnableState();

  if (this->PresetButtons)
    {
    this->PresetButtons->SetWidgetVisibility(
      VTK_KW_WLPS_BUTTON_ADD_ID, 
      (this->AddPresetCommand && *this->AddPresetCommand) ? 1 : 0);

    this->PresetButtons->SetWidgetVisibility(
      VTK_KW_WLPS_BUTTON_UPDATE_ID, 
      (this->UpdatePresetCommand && *this->UpdatePresetCommand) ? 1 : 0);

    int has_selection = 
      (this->PresetList && 
       this->PresetList->GetWidget()->GetNumberOfSelectedCells());
    
    this->PresetButtons->GetWidget(
      VTK_KW_WLPS_BUTTON_REMOVE_ID)->SetEnabled(
        has_selection ? this->PresetButtons->GetEnabled() : 0);

    this->PresetButtons->GetWidget(
      VTK_KW_WLPS_BUTTON_UPDATE_ID)->SetEnabled(
        has_selection ? this->PresetButtons->GetEnabled() : 0);
    }

  if (this->PresetSelectSpinButtons)
    {
    if (!this->GetNumberOfVisiblePresets())
      {
      this->PresetSelectSpinButtons->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PresetList)
    {
    this->PresetList->SetEnabled(this->GetEnabled());
    }

  if (this->PresetControlFrame)
    {
    this->PresetControlFrame->SetEnabled(this->GetEnabled());
    }

  if (this->PresetSelectSpinButtons)
    {
    this->PresetSelectSpinButtons->SetEnabled(this->GetEnabled());
    }

  if (this->PresetButtons)
    {
    this->PresetButtons->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
