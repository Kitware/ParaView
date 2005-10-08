/*=========================================================================

  Module:    vtkKWPresetSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPresetSelector - a volume property preset selector.
// .SECTION Description
// This class is a widget that can be used to store and apply volume property
// presets. 
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWPresetSelector_h
#define __vtkKWPresetSelector_h

#include "vtkKWCompositeWidget.h"

class vtkKWPresetSelectorInternals;
class vtkKWMultiColumnListWithScrollbars;
class vtkKWSpinButtons;
class vtkKWPushButtonSet;
class vtkImageData;
class vtkRenderWindow;
class vtkKWIcon;

class KWWIDGETS_EXPORT vtkKWPresetSelector : public vtkKWCompositeWidget
{
public:
  static vtkKWPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWPresetSelector, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Add a new preset.
  // Return the unique Id of the preset
  virtual int AddPreset();

  // Description:
  // Query if pool has given preset.
  // Return 1 if in the pool, 0 otherwise
  virtual int HasPreset(int id);

  // Description:
  // Assign a group to a preset in the pool.
  // This provide a way of grouping presets (say, if you
  // have different dataset at the same time and want to store
  // different presets for each dataset).
  // If there is already a preset for that group, it is removed
  // first.
  // Return 1 on success, 0 on error
  virtual int SetPresetGroup(int id, const char *group);
  virtual const char* GetPresetGroup(int id);

  // Description:
  // Set/Get the comment associated to a preset in the pool.
  // Return 1 on success, 0 on error
  virtual int SetPresetComment(int id, const char *comment);
  virtual const char* GetPresetComment(int id);

  // Description:
  // Set/Get the filename associated to a preset in the pool.
  // Return 1 on success, 0 on error
  virtual int SetPresetFileName(int id, const char *filename);
  virtual const char* GetPresetFileName(int id);

  // Description:
  // Get the creation time of a preset in the pool, as returned by
  // the vtksys::SystemTools::GetTime() method (can be cast to time_t)
  // Return 0 on error.
  virtual double GetPresetCreationTime(int id);

  // Description:
  // Get the number of presets in the pool, or the number of presets with
  // the same group in the pool, or the number of visible presets, i.e.
  // the presets that are displayed despite the filters (see GroupFilter).
  virtual int GetNumberOfPresets();
  virtual int GetNumberOfPresetsWithGroup(const char *group);
  virtual int GetNumberOfVisiblePresets();

  // Description:
  // Retrieve the Id of the nth-preset, or the id of the
  // nth preset with a given group
  // Return id on success, -1 otherwise
  virtual int GetNthPresetId(int index);
  virtual int GetNthPresetWithGroupId(int index, const char *group);

  // Description:
  // Retrieve the Id of the preset at a given row in the table list, or
  // the row of a given preset.
  virtual int GetPresetAtRowId(int row_index);
  virtual int GetPresetRow(int id);

  // Description:
  // Retrieve the rank of the nth preset with a given group in the pool
  // (i.e. nth-preset with a given group).
  // Return rank on success, -1 otherwise
  virtual int GetNthPresetWithGroupRank(int index, const char *group);

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
  // Set/Get the visibility of the group column.
  // No effect if called before Create().
  virtual void SetGroupColumnVisibility(int);
  virtual int GetGroupColumnVisibility();
  vtkBooleanMacro(GroupColumnVisibility, int);

  // Description:
  // Set/Get the visibility of the comment column.
  // No effect if called before Create().
  virtual void SetCommentColumnVisibility(int);
  virtual int GetCommentColumnVisibility();
  vtkBooleanMacro(CommentColumnVisibility, int);

  // Description:
  // Set/Get the visibility of the select spin buttons.
  virtual void SetSelectSpinButtonsVisibility(int);
  vtkGetMacro(SelectSpinButtonsVisibility,int);
  vtkBooleanMacro(SelectSpinButtonsVisibility,int);

  // Description:
  // Assign an image to a preset in the pool.
  // This will create a thumbnail icon to be displayed in the image column,
  // and a larger screenshot to be displayed when the user hovers over
  // that thumbnail. 
  // If passed a vtkRenderWindow, grab an image of the window contents.
  virtual int SetPresetImage(int id, vtkImageData *img);
  virtual int SetPresetImageFromRenderWindow(
    int id, vtkRenderWindow *win);
  virtual int HasPresetImage(int id);
  virtual vtkKWIcon* GetPresetThumbnail(int id);
  virtual vtkKWIcon* GetPresetScreenshot(int id);

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
  // This gives the opportunity for the application to check and collect the
  // relevant information to store in a new preset. The application will then
  // most likely add the preset (using the AddPreset method) and set its
  // properties (using SetPresetGroup, SetPresetComment, SetPreset..., etc).
  // If not set, the 'add' button is not visible.
  virtual void SetAddPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when the "update" button is pressed.
  // This gives the opportunity for the application to check and collect the
  // relevant information to update in the preset. The application will then
  // most likely update the presets's properties (using SetPresetGroup, 
  // SetPresetComment, SetPreset..., etc).
  // The id of the preset to update is passed to the command.
  // If not set, the 'update' button is not visible.
  virtual void SetUpdatePresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when the the user tries to
  // apply a preset (by double-clicking on the preset for
  // example). 
  // The id of the preset is passed to the command.
  virtual void SetApplyPresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Set/Get if a preset should be applied when it is selected (single-click),
  // or only when it is double-clicked on.
  // If set, only one preset can be selected at a time (if not, multiple
  // preset can be selected, and removed for example).
  virtual void SetApplyPresetOnSelection(int);
  vtkGetMacro(ApplyPresetOnSelection,int);
  vtkBooleanMacro(ApplyPresetOnSelection,int);

  // Description:
  // Specifies a command to be invoked when the the user tries to
  // remove a preset from the pool. 
  // This command is called *before* the preset is removed from the pool,
  // so that the callback can still query all the preset properties.
  // The id of the preset is passed to the command.
  virtual void SetRemovePresetCommand(
    vtkObject* object, const char *method);

  // Description:
  // Set/Get if the user should be prompted before removing a preset
  vtkSetMacro(PromptBeforeRemovePreset, int);
  vtkGetMacro(PromptBeforeRemovePreset, int);
  vtkBooleanMacro(PromptBeforeRemovePreset, int);

  // Description:
  // Specifies a command to be invoked when the preset has been
  // changed using the UI (say, its comment has been edited for example).
  // The id of the preset is passed to the command.
  virtual void SetPresetHasChangedCommand(
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
  virtual void PresetUpdateCallback();
  virtual void PresetRemoveCallback();
  virtual void PresetSelectionCallback();
  virtual void PresetSelectPreviousCallback();
  virtual void PresetSelectNextCallback();

  // Description:
  // Callback used to refresh the contents of the image cell for each preset
  virtual void PresetCellImageCallback(const char*, int, int, const char*);

  // Description:
  // Callback invoked when user starts editing a specific preset field
  // located at cell ('row', 'col') with contents 'text'.
  // This method returns the value that is to become the initial 
  // contents of the temporary embedded widget used for editing.
  // Most of the time, this is the same as 'text'.
  // The next step (validation) is handled by PresetCellEditEndCallback
  virtual const char* PresetCellEditStartCallback(
    int row, int col, const char *text);

  // Description:
  // Callback invoked when the user ends editing a specific preset field
  // located at cell ('row', 'col').
  // The main purpose of this method is to perform a final validation of
  // the edit window's contents 'text'.
  // This method returns the value that is to become the new contents
  // for that cell.
  // The next step (updating) is handled by PresetCellUpdateCallback
  virtual const char* PresetCellEditEndCallback(
    int row, int col, const char *text);

  // Description:
  // Callback invoked when the user successfully updated the preset field
  // located at ('row', 'col') with the new contents 'text', as a result
  // of editing the corresponding cell interactively.
  virtual void PresetCellUpdatedCallback(int row, int col, const char *text);

  // Description:
  // Set/Get a preset user slot.
  // Return 1 on success, 0 on error
  virtual int HasPresetUserSlot(
    int id, const char *slot_name);
  virtual int SetPresetUserSlotAsDouble(
    int id, const char *slot_name, double value);
  virtual double GetPresetUserSlotAsDouble(
    int id, const char *slot_name);
  virtual int SetPresetUserSlotAsInt(
    int id, const char *slot_name, int value);
  virtual int GetPresetUserSlotAsInt(
    int id, const char *slot_name);
  virtual int SetPresetUserSlotAsString(
    int id, const char *slot_name, const char *value);
  virtual const char* GetPresetUserSlotAsString(
    int id, const char *slot_name);
  virtual int SetPresetUserSlotAsPointer(
    int id, const char *slot_name, void *ptr);
  virtual void* GetPresetUserSlotAsPointer(
    int id, const char *slot_name);

  // Description:
  // Some constants
  //BTX
  static const char *IdColumnName;
  static const char *ImageColumnName;
  static const char *GroupColumnName;
  static const char *CommentColumnName;
  //ETX

protected:
  vtkKWPresetSelector();
  ~vtkKWPresetSelector();

  vtkKWMultiColumnListWithScrollbars *PresetList;
  vtkKWFrame                         *PresetControlFrame;
  vtkKWSpinButtons                   *PresetSelectSpinButtons;
  vtkKWPushButtonSet                 *PresetButtons;

  int ApplyPresetOnSelection;
  int SelectSpinButtonsVisibility;

  int ThumbnailSize;
  int ScreenshotSize;
  int PromptBeforeRemovePreset;

  char* GroupFilter;

  // Description:
  // Called when the number of presets has changed
  virtual void NumberOfPresetsHasChanged();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWPresetSelectorInternals *Internals;
  //ETX

  // Description:
  // Create the columns.
  // Subclasses should override this method to add their own columns and
  // display their own preset fields.
  virtual void CreateColumns();

  // Description:
  // Deallocate a preset.
  // Subclasses should override this method to release the memory allocated
  // by their own preset fields.
  virtual void DeAllocatePreset(int id) {};

  // Description:
  // Update the row in the list for a given preset.
  // Subclass should override this method to display their own fields.
  // Return 1 on success, 0 if the row was not (or can not be) updated.
  // Subclasses should call the parent's UpdatePresetRow, and abort
  // if the result is not 1.
  virtual int UpdatePresetRow(int id);

  // Description:
  // Update all rows in the list
  virtual void UpdateRowsInPresetList();

  char *AddPresetCommand;
  virtual void InvokeAddPresetCommand();

  char *UpdatePresetCommand;
  virtual void InvokeUpdatePresetCommand(int id);

  char *ApplyPresetCommand;
  virtual void InvokeApplyPresetCommand(int id);

  char *RemovePresetCommand;
  virtual void InvokeRemovePresetCommand(int id);

  char *PresetHasChangedCommand;
  virtual void InvokePresetHasChangedCommand(int id);

  // Description:
  // Convenience methods to get the index of a given column
  virtual int GetIdColumnIndex();
  virtual int GetImageColumnIndex();
  virtual int GetGroupColumnIndex();
  virtual int GetCommentColumnIndex();

  // Description:
  // Pack
  virtual void Pack();

private:

  vtkKWPresetSelector(const vtkKWPresetSelector&); // Not implemented
  void operator=(const vtkKWPresetSelector&); // Not implemented
};

#endif
