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
// This old "view frame" is now further divided horizontally into two parts 
// by a split frame instance.
// The first part (on the left) is called the "main panel" area, and
// can be hidden or shown using the SetMainPanelVisibility() method.
// A convenience method GetMainPanelFrame() can be used to retrieve that
// "main panel area" frame, even if it is currently entirely allocated to
// the main notebook, MainNotebook.
// The MainNotebook element is packed in the main panel and is used to
// display interface elements organized as *pages* inside *panels*. 
//
// Note that the proper way to do so is to create "user interface panels"
// (UIP, subclasses of vtkKWUserInterfacePanel), and set their 
// "user interface manager" (UIM) to the main user interface manager, as 
// returned by GetMainUserInterfaceManager(). Since the main UIM is itself
// attached to the main notebook, it will display the UIP automatically inside
// the notebook and take care of showing/raising/hiding the pages properly.
// The ShowMainUserInterface() method can be used to show a main interface
// panel given its name. The name is typically the string returned by the
// GetName() method of a vtkKWUserInterfacePanel (UIP). 
// The ShowMainUserInterface() method will query the main UIM to check if it
// is indeed managing a panel (UIP) with that name, and show/raise that UIP
// accordingly. If you do not know which user interface manager is used
// by the panel, just call Show() or Raise() on the panel itself !
//
// Note that by following such framework, a subclass will be free of using
// a totally different type of UIM, while the UIP implementation and 
// manipulation will remain exactly the same. One just has to focus on creating
// simple panels to pack user interface components, and the UIM will be
// responsible for mapping them into a higher-level interface, like a notebook.
// It will also take care of show/hiding conflicting interfaces, provide
// some cross-panels features like drag and drop, serialize the UI state, etc.
//
// In the same way, the right part of the MainSplitFrame (i.e., not the main
// panel itself, but the remaining space on the right side of the separator)
// is also divided vertically into two parts by a split frame instance.
// The same methods as described above are available for this secondary
//  subdivision, i.e. SetSecondaryPanelVisibility() can be
// used to show/hide the secondary panel, GetSecondaryPanelFrame() can be
// used to retrieve the secondary panel frame, a SecondaryNotebook is
// packed inside the secondary panel, and GetMainUserInterfaceManager() can
// be used to retrieve the corresponding UIM.
//
// Below the SecondarySplitFrame is a second toolbar set (SecondaryToolbarSet)
// available for extra toolbars.
//
// The space available for "viewing", or packing 3D scenes, is returned by
// GetViewFrame(). Under the hood, a third user interface manager, the 
// ViewUserInterfaceManager is coupled to a ViewNotebook and packed inside
// the top part of the secondary split frame (i.e. the large part that is
// not considered a user interface panel). A default page is added to that
// notebook to provide the frame for GetViewFrame(). Since there is only 
// one page, the tab is not shown by default, leaving all the notebook
// space available for viewing. This notebook and its user interface manager
// are likely not be manipulated as often as the other panels and UIM, but 
// can be used to provide multiple "views" for example.
// As a convenience, a GetViewPanelFrame() method returns the parent
// of the notebook, i.e. the space into which the notebook was packed, so
// that other elements can be packed below or before the notebook itself.
//
// A fourth user interface manager is used to display and group all
// panels related to application settings (i.e. "Preferences"). It is not
// part of the layout as it will popup as a dialog instead. Note that
// the panels do not have to bother about that, the manager will parse
// each panel and create the dialog accordingly. If you have more application
// settings parameters, just create your own panels and set their UIM to
// the ApplicationSettingsUserInterfaceManager.
//
// This describes the default layout so far, where the secondary panel is
// located below the view frame. The PanelLayout ivar can be set 
// to change this layout to different configurations where:
// - the secondary panel is below the main panel.
// - the secondary panel is below both main and the view panel.
// Note that there is no accessor to get the split frame objects, since
// you should not rely on them to parent your widgets. Use the panel
// frame accessors instead, they will return the correct value even if
// the layout changes (i.e., GetMainPanelFrame(), GetSecondaryPanelFrame(),
// GetViewPanelFrame()).
//
// MB:   GetMenu() (see vtkKWTopLevel)
// MTBS: GetMainToolbarSet() (see superclass)
// MPF:  GetMainPanelFrame()
// MNB:  MainNotebook
// VNB:  ViewNotebook
// VPF:  GetViewPanelFrame()
// VF:   GetViewFrame() (first page of the VNB)
// SPF:  GetSecondaryPanelFrame()
// SNB:  SecondaryNotebook
// STBS: GetSecondaryToolbarSet()
// SF:   GetStatusFrame() (see superclass)
// 
// @verbatim
// +----------------------+  +----------------------+  +----------------------+
// |           MB         |  |        MB            |  |        MB            |
// +----------------------+  +----------------------+  +----------------------+
// |           MTBS       |  |           MTBS       |  |           MTBS       |
// +--------+-------------+  +--------+-------------+  +--------+-------------+
// |+--+ MPF|+--+ VPF     |  |+--+ MPF|+--+  VPF    |  |+--+ MPF|+--+    VPF  |
// ||  +---+||  +--------+|  ||  +---+||  +--------+|  ||  +---+||  +--------+|
// ||      |||           ||  ||      |||           ||  ||      |||           ||
// ||      ||| VNB (VF)  ||  ||      |||           ||  ||      |||           ||
// ||      |||           ||  ||      |||           ||  ||      ||| VNB (VF)  ||
// ||      |||           ||  || MNB  |||           ||  || MNB  |||           ||
// ||      ||+-----------+|  ||      |||           ||  ||      |||           ||
// || MNB  |+-------------+  |+------+|| VNB (VF)  ||  ||      |||           ||
// ||      ||+--+  SPF    |  +--------+|           ||  ||      ||+-----------+|
// ||      |||  +--------+|  |+--+ SPF||           ||  |+------+|   STBS      |
// ||      |||           ||  ||  +---+||           ||  +--------+-------------+
// ||      |||  SNB      ||  ||      |||           ||  |+--+ SPF              |
// ||      |||           ||  ||      |||           ||  ||  +-----------------+|
// ||      |||           ||  || SNB  |||           ||  || SNB                ||
// ||      ||+-----------+|  ||      ||+-----------+|  ||                    ||
// |+------+|    STBS     |  |+------+|      STBS   |  |+--------------------+|
// +--------+-------------+  +--------+-------------+  +----------------------+
// |            SF        |  |            SF        |  |            SF        |
// +----------------------+  +----------------------+  +----------------------+
//   Secondary below View      Secondary below Main      Secondary below Both
// @endverbatim

