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
// according to a given MxN resolution, allocate new ones, handle print/screenshots, etc. 

#ifndef __vtkKWSelectionFrameLayoutManager_h
#define __vtkKWSelectionFrameLayoutManager_h

#include "vtkKWCompositeWidget.h"

class vtkKWSelectionFrame;
class vtkKWSelectionFrameLayoutManagerInternals;
class vtkKWRenderWidget;
class vtkImageData;
class vtkKWMenu;
class vtkKWToolbar;

class KWWIDGETS_EXPORT vtkKWSelectionFrameLayoutManager : public vtkKWCompositeWidget
{
public:
  static vtkKWSelectionFrameLayoutManager* New();
  vtkTypeRevisionMacro(vtkKWSelectionFrameLayoutManager, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Control the resolution in widget by widget (columns, rows)
  virtual void SetResolution(int i, int j);
  virtual void SetResolution(int res[2])
    { this->SetResolution(res[0], res[1]); };
  vtkGetVector2Macro(Resolution, int);

  // Description:
  // Add a new selection frame to the pool (a unique name is required).
  // This will call Register() on the widget (increasing its ref count).
  // Return 1 on success, 0 otherwise
  virtual int AddWidget(vtkKWSelectionFrame *widget, const char *name);

  // Description:
  // Allocate a new selection frame and add it to the pool (a unique name is
  // required).
  // Return the allocated widget, or NULL on error
  virtual vtkKWSelectionFrame* AllocateAndAddWidget(const char *name);

  // Description:
  // Get widget with given name (the unique one that was used to add it to the 
  // pool), with given title (each widget/selection-frame can customize 
  // its own title), or with given rank (n-th widget), or with given
  // rank (n-th widget) but not matching another widget 'avoid'.
  // Return the widget, or NULL if not found
  virtual vtkKWSelectionFrame* GetWidgetWithName(const char *name);
  virtual vtkKWSelectionFrame* GetWidgetWithTitle(const char *title);
  virtual vtkKWSelectionFrame* GetNthWidget(int index);
  virtual vtkKWSelectionFrame* GetNthWidgetNotMatching(
    int index, vtkKWSelectionFrame *avoid);
  virtual const char* GetNthWidgetName(int index);
  virtual int GetNumberOfWidgets();

  // Description:
  // Change widget's name in pool (it still has to be unique).
  // This is not the title of the frame, it is the unique name 
  // (i.e. identifier) of the widget within the pool.
  // Return 1 on success, 0 on error
  virtual int RenameWidget(vtkKWSelectionFrame*, const char *new_name);
  virtual int RenameWidgetWithName(const char *old_name, const char *new_name);

  // Description:
  // Set/Get position of widget with given name, or widget
  // at given position.
  // Return 1 (or widget) on success, 0 (or NULL) on error
  virtual int GetWidgetPosition(vtkKWSelectionFrame*, int pos[2]);
  virtual int GetWidgetPositionWithName(const char *name, int pos[2]);
  virtual int SetWidgetPosition(vtkKWSelectionFrame*, int pos[2]);
  virtual int SetWidgetPositionWithName(const char *name, int pos[2]);
  virtual vtkKWSelectionFrame* GetWidgetAtPosition(int pos[2]);

  // Description:
  // Query if widget in the pool
  virtual int HasWidgetWithName(const char *name);
  virtual int HasWidget(vtkKWSelectionFrame *dswidget);

  // Description:
  // Selected a given widget.
  // If arg is NULL, nothing is selected (all others are deselected)
  virtual void SelectWidgetWithName(const char *name);
  virtual void SelectWidget(vtkKWSelectionFrame *dswidget);

  // Description:
  // Get the selected widget.
  // Return the widget, or NULL if none is selected
  virtual vtkKWSelectionFrame* GetSelectedWidget();
  virtual const char* GetSelectedWidgetName();

  // Description:
  // Specifies a command to be invoked when the selection has changed
  virtual void SetSelectionChangedCommand(
    vtkObject* object, const char *method);

  // Description:
  // Delete widget (or all) of them. This will call the widget's Close() 
  // method, and UnRegister() it.
  // Return 1 on success, 0 on error
  virtual int DeleteWidgetWithName(const char *name);
  virtual int DeleteWidget(vtkKWSelectionFrame *dswidget);
  virtual int DeleteAllWidgets();

  // Description:
  // Save all widgets into an image (as a screenshot) or into the 
  // clipboard (win32). If no filename, the user is prompted for one
  // (provided that this widget is part of a window).
  // Return 1 on success, 0 otherwise
  // GetVisibleRenderWidget() need to be implemented accordingly.
  virtual int SaveScreenshotAllWidgets();
  virtual int SaveScreenshotAllWidgetsToFile(const char* fileName);
  virtual int CopyScreenshotAllWidgetsToClipboard();

  // Description:
  // Append widgets to image data
  // Return 1 on success, 0 otherwise
  // GetVisibleRenderWidget() need to be implemented accordingly.
  virtual int AppendAllWidgetsToImageData(vtkImageData *image);
  virtual int AppendSelectedWidgetToImageData(vtkImageData *image);

  // Description:
  // Print all widgets or the selected one.
  // If no DPI is provided, the DPI settings of the Window ivar is used.
  // Return 1 on success, 0 otherwise
  // GetVisibleRenderWidget() need to be implemented accordingly.
  virtual int PrintAllWidgets();
  virtual int PrintAllWidgetsAtResolution(double dpi);
  virtual int PrintSelectedWidget();
  virtual int PrintSelectedWidgetAtResolution(double dpi);

  // Description:
  // Create a resolution entries menu (specifies its parent).
  // Get the menu (so that it can be added as a cascade).
  // Update the menu entries state, given the number of widgets.
  virtual void CreateResolutionEntriesMenu(vtkKWMenu *parent);
  vtkGetObjectMacro(ResolutionEntriesMenu, vtkKWMenu);
  virtual void UpdateResolutionEntriesMenu();

  // Description:
  // Create a resolution entries toolbar (specifies its parent).
  // Get the toolbar.
  // Update the menu entries state, given the number of widgets.
  virtual void CreateResolutionEntriesToolbar(vtkKWWidget *parent);
  vtkGetObjectMacro(ResolutionEntriesToolbar, vtkKWToolbar);
  virtual void UpdateResolutionEntriesToolbar();

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
  virtual void SelectWidgetCallback(vtkKWSelectionFrame*);
  virtual void SelectAndMaximizeWidgetCallback(vtkKWSelectionFrame*);
  virtual void CloseWidgetCallback(vtkKWSelectionFrame*);
  virtual int  ChangeWidgetTitleCallback(vtkKWSelectionFrame*);
  virtual void SwitchWidgetCallback(
    const char *title, vtkKWSelectionFrame *widget);

  // Description:
  // Update the selection lists
  virtual void UpdateSelectionLists();

protected:
  vtkKWSelectionFrameLayoutManager();
  ~vtkKWSelectionFrameLayoutManager();

  int Resolution[2];
  vtkKWMenu    *ResolutionEntriesMenu;
  vtkKWToolbar *ResolutionEntriesToolbar;
  char *SelectionChangedCommand;

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
  // frame and visible at that point. 
  // Used to Print, Save/Copy screenshot, etc.
  // This should be reimplemented by subclasses (return NULL at the moment).
  virtual vtkKWRenderWidget* GetVisibleRenderWidget(vtkKWSelectionFrame*);

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
  // Append dataset widgets to image data (if selection_only is true, only
  // the selected dataset widget is printed, otherwise all of them).
  // Return 1 on success, 0 otherwise
  virtual int AppendWidgetsToImageData(vtkImageData *image,int selection_only);

  // Description:
  // Called when the number of widgets has changed
  virtual void NumberOfWidgetsHasChanged();

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

private:

  vtkKWSelectionFrameLayoutManager(const vtkKWSelectionFrameLayoutManager&); // Not implemented
  void operator=(const vtkKWSelectionFrameLayoutManager&); // Not implemented
};

#endif
