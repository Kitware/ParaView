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
// .NAME vtkKWWindowLevelPresetSelector - a window level preset selector.
// .SECTION Description
// This class is a widget that can be used to store and apply window/level
// presets. 

#ifndef __vtkKWWindowLevelPresetSelector_h
#define __vtkKWWindowLevelPresetSelector_h

#include "vtkKWCompositeWidget.h"

class vtkKWWindowLevelPresetSelectorInternals;
class vtkKWMultiColumnListWithScrollbars;
class vtkKWSpinButtons;
class vtkKWPushButtonSet;
class vtkImageData;

class KWWIDGETS_EXPORT vtkKWWindowLevelPresetSelector : public vtkKWCompositeWidget
{
public:
  static vtkKWWindowLevelPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWWindowLevelPresetSelector, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Add a new window/level preset.
  // Return a unique Id for that preset.
  virtual int AddWindowLevelPreset(double window, double level);

  // Description:
  // Assign a group to a preset in the pool.
  // This provide a way of grouping window/level presets (say, if you
  // have different dataset at the same time and want to store
  // different window/level presets for each dataset).
  // Return 1 on success, 0 on error
  virtual int SetWindowLevelPresetGroup(int id, const char *group);
  virtual const char* GetWindowLevelPresetGroup(int id);

  // Description:
  // Assign a comment to a preset in the pool.
  // Return 1 on success, 0 on error
  virtual int SetWindowLevelPresetComment(int id, const char *group);
  virtual const char* GetWindowLevelPresetComment(int id);

  // Description:
  // Get the number of presets in the pool, or the number of presets with
  // the same group in the pool, or the number of visible presets, i.e.
  // the presets that are displayed despite the filters (see GroupFilter).
  virtual int GetNumberOfWindowLevelPresets();
  virtual int GetNumberOfWindowLevelPresetsWithGroup(const char *group);
  virtual int GetNumberOfVisibleWindowLevelPresets();

  // Description:
  // Query if a preset is in the pool, given a window/level, and optionally
  // a group. If no group is specified, the search is performed among
  // all presets in the pool.
  virtual int HasWindowLevelPreset(
    double window, double level);
  virtual int HasWindowLevelPresetWithGroup(
    double window, double level, const char *group);

  // Description:
  // Retrieve a window/level preset given its unique Id, or its position in
  // the pool (i.e. nth-preset), or its position in the pool within a group
  // (i.e. nth-preset with a given group).
  // Return 1 on success, 0 otherwise.
  virtual int GetWindowLevelPreset(
    int id, double *window, double *level);
  virtual int GetNthWindowLevelPreset(
    int index, double *window, double *level);
  virtual int GetNthWindowLevelPresetWithGroup(
    int index, const char *group, double *window, double *level);

  // Description:
  // Retrieve the Id of the first preset with a given window/level,
  // or given a window/level and a group, or given its position in the pool, or
  // given its position in the pool within a group (i.e. nth-preset with
  // a given group), or given its row position as currently displayed in the
  // list.
  // Return 1 on success, 0 otherwise
  virtual int GetWindowLevelPresetId(
    double window, double level, int *id);
  virtual int GetWindowLevelPresetIdWithGroup(
    double window, double level, const char *group, int *id);
  virtual int GetNthWindowLevelPresetId(
    int index, int *id);
  virtual int GetNthWindowLevelPresetIdWithGroup(
    int index, const char *group, int *id);
  virtual int GetNthVisibleWindowLevelPresetId(
    int row_index, int *id);

  // Description:
  // Retrieve the rank in the whole pool of the nth preset in the pool within
  // a group (i.e. nth-preset with a given group).
  // Return 1 on success, 0 otherwise
  virtual int GetNthWindowLevelPresetRankWithGroup(
    int index, const char *group, int *rank);

  // Description:
  // Remove a window/level preset, or all of them, or all of the presets
  // with the same group.
  // Return 1 on success, 0 on error
  virtual int RemoveWindowLevelPreset(int id);
  virtual int RemoveAllWindowLevelPresets();
  virtual int RemoveAllWindowLevelPresetsWithGroup(const char *group);

  // Description:
  // Set/Get the group filter.
  // If set to a non-empty value, only the presets have the same group
  // as the filter will be displayed.
  virtual void SetGroupFilter(const char *group);
  vtkGetStringMacro(GroupFilter);

  // Description:
  // Set/Get the list height (in number of items)
  // No effect if called before Create().
  virtual void SetListHeight(int);
  virtual int GetListHeight();