#ifndef __vtkKWWindow_h
#define __vtkKWWindow_h

#include "vtkKWWindowBase.h"

class vtkKWFrame;
class vtkKWNotebook;
class vtkKWSplitFrame;
class vtkKWToolbar;
class vtkKWUserInterfaceManager;
class vtkKWUserInterfaceManagerNotebook;
class vtkKWUserInterfaceManagerDialog;
class vtkKWApplicationSettingsInterface;
class vtkKWUserInterfacePanel;

class KWWidgets_EXPORT vtkKWWindow : public vtkKWWindowBase
{
public:

  static vtkKWWindow* New();
  vtkTypeRevisionMacro(vtkKWWindow,vtkKWWindowBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Main panel. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  virtual vtkKWFrame* GetMainPanelFrame();
  virtual int GetMainPanelVisibility();
  virtual void SetMainPanelVisibility(int);
  vtkBooleanMacro(MainPanelVisibility, int );
  virtual vtkKWNotebook* GetMainNotebook();
  virtual int HasMainUserInterfaceManager();
  virtual vtkKWUserInterfaceManager* GetMainUserInterfaceManager();
  virtual void ShowMainUserInterface(const char *name);
  vtkGetObjectMacro(MainSplitFrame, vtkKWSplitFrame);

  // Description:
  // Secondary panel. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  virtual vtkKWFrame* GetSecondaryPanelFrame();
  virtual int GetSecondaryPanelVisibility();
  virtual void SetSecondaryPanelVisibility(int);
  vtkBooleanMacro(SecondaryPanelVisibility, int );
  virtual vtkKWNotebook* GetSecondaryNotebook();
  virtual int HasSecondaryUserInterfaceManager();
  virtual vtkKWUserInterfaceManager* GetSecondaryUserInterfaceManager();
  virtual void ShowSecondaryUserInterface(const char *name);
  vtkGetObjectMacro(SecondarySplitFrame, vtkKWSplitFrame);

  // Description:
  // Set the panel layout type. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  // IMPORTANT: this ivar has to be set before calling Create(), and can
  // not be changed afterwards.
  //BTX
  enum 
  {
    PanelLayoutSecondaryBelowView = 0,
    PanelLayoutSecondaryBelowMain,
    PanelLayoutSecondaryBelowMainAndView
  };
  //ETX
  vtkSetClampMacro(PanelLayout, int, 
                   vtkKWWindow::PanelLayoutSecondaryBelowView, 
                   vtkKWWindow::PanelLayoutSecondaryBelowMainAndView);
  vtkGetMacro(PanelLayout, int);
  virtual void SetPanelLayoutToSecondaryBelowView()
    { this->SetPanelLayout(vtkKWWindow::PanelLayoutSecondaryBelowView);};
  virtual void SetPanelLayoutToSecondaryBelowMain()
    { this->SetPanelLayout(vtkKWWindow::PanelLayoutSecondaryBelowMain);};
  virtual void SetPanelLayoutToSecondaryBelowMainAndView()
    {this->SetPanelLayout(vtkKWWindow::PanelLayoutSecondaryBelowMainAndView);};

  // Description:
  // Set the view panel position. Default is on the right of the window.
  // IMPORTANT: this ivar has to be set before calling Create(), and can
  // not be changed afterwards.
  //BTX
  enum 
  {
    ViewPanelPositionLeft = 0,
    ViewPanelPositionRight
  };
  //ETX
  virtual void SetViewPanelPosition(int);
  virtual int GetViewPanelPosition();
  virtual void SetViewPanelPositionToLeft()
    { this->SetViewPanelPosition(vtkKWWindow::ViewPanelPositionLeft);};
  virtual void SetViewPanelPositionToRight()
    { this->SetViewPanelPosition(vtkKWWindow::ViewPanelPositionRight);};

  // Description:
  // Get the frame available for "viewing". 
  // Override the superclass to return a page in the notebook of the
  // view user interface manager (located in the first part of the 
  // SecondarySplitFrame).
  // This method should be used instead of GetViewPanelFrame(), unless
  // you really need to have both multiple notebook pages and common UI
  // elements on top or below the notebook.
  // The rational here is that GetViewFrame() always return the frame that
  // can be used by users or developpers to add more "viewing" element (say,
  // renderwidgets, 3D scenes), without knowing about the current layout.
  virtual vtkKWFrame* GetViewFrame();

  // Description:
  // View panel. 
  // The whole layout of the window is described at length at the beginning
  // of this document.
  // This panel is probably not going to be used much, by default it
  // creates a single page in the notebook, which frame is returned by
  // GetViewFrame(). The GetViewPanelFrame() method returns the parent of
  // the notebook, if one really need to pack something out of the 
  // GetViewFrame().
  virtual vtkKWNotebook* GetViewNotebook();
  virtual int HasViewUserInterfaceManager();
  virtual vtkKWUserInterfaceManager* GetViewUserInterfaceManager();
  virtual void ShowViewUserInterface(const char *name);
  virtual vtkKWFrame* GetViewPanelFrame();

  // Description:
  // Get the secondary toolbar set.
  virtual vtkKWToolbarSet* GetSecondaryToolbarSet();

  // Description:
  // Set the status frame position. The default position is at the
  // bottom of the window, but this object can also be displayed
  // at the bottom of the main panel (MainPanel), at the bottom of
  // the secondary panel (SecondaryPanel) or at the bottom of the view panel
  // (ViewPanel). Note that if any of the above is used, the status bar
  // will actually be hidden if the corresponding panel visibility is changed,
  // since the status bar is actually packed in the panel frame itself. Set
  // the position to LeftOfDivider or RightOfDivider to place the status
  // bar out of the panel, but either of the left or the right of the
  // main vertical divider.
  //BTX
  enum 
  {
    StatusFramePositionWindow = 0,
    StatusFramePositionMainPanel,
    StatusFramePositionSecondaryPanel,
    StatusFramePositionViewPanel,
    StatusFramePositionLeftOfDivider,
    StatusFramePositionRightOfDivider
  };
  //ETX
  vtkGetMacro(StatusFramePosition, int);
  virtual void SetStatusFramePosition(int);
  virtual void SetStatusFramePositionToWindow()
    { this->SetStatusFramePosition(
      vtkKWWindow::StatusFramePositionWindow); };
  virtual void SetStatusFramePositionToMainPanel()
    { this->SetStatusFramePosition(
      vtkKWWindow::StatusFramePositionMainPanel); };
  virtual void SetStatusFramePositionToSecondaryPanel()
    { this->SetStatusFramePosition(
      vtkKWWindow::StatusFramePositionSecondaryPanel); };
  virtual void SetStatusFramePositionToViewPanel()
    { this->SetStatusFramePosition(
      vtkKWWindow::StatusFramePositionViewPanel); };
  virtual void SetStatusFramePositionToLeftOfDivider()
    { this->SetStatusFramePosition(
      vtkKWWindow::StatusFramePositionLeftOfDivider); };
  virtual void SetStatusFramePositionToRightOfDivider()
    { this->SetStatusFramePosition(
      vtkKWWindow::StatusFramePositionRightOfDivider); };

  // Description:
  // Call render on all widgets and elements that support that functionality
  virtual void Render();

  // Description:
  // Get the Application Settings Interface as well as the Application
  // Settings User Interface Manager.
  virtual vtkKWUserInterfaceManager* GetApplicationSettingsUserInterfaceManager();
  virtual void ShowApplicationSettingsUserInterface(const char *name);
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
  // Deallocate/delete/reparent some internal objects in order to solve
  // reference loops that would prevent this instance from being deleted.
  virtual void PrepareForDelete();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void MainPanelVisibilityCallback();
  virtual void SecondaryPanelVisibilityCallback();
  virtual void PrintSettingsCallback();
  virtual void ToolbarVisibilityChangedCallback(vtkKWToolbar*);
  virtual void NumberOfToolbarsChangedCallback();

  // Description:
  // Some constants
  vtkGetStringMacro(MainPanelSizeRegKey);
  vtkGetStringMacro(MainPanelVisibilityRegKey);
  vtkGetStringMacro(MainPanelVisibilityKeyAccelerator);
  vtkGetStringMacro(HideMainPanelMenuLabel);
  vtkGetStringMacro(ShowMainPanelMenuLabel);
  vtkGetStringMacro(SecondaryPanelSizeRegKey);
  vtkGetStringMacro(SecondaryPanelVisibilityRegKey);
  vtkGetStringMacro(SecondaryPanelVisibilityKeyAccelerator);
  vtkGetStringMacro(HideSecondaryPanelMenuLabel);
  vtkGetStringMacro(ShowSecondaryPanelMenuLabel);
  vtkGetStringMacro(DefaultViewPanelName);
  vtkGetStringMacro(TclInteractorMenuLabel);
  vtkGetStringMacro(ViewPanelPositionRegKey);

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
  // The ShowApplicationSettingsUserInterface will do the same on the app 
  // settings UIM.
  virtual void ShowMainUserInterface(vtkKWUserInterfacePanel *panel);
  virtual void ShowSecondaryUserInterface(vtkKWUserInterfacePanel *panel);
  virtual void ShowViewUserInterface(vtkKWUserInterfacePanel *panel);
  virtual void ShowApplicationSettingsUserInterface(vtkKWUserInterfacePanel *panel);

  // Description:
  // Pack/repack the UI
  virtual void Pack();

  int PanelLayout;

  vtkKWSplitFrame *MainSplitFrame;

  vtkKWSplitFrame *SecondarySplitFrame;

  vtkKWApplicationSettingsInterface *ApplicationSettingsInterface;

  int             StatusFramePosition;

  // Description:
  // Some constants
  vtkSetStringMacro(MainPanelSizeRegKey);
  vtkSetStringMacro(MainPanelVisibilityRegKey);
  vtkSetStringMacro(MainPanelVisibilityKeyAccelerator);
  vtkSetStringMacro(HideMainPanelMenuLabel);
  vtkSetStringMacro(ShowMainPanelMenuLabel);
  vtkSetStringMacro(SecondaryPanelSizeRegKey);
  vtkSetStringMacro(SecondaryPanelVisibilityRegKey);
  vtkSetStringMacro(SecondaryPanelVisibilityKeyAccelerator);
  vtkSetStringMacro(HideSecondaryPanelMenuLabel);
  vtkSetStringMacro(ShowSecondaryPanelMenuLabel);
  vtkSetStringMacro(DefaultViewPanelName);
  vtkSetStringMacro(TclInteractorMenuLabel);
  vtkSetStringMacro(ViewPanelPositionRegKey);

private:

  vtkKWNotebook   *MainNotebook;
  vtkKWNotebook   *SecondaryNotebook;
  vtkKWNotebook   *ViewNotebook;

  vtkKWToolbarSet *SecondaryToolbarSet;

  vtkKWUserInterfaceManagerNotebook *MainUserInterfaceManager;
  vtkKWUserInterfaceManagerNotebook *SecondaryUserInterfaceManager;
  vtkKWUserInterfaceManagerNotebook *ViewUserInterfaceManager;

  vtkKWUserInterfaceManagerDialog *ApplicationSettingsUserInterfaceManager;

  vtkKWWindow(const vtkKWWindow&); // Not implemented
  void operator=(const vtkKWWindow&); // Not implemented

  // Description:
  // Some constants
  char *MainPanelSizeRegKey;
  char *MainPanelVisibilityRegKey;
  char *MainPanelVisibilityKeyAccelerator;
  char *HideMainPanelMenuLabel;
  char *ShowMainPanelMenuLabel;
  char *SecondaryPanelSizeRegKey;
  char *SecondaryPanelVisibilityRegKey;
  char *SecondaryPanelVisibilityKeyAccelerator;
  char *HideSecondaryPanelMenuLabel;
  char *ShowSecondaryPanelMenuLabel;
  char *DefaultViewPanelName;
  char *TclInteractorMenuLabel;
  char *ViewPanelPositionRegKey;
};

#endif

