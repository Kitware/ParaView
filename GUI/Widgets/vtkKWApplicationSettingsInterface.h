/*=========================================================================

  Module:    vtkKWApplicationSettingsInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWApplicationSettingsInterface - a user interface panel.
// .SECTION Description
// A concrete implementation of a user interface panel.
// See vtkKWUserInterfacePanel for a more detailed description.
// .SECTION See Also
// vtkKWUserInterfacePanel vtkKWUserInterfaceManager vtkKWUserInterfaceNotebookManager

#ifndef __vtkKWApplicationSettingsInterface_h
#define __vtkKWApplicationSettingsInterface_h

#include "vtkKWUserInterfacePanel.h"

//----------------------------------------------------------------------------

#define VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY        "SaveWindowGeometry"
#define VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY          "ShowSplashScreen"
#define VTK_KW_SHOW_TOOLTIPS_REG_KEY               "ShowBalloonHelp"
#define VTK_KW_SHOW_MOST_RECENT_PANELS_REG_KEY     "ShowMostRecentPanels"
#define VTK_KW_ENABLE_GUI_DRAG_AND_DROP_REG_KEY    "EnableGUIDragAndDrop"

#define VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY      "ToolbarFlatFrame"
#define VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY    "ToolbarFlatButtons"

//----------------------------------------------------------------------------

class vtkKWCheckButton;
class vtkKWFrame;
class vtkKWFrameLabeled;
class vtkKWPushButton;
class vtkKWWindow;

class VTK_EXPORT vtkKWApplicationSettingsInterface : public vtkKWUserInterfacePanel
{
public:
  static vtkKWApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkKWApplicationSettingsInterface,vtkKWUserInterfacePanel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window (do not ref count it since the window will ref count
  // this widget).
  vtkGetObjectMacro(Window, vtkKWWindow);
  virtual void SetWindow(vtkKWWindow*);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Callback used when interaction has been performed.
  virtual void ConfirmExitCallback();
  virtual void SaveWindowGeometryCallback();
  virtual void ShowSplashScreenCallback();
  virtual void ShowBalloonHelpCallback();
  virtual void ShowMostRecentPanelsCallback();
  virtual void EnableDragAndDropCallback();
  virtual void ResetDragAndDropCallback();
  virtual void FlatFrameCallback();
  virtual void FlatButtonsCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWApplicationSettingsInterface();
  ~vtkKWApplicationSettingsInterface();

  vtkKWWindow       *Window;

  // Interface settings

  vtkKWFrameLabeled *InterfaceSettingsFrame;

  vtkKWCheckButton  *ConfirmExitCheckButton;
  vtkKWCheckButton  *SaveWindowGeometryCheckButton;
  vtkKWCheckButton  *ShowSplashScreenCheckButton;
  vtkKWCheckButton  *ShowBalloonHelpCheckButton;
  vtkKWCheckButton  *ShowMostRecentPanelsCheckButton;

  // Interface customization

  vtkKWFrameLabeled *InterfaceCustomizationFrame;
  vtkKWCheckButton  *EnableDragAndDropCheckButton;
  vtkKWPushButton   *ResetDragAndDropButton;

  // Toolbar settings

  vtkKWFrameLabeled *ToolbarSettingsFrame;
  vtkKWCheckButton  *FlatFrameCheckButton;
  vtkKWCheckButton  *FlatButtonsCheckButton;

private:
  vtkKWApplicationSettingsInterface(const vtkKWApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkKWApplicationSettingsInterface&); // Not Implemented
};

#endif


