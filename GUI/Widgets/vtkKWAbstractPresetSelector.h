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
// .NAME vtkKWAbstractPresetSelector - an abstract preset selector.
// .SECTION Description
// This class is superclass for all widgets that can be used to store
// and apply presets. IN PROGRESS. DO NOT USE.

#ifndef __vtkKWAbstractPresetSelector_h
#define __vtkKWAbstractPresetSelector_h

#include "vtkKWCompositeWidget.h"

class vtkKWAbstractPresetSelectorInternals;
class vtkKWMultiColumnListWithScrollbars;
class vtkKWSpinButtons;
class vtkKWPushButtonSet;
class vtkImageData;
class vtkRenderWindow;

class KWWIDGETS_EXPORT vtkKWAbstractPresetSelector : public vtkKWCompositeWidget
{
public:
  static vtkKWAbstractPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWAbstractPresetSelector, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Assign a group to a preset in the pool.
  // This provide a way of grouping presets (say, if you
  // have different dataset loaded at the same time and want to store
  // different presets for each dataset).
  // Return 1 on success, 0 on error
  virtual int SetPresetGroup(int id, const char *group);
  virtual const char* GetPresetGroup(int id);

  // Description:
  // Assign a comment to a preset in the pool.
  // Return 1 on success, 0 on error
  virtual int SetPresetComment(int id, const char *group);
  virtual const char* GetPresetComment(int id);

  // Description:
  // Get the number of presets in the pool, or the number of presets with
  // the same group in the pool, or the number of visible presets, i.e.
  // the presets that are displayed despite the filters (see GroupFilter).
  virtual int GetNumberOfPresets();
  virtual int GetNumberOfPresetsWithGroup(const char *group);
  virtual int GetNumberOfVisiblePresets();

  // Description:
  // Retrieve the Id of a preset given its position in the pool, or
  // given its position in the pool within a group (i.e. nth-preset with
  // a given group), or given its row position as currently displayed in the
  // list.
  // Return id on success, -1 otherwise
  virtual int GetNthPresetId(int index);
  virtual int GetNthPresetIdWithGroup(int index, const char *group);
  virtual int GetNthVisiblePresetId(int row_index);

  // Description:
  // Retrieve the rank in the whole pool of the nth preset in the pool within
  // a group (i.e. nth-preset with a given group).
  // Return rank on success, -1 otherwise
  virtual int GetNthPresetRankWithGroup(
    int index, const char *group);

  // Description:
  // Remove a preset, or all of them, or all of the presets
  // with the same group.
  // Return 1 on success, 0 on error
  virtual int RemovePreset(int id);
  virtual int RemoveAllPresets();
  virtual int RemoveAllPresetsWithGroup(const char *group);

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
  // If passed a vtkRenderWindow, grab an image of the window contents.
  virtual int SetPresetImage(int id, vtkImageData *img);
  virtual int SetPresetImageFromRenderWindow(
    int id, vtkRenderWindow *win);
  virtual int HasPresetImage(int id);

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
  // This is used by the application to actually check which parameters
  // need to be stored in the preset, and call back this object to add
  // a preset (using the concrete subclass AddPreset method for example).
  // If it is not set, the 'add' button is not visible.
  virtual void SetAddPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when the the user tries to
  // apply a preset (by double-clicking on the preset for
  // example). The id of the preset is passed to the command.
  virtual void SetApplyPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Set/Get if a preset should be applied when it is selected (single-click),
  // or only when it is double-clicked on.
  // If set, only one preset can be selected at a time (if not, multiple
  // preset can be selected, and removed for example).
  virtual void SetApplyPresetOnSingleClick(int);
  vtkGetMacro(ApplyPresetOnSingleClick,int);
  vtkBooleanMacro(ApplyPresetOnSingleClick,int);

  // Description:
  // Specifies a command to be invoked when the the user tries to
  // remove a preset. The id of the preset is passed to the command.
  virtual void SetRemovePresetCommand(
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
  virtual const char* PresetCellEditStartCallback(
    const char*, int, int, const char*);
  virtual const char* PresetCellEditEndCallback(
    const char*, int, int, const char*);
  virtual void PresetCellIconCallback(const char*, int, int, const char*);
  virtual void PresetSelectionCallback();
  virtual void PresetSelectAndApplyPreviousCallback();
  virtual void PresetSelectAndApplyNextCallback();

protected:
  vtkKWAbstractPresetSelector();
  ~vtkKWAbstractPresetSelector();

  vtkKWMultiColumnListWithScrollbars *PresetList;
  vtkKWFrame                         *ControlFrame;
  vtkKWSpinButtons                   *PresetSpinButtons;
  vtkKWPushButtonSet                 *PresetButtons;

  int ApplyPresetOnSingleClick;

  int ThumbnailSize;
  int ScreenshotSize;

  char* GroupFilter;

  // Description:
  // Called when the number of presets has changed
  virtual void NumberOfPresetsHasChanged();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWAbstractPresetSelectorInternals *Internals;
  //ETX

  // Description:
  // Update one or all rows in the list
  virtual void UpdateRowsInPresetList();
  virtual void UpdateRowInPresetList(void*);

  char *AddPresetCommand;
  virtual void InvokeAddPresetCommand();

  char *ApplyPresetCommand;
  virtual void InvokeApplyPresetCommand(int id);

  char *RemovePresetCommand;
  virtual void InvokeRemovePresetCommand(int id);

private:

  vtkKWAbstractPresetSelector(const vtkKWAbstractPresetSelector&); // Not implemented
  void operator=(const vtkKWAbstractPresetSelector&); // Not implemented
};

#endif