  // Description:
  // Set/Get the visibility of the image column.
  // No effect if called before Create().
  virtual void SetImageColumnVisibility(int);
  virtual int GetImageColumnVisibility();
  vtkBooleanMacro(ImageColumnVisibility, int);

  // Description:
  // Set/Get the visibility of the comment column.
  // No effect if called before Create().
  virtual void SetCommentColumnVisibility(int);
  virtual int GetCommentColumnVisibility();
  vtkBooleanMacro(CommentColumnVisibility, int);

  // Description:
  // Assign an image to a preset in the pool.
  // It will be used to generate both a thumbnail view of the
  // preset and a popup mini-screenshot.
  virtual int SetWindowLevelPresetImage(int id, vtkImageData *img);
  virtual int HasWindowLevelPresetImage(int id);

  // Description:
  // Set/Get the thumbnail size.
  // Changing the size will not resize the current thumbnails, but will
  // affect the presets inserted later on.
  vtkSetClampMacro(ThumbnailSize,int,8,512);
  vtkGetMacro(ThumbnailSize,int);

  // Description:
  // Set/Get the screenshot size (i.e. the image that appears as
  // a popup when the mouse is on top of the thumbnail).
  vtkSetClampMacro(ScreenshotSize,int,8,2048);
  vtkGetMacro(ScreenshotSize,int);

  // Description:
  // Specifies a command to be invoked when the "add" button is pressed.
  // This is used by the application to actually check which window/level
  // is used on whichever dataset is loaded or selected, and call back
  // this object to add this window/level as a preset (using the
  // AddWindowLevelPreset method for example).
  // If it is not set, the 'add' button is not visible.
  virtual void SetAddWindowLevelPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when the the user tries to
  // apply a window/level preset (by double-clicking on the preset for
  // example). The id of the preset is passed to the command.
  virtual void SetApplyWindowLevelPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Set/Get if a preset should be applied when it is selected (single-click),
  // or only when it is double-clicked on.
  // If set, only one preset can be selected at a time (if not, multiple
  // preset can be selected, and removed for example).
  virtual void SetApplyPresetOnSelectionChanged(int);
  vtkGetMacro(ApplyPresetOnSelectionChanged,int);
  vtkBooleanMacro(ApplyPresetOnSelectionChanged,int);

  // Description:
  // Specifies a command to be invoked when the the user tries to
  // apply a window/level preset (by double-clicking on the preset for
  // example). The id of the preset is passed to the command.
  virtual void SetRemoveWindowLevelPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Refresh the interface.
  virtual void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks
  virtual void PresetAddCallback();
  virtual void PresetRemoveCallback();
  virtual void PresetCellIconCallback(const char*, int, int, const char*);
  virtual const char* PresetCellEditStartCallback(
    const char*, int, int, const char*);
  virtual const char* PresetCellEditEndCallback(
    const char*, int, int, const char*);
  virtual void PresetSelectionChangedCallback();
  virtual void PresetSelectAndApplyPreviousCallback();
  virtual void PresetSelectAndApplyNextCallback();

protected:
  vtkKWWindowLevelPresetSelector();
  ~vtkKWWindowLevelPresetSelector();

  vtkKWMultiColumnListWithScrollbars *PresetList;
  vtkKWFrame                         *ControlFrame;
  vtkKWSpinButtons                   *PresetSpinButtons;
  vtkKWPushButtonSet                 *PresetButtons;

  int ApplyPresetOnSelectionChanged;

  int ThumbnailSize;
  int ScreenshotSize;

  char* GroupFilter;

  // Description:
  // Called when the number of presets has changed
  virtual void NumberOfWindowLevelPresetsHasChanged();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWWindowLevelPresetSelectorInternals *Internals;
  //ETX

  // Description:
  // Update one or all rows in the list
  virtual void UpdateRowsInPresetList();
  virtual void UpdateRowInPresetList(void*);

  char *AddWindowLevelPresetCommand;
  virtual void InvokeAddWindowLevelPresetCommand();

  char *ApplyWindowLevelPresetCommand;
  virtual void InvokeApplyWindowLevelPresetCommand(int id);

  char *RemoveWindowLevelPresetCommand;
  virtual void InvokeRemoveWindowLevelPresetCommand(int id);

private:

  vtkKWWindowLevelPresetSelector(const vtkKWWindowLevelPresetSelector&); // Not implemented
  void operator=(const vtkKWWindowLevelPresetSelector&); // Not implemented
};

#endif
