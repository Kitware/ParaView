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

class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenu;
class vtkKWProgressGauge;
class vtkKWTclInteractor;
class vtkKWToolbarSet;
class vtkKWMostRecentFilesManager;

class VTK_EXPORT vtkKWWindowBase : public vtkKWTopLevel
{
public:

  static vtkKWWindowBase* New();
  vtkTypeRevisionMacro(vtkKWWindowBase,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the window
  virtual void Create(vtkKWApplication *app, const char *args);

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
  // Convenience methods to popup a warning/error message.
  // This can be overriden in a subclass to redirect errors and warnings
  // to log files or more elaborate log windows.
  virtual void WarningMessage(const char* message);
  virtual void ErrorMessage(const char* message);

  // Description:
  // Show or hide the error / warning icon in the tray.
  //BTX
  enum 
  {
    ERROR_ICON_NONE = 0,
    ERROR_ICON_BLACK,
    ERROR_ICON_RED
  };
  //ETX
  virtual void SetErrorIcon(int);

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
  // can be used by users or developpers to add more UI, without knowing
  // about the current layout.
  virtual vtkKWFrame* GetViewFrame();

  // Description:
  // Get the toolbar set.
  vtkGetObjectMacro(Toolbars, vtkKWToolbarSet);

  // Description:
  // Get the status frame object.
  vtkGetObjectMacro(StatusFrame, vtkKWFrame);

  // Description:
  // Get the progress gauge widget.  The progress gauge is displayed
  // in the Status frame on the bottom right corner of the window.
  vtkGetObjectMacro(ProgressGauge, vtkKWProgressGauge);

  // Description:
  // Get the menu objects.
  vtkGetObjectMacro(FileMenu,vtkKWMenu);
  vtkGetObjectMacro(HelpMenu,vtkKWMenu);
  vtkKWMenu *GetEditMenu();
  vtkKWMenu *GetViewMenu();
  vtkKWMenu *GetWindowMenu();
  vtkKWMenu *GetToolbarsVisibilityMenu();
  
  // Description:
  // Convenience method that return the position where to safely insert 
  // entries in the file menu without interferring with entries that should
  // stay at the end of the menu (at the moment, it checks for the 'close',
  // 'exit' or 'print setup' commands).
  int GetFileMenuInsertPosition();

  // Description:
  // Convenience method that return the position where to safely insert 
  // entries in the help menu without interferring with entries that should
  // stay at the end of the menu (at the moment, it checks for the 'about'
  // commands).
  int GetHelpMenuInsertPosition();

  // Description:
  // Does the window support help (if false, no help entries 
  // should be added to the  menus)
  vtkSetClampMacro(SupportHelp, int, 0, 1);
  vtkGetMacro(SupportHelp, int);
  vtkBooleanMacro(SupportHelp, int);

  // Description:
  // Add a file to the Recent File list, and save the whole list to
  // the registry.
  // If the "Recent files" sub-menu has been inserted at that point (see
  // the InsertRecentFilesMenu method), it will be updated as well.
  virtual void AddRecentFile(
    const char *filename, vtkKWObject *target, const char *command);
  
  // Description:
  // Does the window support print (if false, no print-related entries 
  // will/should be added to the  menus)
  vtkSetClampMacro(SupportPrint, int, 0, 1);
  vtkGetMacro(SupportPrint, int);
  vtkBooleanMacro(SupportPrint, int);

  // Description:
  // Get title of window.
  // Override the superclass to use app name if the title was not set
  virtual char* GetTitle();

  // Description:
  // Display the tcl interactor.
  virtual void DisplayTclInteractor();
  
  // Description:
  // Get the Tcl interactor object.
  vtkGetObjectMacro(TclInteractor, vtkKWTclInteractor);

  // Description:
  // Update the UI. This will call:
  //   UpdateToolbarState
  //   UpdateEnableState 
  //   UpdateMenuState
  //   Update on all panels belonging to the GetUserInterfaceManager, if any
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
  // Callbacks.
  // Process the click on the error icon.
  // Override it in subclasses to popup more elaborate log/error dialog.
  virtual void ErrorIconCallback();
  virtual void PrintOptionsCallback() {};
  virtual void ToolbarVisibilityChangedCallback();
  virtual void NumberOfToolbarsChangedCallback();

  // Description:
  // Some constants
  //BTX
  static const char *PrintOptionsMenuLabel;
  static const char *GetPrintOptionsMenuLabel();
  static const char *WindowGeometryRegKey;
  static const unsigned int DefaultWidth;
  static const unsigned int DefaultHeight;
  //ETX

protected:
  vtkKWWindowBase();
  ~vtkKWWindowBase();

  // Description:
  // Insert a "Recent Files" sub-menu to the File menu at position 'pos'
  // and fill it with the most recent files stored in the registry.
  // The 'target' parameter is the object against which the command
  // associated to a most recent file will be executed (usually the instance).
  virtual void InsertRecentFilesMenu(int pos, vtkKWObject *target);

  // Description:
  // Display the close dialog.
  // Return 1 if the user wants to close the window, 0 otherwise
  virtual int DisplayCloseDialog();

  // Description:
  // Update the image in the status frame. Usually a logo of some sort.
  virtual void UpdateStatusImage();

  // Description:
  // Recent files manager
  vtkKWMostRecentFilesManager *MostRecentFilesManager;

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometryToRegistry();
  virtual void RestoreWindowGeometryFromRegistry();

  vtkKWMenu *FileMenu;
  vtkKWMenu *EditMenu;
  vtkKWMenu *ViewMenu;
  vtkKWMenu *WindowMenu;
  vtkKWMenu *HelpMenu;
  vtkKWMenu *ToolbarsVisibilityMenu;

  vtkKWFrame *MenuBarSeparatorFrame;

  vtkKWFrame *MainFrame;

  vtkKWFrame *StatusFrameSeparator;
  vtkKWFrame *StatusFrame;
  vtkKWLabel *StatusImage;
  vtkKWLabel *StatusLabel;

  vtkKWFrame         *ProgressFrame;
  vtkKWProgressGauge *ProgressGauge;

  vtkKWFrame      *TrayFrame;
  vtkKWLabel      *TrayImageError;

  vtkKWToolbarSet *Toolbars;

  char  *ScriptExtension;
  char  *ScriptType;
  int   SupportHelp;
  int   SupportPrint;
  int   PromptBeforeClose;

  vtkKWTclInteractor *TclInteractor;

private:
  vtkKWWindowBase(const vtkKWWindowBase&); // Not implemented
  void operator=(const vtkKWWindowBase&); // Not implemented
};

#endif

