/*=========================================================================

  Module:    vtkKWWindowBase.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWindowBase - a window superclass
// .SECTION Description
// This class represents a top level window with a menu bar, a status
// line and a main central frame.

#ifndef __vtkKWWindowBase_h
#define __vtkKWWindowBase_h

#include "vtkKWTopLevel.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenu;
class vtkKWMostRecentFilesManager;
class vtkKWProgressGauge;
class vtkKWSeparator;
class vtkKWTclInteractor;
class vtkKWToolbar;
class vtkKWToolbarSet;

class KWWidgets_EXPORT vtkKWWindowBase : public vtkKWTopLevel
{
public:

  static vtkKWWindowBase* New();
  vtkTypeRevisionMacro(vtkKWWindowBase,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Close this window, possibly prompting the user.
  // Note that the current vtkKWApplication implementation will
  // exit the application if no more windows are open.
  // Return 1 if the window closed successfully, 0 otherwise (for example,
  // if some dialogs are still up, or the user did not confirm, etc).
  virtual int Close();

  // Description:
  // Set/Get if a confirmation dialog should be displayed before a
  // window is closed. Default to false.
  vtkSetMacro(PromptBeforeClose, int);
  vtkGetMacro(PromptBeforeClose, int);
  vtkBooleanMacro(PromptBeforeClose, int);

  // Description:
  // Load and evaluate a Tcl based script. 
  // If called without an argument it will open a file dialog.
  // This implementation basically forwards the call to 
  // vtkKWApplication::LoadScript.
  virtual void LoadScript();
  virtual void LoadScript(const char *filename);

  // Description:
  // The extension used in LoadScript. Default is .tcl.
  vtkSetStringMacro(ScriptExtension);
  vtkGetStringMacro(ScriptExtension);

  // Description:
  // The type name used in LoadScript. Default is Tcl.
  vtkSetStringMacro(ScriptType);
  vtkGetStringMacro(ScriptType);

  // Description:
  // Set the text for the status bar of this window.
  virtual void SetStatusText(const char *);
  virtual const char *GetStatusText();
  
  // Description:
  // Popup a warning/error message.
  // This can be overriden in a subclass to redirect errors and warnings
  // to log files or more elaborate log windows.
  virtual void WarningMessage(const char* message);
  virtual void ErrorMessage(const char* message);

  // Description:
  // Set the error / warning icon in the tray.
  // Note that if StatusFrameVisibility is Off, you may want to move the
  // tray frame to a different position (say, in a toolbar), using
  // the SetTrayFramePosition() method.
  //BTX
  enum 
  {
    ErrorIconNone = 0,
    ErrorIconBlack,
    ErrorIconRed
  };
  //ETX
  virtual void SetErrorIcon(int);
  virtual void SetErrorIconToNone()
    { this->SetErrorIcon(vtkKWWindowBase::ErrorIconNone); };
  virtual void SetErrorIconToBlack()
    { this->SetErrorIcon(vtkKWWindowBase::ErrorIconBlack); };
  virtual void SetErrorIconToRed()
    { this->SetErrorIcon(vtkKWWindowBase::ErrorIconRed); };

  // Description:
  // The window is made of a menu bar (methods are available to access each
  // menus), a separator, a toolbar placeholder, a large main frame called
  // the "view frame", and a status frame (inside which the a progress
  // gauge and some other UI elements can be found).
  // Note that this large frame is likely to be re-allocated by subclasses
  // into a different UI structure involving panels, notebooks, interface 
  // managers, etc. therefore GetViewFrame() will be overriden in order to
  // return the most convenient viewing frame. 
  // The rational here is that GetViewFrame() always return the frame that
  // can be used by users or developpers to add more "viewing" element (say,
  // renderwidgets, 3D scenes), without knowing about the current layout.
  virtual vtkKWFrame* GetViewFrame();

  // Description:
  // Get the main toolbar set.
  vtkGetObjectMacro(MainToolbarSet, vtkKWToolbarSet);

  // Description:
  // Get the status frame object.
  vtkGetObjectMacro(StatusFrame, vtkKWFrame);

  // Description:
  // Set/Get the visibility of the status frame. If set to Off, the status
  // text, as set by SetStatusText(), will not be displayed anymore. Neither
  // will the progress gauge, the application icon, the tray frame and status
  // icons. Both the progress gauge and tray frame position can be changed 
  // independently though (see SetProgressGaugePosition and 
  // SetTrayFramePosition).
  virtual void SetStatusFrameVisibility(int flag);
  vtkGetMacro(StatusFrameVisibility, int);
  vtkBooleanMacro(StatusFrameVisibility, int);  

  // Description:
  // Get the progress gauge widget.  The progress gauge is displayed
  // in the Status frame on the bottom right corner of the window.
  vtkGetObjectMacro(ProgressGauge, vtkKWProgressGauge);

  // Description:
  // Set the progress gauge position. The default position is in the
  // status frame, but this object can also be displayed in a toolbar, on
  // top of the window. This is useful when StatusFrameVisibility is set
  // to Off.
  //BTX
  enum 
  {
    ProgressGaugePositionStatusFrame = 0,
    ProgressGaugePositionToolbar
  };
  //ETX
  virtual void SetProgressGaugePosition(int);
  virtual void SetProgressGaugePositionToStatusFrame()
    { this->SetProgressGaugePosition(
      vtkKWWindowBase::ProgressGaugePositionStatusFrame); };
  virtual void SetProgressGaugePositionToToolbar()
    { this->SetProgressGaugePosition(
      vtkKWWindowBase::ProgressGaugePositionToolbar); };

  // Description:
  // Get the tray frame object. A default status icon is already packed
  // in this frame and modified by SetErrorIcon, but other icons can
  // probably fit there.
  vtkGetObjectMacro(TrayFrame, vtkKWFrame);

  // Description:
  // Set the tray frame position. The default position is in the
  // status frame, but this object can also be displayed in a toolbar, on
  // top of the window. This is useful when StatusFrameVisibility is set
  // to Off.
  //BTX
  enum 
  {
    TrayFramePositionStatusFrame = 0,
    TrayFramePositionToolbar
  };
  //ETX
  vtkGetMacro(TrayFramePosition, int);
  virtual void SetTrayFramePosition(int);
  virtual void SetTrayFramePositionToStatusFrame()
    { this->SetTrayFramePosition(
      vtkKWWindowBase::TrayFramePositionStatusFrame); };
  virtual void SetTrayFramePositionToToolbar()
    { this->SetTrayFramePosition(
      vtkKWWindowBase::TrayFramePositionToolbar); };

  // Description:
  // Get the menu objects. This will allocate and create them on the fly.
  // Several convenience functions are also available to get the position
  // where to safely insert entries in those menus without interferring with
  // entries that should stay at the end of the menus.
  vtkKWMenu *GetFileMenu();
  vtkKWMenu *GetEditMenu();
  vtkKWMenu *GetViewMenu();
  vtkKWMenu *GetWindowMenu();
  vtkKWMenu *GetHelpMenu();
  vtkKWMenu *GetToolbarsVisibilityMenu();
  
  // Description:
  // Convenience method that return the position where to safely insert 
  // entries in the corresponding menu without interferring with entries
  // that should stay at the end of the menu.
  // At the moment, GetFileMenuInsertPosition() checks for the 'close',
  // 'exit' or 'print setup' commands, GetHelpMenuInsertPosition() checks for
  // the 'about' commands, GetViewMenuInsertPosition() is available for
  // subclasses to be redefined. 
  virtual int GetFileMenuInsertPosition();
  virtual int GetHelpMenuInsertPosition();
  virtual int GetViewMenuInsertPosition();

  // Description:
  // Set/Get a hint about help support. Disabled by default.
  // If set to true (programmatically or by a superclass), it will hint the
  // instance about populating the help menu with common entries. 
  // For example, an entry invoking the application's DisplayHelpDialog.
  vtkSetClampMacro(SupportHelp, int, 0, 1);
  vtkGetMacro(SupportHelp, int);
  vtkBooleanMacro(SupportHelp, int);

  // Description:
  // Add a file to the Recent File list, and save the whole list to
  // the registry.
  // If the "Recent files" sub-menu has been inserted at that point (see
  // the InsertRecentFilesMenu method), it will be updated as well.
  virtual void AddRecentFile(
    const char *filename, vtkObject *target, const char *command);
  
  // Description:
  // Set/Get a hint about print support. Disabled by default.
  // If set to true (programmatically or by a superclass), it will hint the
  // instance about populating some menus with common print-related entries. 
  // For example, an entry in the file menu to set up print options like
  // the application's PrintTargetDPI.
  vtkSetClampMacro(SupportPrint, int, 0, 1);
  vtkGetMacro(SupportPrint, int);
  vtkBooleanMacro(SupportPrint, int);

  // Description:
  // Get title of window.
  // Override the superclass to use app name if the title was not set
  virtual char* GetTitle();

  // Description:
  // Get/display the tcl interactor.
  virtual vtkKWTclInteractor* GetTclInteractor();
  virtual void DisplayTclInteractor();
  
  // Description:
  // Update the UI. This will call:
  //   UpdateToolbarState
  //   UpdateEnableState 
  //   UpdateMenuState
  //   Update on all panels belonging to the UserInterfaceManager, if any
  virtual void Update();

  // Description:
  // Update the toolbar state
  virtual void UpdateToolbarState();

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
  // Deallocate/delete/reparent some internal objects in order to solve
  // reference loops that would prevent this instance from being deleted.
  virtual void PrepareForDelete();

  // Description:
  // Some constants
  vtkGetStringMacro(PrintOptionsMenuLabel);
  vtkGetStringMacro(FileMenuLabel);
  vtkGetStringMacro(FileCloseMenuLabel);
  vtkGetStringMacro(FileExitMenuLabel);
  vtkGetStringMacro(OpenRecentFileMenuLabel);
  vtkGetStringMacro(EditMenuLabel);
  vtkGetStringMacro(ViewMenuLabel);
  vtkGetStringMacro(WindowMenuLabel);
  vtkGetStringMacro(HelpMenuLabel);
  vtkGetStringMacro(HelpAboutMenuLabel);
  vtkGetStringMacro(HelpTopicsMenuLabel);
  vtkGetStringMacro(HelpCheckForUpdatesMenuLabel);
  vtkGetStringMacro(ToolbarsVisibilityMenuLabel);
  vtkGetStringMacro(WindowGeometryRegKey);
  vtkGetStringMacro(DefaultGeometry);

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ErrorIconCallback();
  virtual void PrintSettingsCallback() {};
  virtual void ToolbarVisibilityChangedCallback(vtkKWToolbar*);
  virtual void NumberOfToolbarsChangedCallback();

protected:
  vtkKWWindowBase();
  ~vtkKWWindowBase();

  // Description:
  // Insert a "Recent Files" sub-menu to the File menu at position 'pos'
  // and fill it with the most recent files stored in the registry.
  // The 'target' parameter is the object against which the command
  // associated to a most recent file will be executed (usually the instance).
  virtual void InsertRecentFilesMenu(int pos, vtkObject *target);

  // Description:
  // Display the close dialog.
  // Return 1 if the user wants to close the window, 0 otherwise
  virtual int DisplayCloseDialog();

  // Description:
  // Update the image in the status frame. Usually a logo of some sort.
  // Override this function to include your own application logo
  virtual void UpdateStatusImage();
  virtual vtkKWLabel *GetStatusImage();

  // Description:
  // Recent files manager
  vtkKWMostRecentFilesManager *MostRecentFilesManager;

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometryToRegistry();
  virtual void RestoreWindowGeometryFromRegistry();

  // Description:
  // Pack/repack the UI
  virtual void Pack();

  vtkKWSeparator *MenuBarSeparator;
  vtkKWFrame *MainFrame;

  vtkKWSeparator *StatusFrameSeparator;
  vtkKWFrame *StatusFrame;
  vtkKWLabel *StatusImage;
  vtkKWLabel *StatusLabel;

  vtkKWProgressGauge *ProgressGauge;
  int                ProgressGaugePosition;

  vtkKWFrame      *TrayFrame;
  vtkKWLabel      *TrayImageError;
  int             TrayFramePosition;

  vtkKWToolbarSet *MainToolbarSet;
  vtkKWToolbar    *StatusToolbar;

  char *ScriptExtension;
  char *ScriptType;
  int  SupportHelp;
  int  SupportPrint;
  int  PromptBeforeClose;
  int  StatusFrameVisibility;

  // Allocated and created when queried

  vtkKWMenu *FileMenu;
  vtkKWMenu *EditMenu;
  vtkKWMenu *ViewMenu;
  vtkKWMenu *WindowMenu;
  vtkKWMenu *HelpMenu;
  vtkKWMenu *ToolbarsVisibilityMenu;

  vtkKWTclInteractor *TclInteractor;

  // Description:
  // Some constants
  vtkSetStringMacro(PrintOptionsMenuLabel);
  vtkSetStringMacro(FileMenuLabel);
  vtkSetStringMacro(FileCloseMenuLabel);
  vtkSetStringMacro(FileExitMenuLabel);
  vtkSetStringMacro(OpenRecentFileMenuLabel);
  vtkSetStringMacro(EditMenuLabel);
  vtkSetStringMacro(ViewMenuLabel);
  vtkSetStringMacro(WindowMenuLabel);
  vtkSetStringMacro(HelpMenuLabel);
  vtkSetStringMacro(HelpTopicsMenuLabel);
  vtkSetStringMacro(HelpAboutMenuLabel);
  vtkSetStringMacro(HelpCheckForUpdatesMenuLabel);
  vtkSetStringMacro(ToolbarsVisibilityMenuLabel);
  vtkSetStringMacro(WindowGeometryRegKey);
  vtkSetStringMacro(DefaultGeometry);

private:

  // Description:
  // Some constants
  char *PrintOptionsMenuLabel;
  char *FileMenuLabel;
  char *FileCloseMenuLabel;
  char *FileExitMenuLabel;
  char *OpenRecentFileMenuLabel;
  char *EditMenuLabel;
  char *ViewMenuLabel;
  char *WindowMenuLabel;
  char *HelpMenuLabel;
  char *HelpTopicsMenuLabel;
  char *HelpAboutMenuLabel;
  char *HelpCheckForUpdatesMenuLabel;
  char *ToolbarsVisibilityMenuLabel;
  char *WindowGeometryRegKey;
  char *DefaultGeometry;

  vtkKWWindowBase(const vtkKWWindowBase&); // Not implemented
  void operator=(const vtkKWWindowBase&); // Not implemented
};

#endif
