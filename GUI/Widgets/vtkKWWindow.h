/*=========================================================================

  Module:    vtkKWWindow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWindow - a window superclass which holds one or more views
// .SECTION Description
// This class represents a top level window with menu bar and status
// line. It is designed to hold one or more vtkKWViews in it.

#ifndef __vtkKWWindow_h
#define __vtkKWWindow_h

#include "vtkKWWindowBase.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWNotebook;
class vtkKWSplitFrame;
class vtkKWToolbar;
class vtkKWUserInterfaceManager;

class VTK_EXPORT vtkKWWindow : public vtkKWWindowBase
{
public:

  static vtkKWWindow* New();
  vtkTypeRevisionMacro(vtkKWWindow,vtkKWWindowBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the window
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // The "view frame" of the superclass is now divided into two parts by 
  // a split frame instance, the MainSplitFrame.
  // The first part is called the "main panel" area, and it
  // can be hidden or shown by setting its visibility flag.
  // A convenience method GetMainPanelFrame can be used to retrieve that
  // "main panel area", even if it is currently entirely allocated to
  // the main notebook.
  // The MainNotebook element is packed in the main panel and is used to
  // display interface elements organized as pages inside panels.
  // The second part of the split frame is now the new "view frame".
  vtkGetObjectMacro(MainSplitFrame, vtkKWSplitFrame);
  virtual vtkKWFrame* GetMainPanelFrame();
  virtual int GetMainPanelVisibility();
  virtual void SetMainPanelVisibility(int);
  vtkBooleanMacro(MainPanelVisibility, int );
  vtkGetObjectMacro(MainNotebook, vtkKWNotebook);

  // Description:
  // Convenience method to get the frame available for "viewing". 
  // Override the superclass to return the second part of the MainSplitFrame.
  // The rational here is that GetViewFrame() always return the frame that
  // can be used by users or developpers to add more UI, without knowing
  // about the current layout.
  virtual vtkKWFrame* GetViewFrame();

  // Description:
  // Call render on all widgets and elements that support that functionality
  virtual void Render();

  // Description:
  // Get the User Interface Manager.
  virtual vtkKWUserInterfaceManager* GetUserInterfaceManager()
    { return 0; };

  // Description:
  // Update the UI. This will call:
  //   UpdateToolbarState
  //   UpdateEnableState 
  //   UpdateMenuState
  //   Update on all panels belonging to the GetUserInterfaceManager, if any
  virtual void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
  virtual void UpdateMenuState();

  // Description:
  // Callbacks.
  // Process the click on the error icon.
  // Override it in subclasses to popup more elaborate log/error dialog.
  virtual void MainPanelVisibilityCallback();

  // Description:
  // Some constants
  //BTX
  static const char *MainPanelSizeRegKey;
  static const char *HideMainPanelMenuLabel;
  static const char *ShowMainPanelMenuLabel;
  //ETX

protected:
  vtkKWWindow();
  ~vtkKWWindow();

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometryToRegistry();
  virtual void RestoreWindowGeometryFromRegistry();

  vtkKWNotebook *MainNotebook;
  vtkKWSplitFrame *MainSplitFrame;

private:
  vtkKWWindow(const vtkKWWindow&); // Not implemented
  void operator=(const vtkKWWindow&); // Not implemented
};

#endif

