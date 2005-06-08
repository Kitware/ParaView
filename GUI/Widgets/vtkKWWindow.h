/*=========================================================================

  Module:    vtkKWWindow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWindow - a window superclass which holds splittable panels.
// .SECTION Description
// This class represents a top level window with a menu bar, a status
// line (as per vtkKWWindowBase) and user interface panels that can be
// resized and/or collapsed.
//
// Here is the layout of the whole window. Remember that the superclass
// "view frame" (as returned by GetViewFrame()) was the whole usable space
// between the menu bar on top (and eventually toolbars) and the status bar
// at the bottom.
//
// This "view frame" is now further divided horizontally into two parts by 
// a split frame instance, the MainSplitFrame.
// The first part (on the left usually) is called the "main panel" area, and
// it can be hidden or shown using the SetMainPanelVisibility() method.
// A convenience method GetMainPanelFrame() can be used to retrieve that
// "main panel area", even if it is currently entirely allocated to
// the main notebook, MainNotebook.
// The MainNotebook element is packed in the main panel and is used to
// display interface elements organized as *pages* inside *panels*. 
//
// Note that the proper way to do so is to create "user interface panels"
// (UIP, subclasses of vtkKWUserInterfacePanel), and set their 
// "user interface manager" (UIM) to the main user interface manager, as 
// returned by GetMainUserInterfaceManager(). Since the main UIM is itself
// attached to the main notebook, it will display the UIP automatically inside
// the notebook and take care of showing/raising/hiding the panels properly.
// The ShowMainUserInterface() method can be used show a main interface panel
// given its name. The name is the typically the string returned by the
// GetName() method of a vtkKWUserInterfacePanel (UIP). 
// The ShowMainUserInterface() method will query the main UIM to check if it
// is indeed managing a panel (UIP) with that name, and show/raise that UIP
// accordingly.
//
// Note that by doing following a framework, a subclass will be free of using
// a totally different type of UIM, while the UIP implementation and 
// manipulation will remain exactly the same. One just has to focus on creating
// simple panels packing user interface components, and the UIM will be
// responsible for mapping them into a higher-level interface, like a notebook.
// It will also take care of show/hiding conflicting interfaces, provide
// some cross-panels features like drag and drop, serialize the UI state, etc.
//
// In the same way, the right part of the MainSplitFrame (i.e., not the main
// panel itself, but the remaining space on the other side of the separator)
// is also divided vertically into two parts by a split frame instance, 
// the SecondarySplitFrame. The same methods as described above are available
// for this secondary subdivision, i.e. SetSecondaryPanelVisibility() can be
// used to show/hide the secondary panel, GetSecondaryPanelFrame() can be
// used to retrieve the secondary panel frame, the SecondaryNotebook is packed
// inside the secondary panel, and GetMainUserInterfaceManager() can be used
// to retrieve the corresponding UIM.
//
// Below the SecondarySplitFrame is a second toolbar set packed
// (SecondaryToolbarSet)
//
// Last, a third user interface manager, the ViewUserInterfaceManager is
// coupled to a ViewNotebook and packed inside the top part of the
// secondary split frame. A default page is added to provide the
// frame for GetViewFrame(). This notebook and user interface manager
// are likely not be manipulated as often as the other panels and UIM, but 
// can be used to provide multiple "views".
//
// +---------------------------+
// |           MB              |    MB: GetMenu() (see vtkKWTopLevel)
// +---------------------------+
// |           TBS             |    TBS: GetToolbars() (see superclass)
// +---------------------------+
// |+--+ MPF|+--+              |
// ||  +---+||  +-------------+|
// ||      |||                ||    MPF: GetMainPanelFrame()
// ||      |||    VNB/VF      ||    MNB: MainNotebook
// ||      |||                ||   
// ||      |||                ||    
// ||      ||+----------------+|
// || MNB  |+------------------+    SPF: GetSecondaryPanelFrame()
// ||      ||+--+      SPF     |    SNB: SecondaryNotebook
// ||      |||  +-------------+|
// ||      |||                ||    VNB: ViewNotebook
// ||      |||    SNB         ||    VF: GetViewFrame() (first page of VNB)
// ||      |||                ||
// ||      ||+----------------+|
// ||      |+------------------+
// |+------+|                  |    SF: GetStatusFrame() (see superclass)
// +--------+------------------+
// |            SF             |    SF: GetStatusFrame() (see superclass)
// +---------------------------+


#ifndef __vtkKWWindow_h
#define __vtkKWWindow_h

#include "vtkKWWindowBase.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWNotebook;
class vtkKWSplitFrame;
class vtkKWToolbar;
class vtkKWUserInterfaceManager;
class vtkKWUserInterfaceNotebookManager;
class vtkKWApplicationSettingsInterface;
class vtkKWUserInterfacePanel;

class KWWIDGETS_EXPORT vtkKWWindow : public vtkKWWindowBase
{
public:

  static vtkKWWindow* New();
  vtkTypeRevisionMacro(vtkKWWindow,vtkKWWindowBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the window
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Main panel. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  vtkGetObjectMacro(MainSplitFrame, vtkKWSplitFrame);
  virtual vtkKWFrame* GetMainPanelFrame();
  virtual int GetMainPanelVisibility();
  virtual void SetMainPanelVisibility(int);
  vtkBooleanMacro(MainPanelVisibility, int );
  vtkGetObjectMacro(MainNotebook, vtkKWNotebook);
  virtual vtkKWUserInterfaceManager* GetMainUserInterfaceManager();
  virtual void ShowMainUserInterface(const char *name);

  // Description:
  // Secondary panel. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  vtkGetObjectMacro(SecondarySplitFrame, vtkKWSplitFrame);
  virtual vtkKWFrame* GetSecondaryPanelFrame();
  virtual int GetSecondaryPanelVisibility();
  virtual void SetSecondaryPanelVisibility(int);
  vtkBooleanMacro(SecondaryPanelVisibility, int );
  vtkGetObjectMacro(SecondaryNotebook, vtkKWNotebook);
  virtual vtkKWUserInterfaceManager* GetSecondaryUserInterfaceManager();
  virtual void ShowSecondaryUserInterface(const char *name);

  // Description:
  // View panel. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  // This panel is probably not going to be used much, by default it
  // creates a single page in the notebook, which frame is returned by
  // GetViewFrame.
  vtkGetObjectMacro(ViewNotebook, vtkKWNotebook);
  virtual vtkKWUserInterfaceManager* GetViewUserInterfaceManager();
  virtual void ShowViewUserInterface(const char *name);

  // Description:
  // Convenience method to get the frame available for "viewing". 
  // Override the superclass to return a page in the notebook of the
  // view user interface manager (located in the first part of the 
  // SecondarySplitFrame).
  // The rational here is that GetViewFrame() always return the frame that
  // can be used by users or developpers to add more "viewing" element (say,
  // renderwidgets, 3D scenes), without knowing about the current layout.
  virtual vtkKWFrame* GetViewFrame();

  // Description:
  // Get the secondary toolbar set.
  vtkGetObjectMacro(SecondaryToolbarSet, vtkKWToolbarSet);

  // Description:
  // Call render on all widgets and elements that support that functionality
  virtual void Render();

  // Description:
  // Get the Application Settings Interface. 
  virtual vtkKWApplicationSettingsInterface *GetApplicationSettingsInterface();

  // Description:
  // Update the UI. This will call:
  //   UpdateToolbarState
  //   UpdateEnableState 
  //   UpdateMenuState
  //   Update on all panels belonging to the UserInterfaceManager, if any
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
  // Update the toolbar state
  virtual void UpdateToolbarState();

  // Description:
  // Callbacks.
  virtual void MainPanelVisibilityCallback();
  virtual void SecondaryPanelVisibilityCallback();
  virtual void PrintOptionsCallback();
  virtual void ToolbarVisibilityChangedCallback();
  virtual void NumberOfToolbarsChangedCallback();

  // Description:
  // Deallocate/delete/reparent some internal objects in order to solve
  // reference loops that would prevent this instance from being deleted.
  virtual void PrepareForDelete();

  // Description:
  // Some constants
  //BTX
  static const char *MainPanelSizeRegKey;
  static const char *MainPanelVisibilityRegKey;
  static const char *MainPanelVisibilityKeyAccelerator;
  static const char *HideMainPanelMenuLabel;
  static const char *ShowMainPanelMenuLabel;
  static const char *SecondaryPanelSizeRegKey;
  static const char *SecondaryPanelVisibilityRegKey;
  static const char *SecondaryPanelVisibilityKeyAccelerator;
  static const char *HideSecondaryPanelMenuLabel;
  static const char *ShowSecondaryPanelMenuLabel;
  static const char *DefaultViewPanelName;
  static const char *TclInteractorMenuLabel;
  //ETX

protected:
  vtkKWWindow();
  ~vtkKWWindow();

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometryToRegistry();
  virtual void RestoreWindowGeometryFromRegistry();

  // Description:
  // Show a main or secondary user interface panel.
  // The ShowMainUserInterface() method will
  // query the main UserInterfaceManager (UIM) to check if it is indeed
  // managing the UIP, and show/raise that UIP accordingly.
  // The ShowSecondaryUserInterface will do the same on the secondary UIM.
  // The ShowViewUserInterface will do the same on the view UIM.
  virtual void ShowMainUserInterface(vtkKWUserInterfacePanel *panel);
  virtual void ShowSecondaryUserInterface(vtkKWUserInterfacePanel *panel);
  virtual void ShowViewUserInterface(vtkKWUserInterfacePanel *panel);

  vtkKWSplitFrame *MainSplitFrame;
  vtkKWNotebook *MainNotebook;

  vtkKWSplitFrame *SecondarySplitFrame;
  vtkKWNotebook *SecondaryNotebook;

  vtkKWNotebook *ViewNotebook;

  vtkKWApplicationSettingsInterface *ApplicationSettingsInterface;

  vtkKWToolbarSet *SecondaryToolbarSet;

private:

  vtkKWUserInterfaceNotebookManager *MainUserInterfaceManager;
  vtkKWUserInterfaceNotebookManager *SecondaryUserInterfaceManager;
  vtkKWUserInterfaceNotebookManager *ViewUserInterfaceManager;

  vtkKWWindow(const vtkKWWindow&); // Not implemented
  void operator=(const vtkKWWindow&); // Not implemented
};

#endif

