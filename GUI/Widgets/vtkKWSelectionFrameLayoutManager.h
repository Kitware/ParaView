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
// .NAME vtkKWSelectionFrameLayoutManager - a MxN layout manager for a set of vtkKWSelectionFrame
// .SECTION Description
// This class is a layout manager for vtkKWSelectionFrame. It will grid them
// according to a given MxN resolution, allocate new ones, handle 
// print/screenshots, etc. 
// Note that the selection frame can be given any position, the resolution
// and the origin of the layout manager are actually the parameters that
// specify which one are visible. For example, one can set 4 widgets on
// a 2x2 grid, set the resolution to 1x1, but set the origin at (0,0) or
// (1,1) to zoom on a specific widget. 

#ifndef __vtkKWSelectionFrameLayoutManager_h
#define __vtkKWSelectionFrameLayoutManager_h

#include "vtkKWCompositeWidget.h"

class vtkKWSelectionFrame;
class vtkKWSelectionFrameLayoutManagerInternals;
class vtkKWRenderWidget;
class vtkImageData;
class vtkKWMenu;
class vtkKWToolbar;
class vtkKWFrame;

class KWWidgets_EXPORT vtkKWSelectionFrameLayoutManager : public vtkKWCompositeWidget
{
public:
  static vtkKWSelectionFrameLayoutManager* New();
  vtkTypeRevisionMacro(vtkKWSelectionFrameLayoutManager, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the resolution of the layout (number of columns, number of rows).
  virtual void SetResolution(int nb_cols, int nb_rows);
  virtual void SetResolution(int res[2])
    { this->SetResolution(res[0], res[1]); };
  vtkGetVector2Macro(Resolution, int);

  // Description:
  // Set/Get the origin of the layout (column, row), i.e. where to start
  // displaying widget from in the virtual grid on which they are set.
  virtual void SetOrigin(int col, int row);
  virtual void SetOrigin(int origin[2])
    { this->SetOrigin(origin[0], origin[1]); };
  vtkGetVector2Macro(Origin, int);

  // Description:
  // Add a new selection frame to the pool
  // This will call Register() on the widget (increasing its ref count).
  // Return 1 on success, 0 otherwise
  virtual int AddWidget(vtkKWSelectionFrame *widget);
  virtual int AddWidgetWithTagAndGroup(
    vtkKWSelectionFrame *widget, const char *tag, const char *group);

  // Description:
  // Allocate a new selection frame and add it to the pool.
  // Return the allocated widget, or NULL on error
  virtual vtkKWSelectionFrame* AllocateAndAddWidget();

  // Description:
  // Set the widget's tag in the pool.
  // This provide an alternate way of accessing the widget in the pool.
  // Note that this is *not* the title of the frame, as set using the
  // vtkKWSelectionFrame::SetTitle method.
  // If several widgets have the same tag, the first one is usually
  // retrieved. No constraints is put on the uniqueness of the tag.
  // Return 1 on success, 0 on error
  virtual int SetWidgetTag(vtkKWSelectionFrame*, const char *tag);
  virtual const char* GetWidgetTag(vtkKWSelectionFrame*);

  // Description:
  // Set the widget's group in the pool.
  // This provide an way of grouping widgets logicaly in the pool.
  // Return 1 on success, 0 on error
  virtual int SetWidgetGroup(vtkKWSelectionFrame*, const char *group);
  virtual const char* GetWidgetGroup(vtkKWSelectionFrame*);

  // Description:
  // Get the number of widgets in the pool
  virtual int GetNumberOfWidgets();
  virtual int GetNumberOfWidgetsWithTag(const char *tag);
  virtual int GetNumberOfWidgetsWithGroup(const char *group);

  // Description:
  // Query if widget is in the pool
  virtual int HasWidget(vtkKWSelectionFrame *widget);
  virtual int HasWidgetWithTag(const char *tag);
  virtual int HasWidgetWithTagAndGroup(const char *tag, const char *group);

  // Description:
  // Retrieve a widget given its tag (as set using SetWidgetTag()), its 
  // tag and group, its title (as set using vtkKWSelectionFrame::SetTitle), 
  // its rank in the pool (n-th widget), or its rank (n-th widget) but not
  // matching another widget 'avoid'.
  // Return the widget, or NULL if not found
  virtual vtkKWSelectionFrame* GetWidgetWithTag(const char *tag);
  virtual vtkKWSelectionFrame* GetWidgetWithTagAndGroup(
    const char *tag, const char *group);
  virtual vtkKWSelectionFrame* GetWidgetWithTitle(const char *title);
  virtual vtkKWSelectionFrame* GetNthWidget(int index);
  virtual vtkKWSelectionFrame* GetNthWidgetNotMatching(
    int index, vtkKWSelectionFrame *avoid);
  virtual vtkKWSelectionFrame* GetNthWidgetWithGroup(
    int index, const char *group);

  // Description:
  // Set/Get position of widget.
  // Note that the position is independent of the resolution, you can
  // still set the position of a widget anywhere while displaying only
  // a subset of the largest grid encompassing all widgets (i.e. you can
  // set a widget at (2, 3), i.e. third column, fourth row, but set the
  // resolution to (1, 1) to display only the first column and first row: the
  // next time the resolution is set to, say, (4, 4), the widget at (2, 3)
  // will be shown).
  // Return 1 (or widget) on success, 0 (or NULL) on error
  virtual int GetWidgetPosition(vtkKWSelectionFrame *w, int *col, int *row);
  virtual int GetWidgetPosition(vtkKWSelectionFrame *w, int pos[2])
    { return this->GetWidgetPosition(w, pos, pos + 1); }
  virtual int SetWidgetPosition(vtkKWSelectionFrame *w, int col, int row);
  virtual int SetWidgetPosition(vtkKWSelectionFrame *w, int pos[2])
    { return this->SetWidgetPosition(w, pos[0], pos[1]); }
  virtual vtkKWSelectionFrame* GetWidgetAtPosition(int col, int row);
  virtual vtkKWSelectionFrame* GetWidgetAtPosition(int pos[2])
    { return this->GetWidgetAtPosition(pos[0], pos[1]); }

  // Description:
  // Switch widgets position (set the position of two widgets)
  // Return 1 (or widget) on success, 0 (or NULL) on error
  virtual int SwitchWidgetsPosition(
    vtkKWSelectionFrame *w1, vtkKWSelectionFrame *w2);

  // Description:
  // Reorganize the widgets position automatically (for example, to try
  // to avoid empty spaces).
  vtkBooleanMacro(ReorganizeWidgetPositionsAutomatically, int);
  virtual void SetReorganizeWidgetPositionsAutomatically(int);
  vtkGetMacro(ReorganizeWidgetPositionsAutomatically, int);

  // Description:
  // Return if a specific widget is visible at this point, i.e. mapped
  // on screen at a specific position given the current resolution.
  virtual int GetWidgetVisibility(vtkKWSelectionFrame *w);

  // Description:
  // Check if a widget is maximized, i.e. at position (0,0) in a (1,1)
  // resolution. Maximize it (this will save the old resolution and position)
  // or undo maximize (this will restore old resolution and position).
  // Toggle maximize will maximize if widget is not maximized already or undo
  // the maximize
  // Return 1 on success, 0 on error
  virtual int IsWidgetMaximized(vtkKWSelectionFrame *w);
  virtual int MaximizeWidget(vtkKWSelectionFrame *w);
  virtual int UndoMaximizeWidget();
  virtual int ToggleMaximizeWidget(vtkKWSelectionFrame *w);

  // Description:
  // Select a a widget.
  // If arg is NULL, nothing is selected (all others are deselected)
  virtual void SelectWidget(vtkKWSelectionFrame *widget);

  // Description:
  // Get the selected widget.
  // Return the widget, or NULL if none is selected
  virtual vtkKWSelectionFrame* GetSelectedWidget();

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the selection is changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetSelectionChangedCommand(
    vtkObject *object, const char *method);
  vtkGetStringMacro(SelectionChangedCommand);

  // Description:
  // Remove a widget, or all of them, or all of the widgets in a group.
  // This will call each widget's Close() method, and UnRegister() it.
  // Return 1 on success, 0 on error
  virtual int RemoveWidget(vtkKWSelectionFrame *widget);
  virtual int RemoveAllWidgets();
  virtual int RemoveAllWidgetsWithGroup(const char *group);

  // Description:
  // Try to show all the widgets belonging to a specific group.
  // This method will check if the visible widgets belong to the group,
  // and exchange change if this is not the case.
  // Return 1 on success, 0 on error
  virtual int ShowWidgetsWithGroup(const char *group);

  // Description:
  // Save all widgets into an image (as a screenshot) or into the 
  // clipboard (win32). If no filename, the user is prompted for one
  // (provided that this widget is part of a window).
  // Return 1 on success, 0 otherwise
  // GetRenderWidget() need to be implemented accordingly.
  virtual int SaveScreenshotAllWidgets();
  virtual int SaveScreenshotAllWidgetsToFile(const char* fileName);
  virtual int CopyScreenshotAllWidgetsToClipboard();

  // Description:
  // Append widgets to image data.
  // The 'Fast' version of each method does not set the render widget to 
  // Offscreen rendering (unless OnScreenRendering is 1) 
  // and re-render them, the backbuffer of the render widget is taken as-is 
  // (this is useful for lower quality screenshot or thumbnails). 
  // Return 1 on success, 0 otherwise
  // GetRenderWidget() need to be implemented accordingly.
  virtual int AppendAllWidgetsToImageData(vtkImageData *image, 
                                          int OnScreenRendering = 0);
  virtual int AppendAllWidgetsToImageDataFast(vtkImageData *image);
  virtual int AppendSelectedWidgetToImageData(vtkImageData *image);
  virtual int AppendSelectedWidgetToImageDataFast(vtkImageData *image);

  // Description:
  // Print all widgets or the selected one.
  // If no DPI is provided, the DPI settings of the Window ivar is used.
  // Return 1 on success, 0 otherwise
  // GetRenderWidget() need to be implemented accordingly.
  virtual int PrintAllWidgets();
  virtual int PrintAllWidgetsAtResolution(double dpi);
  virtual int PrintSelectedWidget();
  virtual int PrintSelectedWidgetAtResolution(double dpi);

  // Description:
  // Create a resolution entries menu (specifies its parent).
  // Get the menu (so that it can be added as a cascade).
  // Update the menu entries state, given the number of widgets.
  // If the resolution is changed using a menu entry, a
  // ResolutionChangedEvent event is sent.
  virtual void CreateResolutionEntriesMenu(vtkKWMenu *parent);
  vtkGetObjectMacro(ResolutionEntriesMenu, vtkKWMenu);
  virtual void UpdateResolutionEntriesMenu();

  // Description:
  // Create a resolution entries toolbar (specifies its parent).
  // Get the toolbar.
  // Update the menu entries state, given the number of widgets.
  // If the resolution is changed using a toolbar button, a
  // ResolutionChangedEvent event is sent.
  virtual void CreateResolutionEntriesToolbar(vtkKWWidget *parent);
  vtkGetObjectMacro(ResolutionEntriesToolbar, vtkKWToolbar);
  virtual void UpdateResolutionEntriesToolbar();

  // Description:
  // Events. The SelectionChangedEvent is triggered when the selection
  // is changed. It is similar in concept as the 'SelectionChangedCommand' 
  // callbacks but can be used by multiple listeners/observers at a time.
  //BTX
  enum
  {
    SelectionChangedEvent = 10000,
    ResolutionChangedEvent
  };
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Update the selection lists
  virtual void UpdateSelectionLists();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void SelectWidgetCallback(vtkKWSelectionFrame*);
  virtual void SelectAndMaximizeWidgetCallback(vtkKWSelectionFrame*);
  virtual void CloseWidgetCallback(vtkKWSelectionFrame*);
  virtual int  ChangeWidgetTitleCallback(vtkKWSelectionFrame*);
  virtual void WidgetTitleChangedCallback(vtkKWSelectionFrame*);
  virtual void SwitchWidgetCallback(
    const char *title, vtkKWSelectionFrame *widget);
  virtual void ResolutionCallback(int i, int j);
  virtual void NumberOfWidgetsHasChangedCallback();

protected:
  vtkKWSelectionFrameLayoutManager();
  ~vtkKWSelectionFrameLayoutManager();

  // Description:
  // Get if a specific position is inside the layout defined by the
  // current resolution and origin.
  virtual int IsPositionInLayout(int col, int row);
  virtual int IsPositionInLayout(int pos[2]) 
    { return this->IsPositionInLayout(pos[0], pos[1]); };

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  int Resolution[2];
  int Origin[2];
  int ReorganizeWidgetPositionsAutomatically;

  vtkKWMenu    *ResolutionEntriesMenu;
  vtkKWToolbar *ResolutionEntriesToolbar;

  virtual void InvokeSelectionChangedCommand();
  char *SelectionChangedCommand;

  vtkKWFrame *LayoutFrame;

  // Description:
  // Allocate a new widget.
  virtual vtkKWSelectionFrame* AllocateWidget();

  // Description:
  // Create a widget (i.e., create the underlying Tk widget) and configure it.
  // If the layout manager has not been created, the widget won't be
  // created either, since it is used as parent.
  virtual void CreateWidget(vtkKWSelectionFrame*);
  virtual void ConfigureWidget(vtkKWSelectionFrame*);

  // Description:
  // Get the render widget (if any) associated to the selection
  // frame at that point. 
  // Used to Print, Save/Copy screenshot, etc.
  // This should be reimplemented by subclasses.
  // This implementation searches for a vtkKWRenderWidget in the children
  // of the frame.
  virtual vtkKWRenderWidget* GetRenderWidget(vtkKWSelectionFrame*);

  // Description:
  // Pack all widgets
  virtual void Pack();

  // Description:
  // Adjust the resolution so that all widgets are shown
  virtual void AdjustResolution();

  // Description:
  // Print widgets (if selection_only is true, only the selected
  // widget is printed, otherwise all of them).
  // Return 1 on success, 0 otherwise
  virtual int PrintWidgets(double dpi, int selection_only);

  // Description:
  // Append dataset widgets to image data.
  // If selection_only is true, onlythe selected dataset widget is printed, 
  // otherwise all of them. If direct is true, the renderwidgets are not
  // set to Offscreen rendering and re-rendered, the backbuffer of the
  // window is taken as-is (this is useful for lower quality screenshot or
  // thumbnails). 
  // If direct is true and ForceUpdateOnScreenRendering is 1, the render
  // widgets are forcibly re-rendererd prior to grabbing the screenshot. 
  // Return 1 on success, 0 otherwise
  virtual int AppendWidgetsToImageData(
    vtkImageData *image,int selection_only, int direct = 0, 
    int ForceUpdateOnScreenRendering = 0);

  // Description:
  // Called when the number of widgets has changed
  virtual void NumberOfWidgetsHasChanged();
  virtual void ScheduleNumberOfWidgetsHasChanged();

  // Description:
  // Reorganize positions of widgets so that the grid defined
  // by the current resolution is filled
  virtual void ReorganizeWidgetPositions();

  // Description:
  // Can a given widget's title be changed to a new one
  // It is used by ChangeWidgetTitleCallback and is likely to be overriden
  // in subclasses.
  virtual int CanWidgetTitleBeChanged(
    vtkKWSelectionFrame *widget, const char *new_title);

  // PIMPL Encapsulation for STL containers

  vtkKWSelectionFrameLayoutManagerInternals *Internals;

  // Description:
  // Add/Remove callbacks on a selection frame
  virtual void AddCallbacksToWidget(vtkKWSelectionFrame *widget);
  virtual void RemoveCallbacksFromWidget(vtkKWSelectionFrame *widget);

  // Description:
  // Delete a widget, i.e. Close and UnRegister it. Internal use only, as
  // a helper to the public RemoveWidget methods.
  virtual void DeleteWidget(vtkKWSelectionFrame *widget);

  // Description:
  // Set the widget position without repacking
  virtual int SetWidgetPositionInternal(
    vtkKWSelectionFrame *w, int col, int row);

  // Description:
  // Automatically move the selection inside the visible layout, i.e. if
  // the selected widget is not visible, select a visible one.
  // If pos_hint is non-null, use it as a hint regarding where the most
  // likely selection could be (array of 2 ints).
  virtual void MoveSelectionInsideVisibleLayout(int *pos_hint);

  // Description:
  // Set resolution and origin at the same time, to minimize redraw
  virtual void SetResolutionAndOrigin(int res[2], int origin[2])
    { this->SetResolutionAndOrigin(res[0], res[1], origin[0], origin[1]); };
  virtual void SetResolutionAndOrigin(
    int nb_cols, int nb_rows, int col, int row);

  // Description:
  // Push/Pop resolutions or origins on a stack
  // Is used by the maximize/minimize code.
  virtual int PushResolution(int nb_cols, int nb_rows);
  virtual int PushResolution(int res[2])
    { return this->PushResolution(res[0], res[1]); };
  virtual int PopResolution(int *nb_cols, int *nb_rows);
  virtual int PopResolution(int res[2])
    { return this->PopResolution(&res[0], &res[1]); };
  virtual int PushPosition(int col, int row);
  virtual int PushPosition(int origin[2])
    { return this->PushPosition(origin[0], origin[1]); };
  virtual int PopPosition(int *col, int *row);
  virtual int PopPosition(int origin[2])
    { return this->PopPosition(&origin[0], &origin[1]); };

  // Description:
  // Save/Restore layout before maximize/minimize.
  // Return 1 on success, 0 otherwise
  virtual int SaveLayoutBeforeMaximize();
  virtual int RestoreLayoutBeforeMaximize();

private:

  vtkKWSelectionFrameLayoutManager(const vtkKWSelectionFrameLayoutManager&); // Not implemented
  void operator=(const vtkKWSelectionFrameLayoutManager&); // Not implemented
};

#endif
