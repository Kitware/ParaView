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

#include "vtkKWTopLevel.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLoadSaveDialog;
class vtkKWMenu;
class vtkKWMessageDialog;
class vtkKWNotebook;
class vtkKWProgressGauge;
class vtkKWSplitFrame;
class vtkKWTclInteractor;
class vtkKWToolbarSet;
class vtkKWToolbar;
class vtkKWUserInterfaceManager;
class vtkKWMostRecentFilesManager;

#define VTK_KW_PAGE_SETUP_MENU_LABEL            "Page Setup"
#define VTK_KW_RECENT_FILES_MENU_LABEL          "Open Recent File"

#define VTK_KW_WINDOW_GEOMETRY_REG_KEY          "WindowGeometry"
#define VTK_KW_WINDOW_FRAME1_SIZE_REG_KEY       "WindowFrame1Size"
#define VTK_KW_ENABLE_GUI_DRAG_AND_DROP_REG_KEY "EnableGUIDragAndDrop"
#define VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY       "ToolbarFlatFrame"
#define VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY     "ToolbarFlatButtons"

class VTK_EXPORT vtkKWWindow : public vtkKWTopLevel
{
public:
  static vtkKWWindow* New();
  vtkTypeRevisionMacro(vtkKWWindow,vtkKWTopLevel);
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
  // The window is divided into two parts (1 and 2) by the MainSplitFrame.
  // The first part is called the "main panel" area, and it
  // can be hidden or shown by setting its visibility flag.
  // A convenience method GetMainPanelFrame() can be used to retrive that
  // "main panel area".
  // The MainNotebook element is packed in the main panel and is used to
  // display interface elements organized as pages inside panels.
  vtkGetObjectMacro(MainSplitFrame, vtkKWSplitFrame);
  virtual vtkKWFrame* GetMainPanelFrame();
  virtual int GetMainPanelVisibility();
  virtual void SetMainPanelVisibility(int);
  vtkBooleanMacro(MainPanelVisibility, int );
  vtkGetObjectMacro(MainNotebook, vtkKWNotebook);

  // Description:
  // A convenience method to retrieve a pointer to the large frame available 
  // for viewing. In this implementation it defaults to the frame on
  // the right of the split bar (MainSplitFrame).
  virtual vtkKWFrame* GetViewFrame();

  // Description:
  // Get the menu objects.
  vtkGetObjectMacro(FileMenu,vtkKWMenu);
  vtkGetObjectMacro(HelpMenu,vtkKWMenu);
  vtkKWMenu *GetEditMenu();
  vtkKWMenu *GetViewMenu();
  vtkKWMenu *GetWindowMenu();
  
  // Description::
  // Add a "Recent Files" sub-menu to the File menu and fill it with the
  // most recent files stored in the registry.
  // - menuEntry is the name of the menu entry in the File menu above which 
  //   the sub-menu will be inserted. If NULL (or not found), the sub-menu
  //   will be inserted before the last entry in the File menu 
  //   (see GetFileMenuIndex)
  // - target is the object against which the command associated to a most
  //   recent file will be executed.
  // - label is an optional label that will be used for the sub-menu
  virtual void AddRecentFilesMenu(
    const char *menuEntry, 
    vtkKWObject *target, 
    const char *label = VTK_KW_RECENT_FILES_MENU_LABEL,
    int underline = 6);

  // Description::
  // Add a file to the Recent File list.
  virtual void AddRecentFile(
    const char *filename, vtkKWObject *target, const char *command);
  
  // Description:
  // Return the index of the entry above the last entry in the file menu.
  int GetFileMenuIndex();

  // Description:
  // Return the index of the entry above the "About" entry in the help menu.
  int GetHelpMenuIndex();

  // Description:
  // Callbacks used to set the print quality.
  void OnPrint(int propagate, int resolution);
  vtkGetMacro(PrintTargetDPI,float);
  
  // Description:
  // The toolbar container.
  vtkGetObjectMacro(Toolbars, vtkKWToolbarSet);

  // Description:
  // Add a toolbar to the window. Do not directly add the toolbar to
  // the vtkKWToolbarSet instance if
  // it must be made availble in the View | Toolbars menu to
  // toggle visibility. Also, 
  // use HideToolbar / ShowToolbar / SetToolbarVisibility methods
  // alone to change the visibility of the toolbar.
  // visibility is the default toolbar visibility used
  // if there is not rehistry entry for that toolbar.
  void AddToolbar(vtkKWToolbar* toolbar, const char* name, int visibility=1);

  // Description:
  // Change the visibility of a toolbar.
  void HideToolbar(vtkKWToolbar* toolbar, const char* name);
  void ShowToolbar(vtkKWToolbar* toolbar, const char* name);
  void SetToolbarVisibility(vtkKWToolbar* toolbar, const char* name, int flag);

  // Description:;
  // Callback to toggle toolbar visibility
  void ToggleToolbarVisibility(int id, const char* name);
  
  // Description:
  // The status frame.
  vtkGetObjectMacro(StatusFrame, vtkKWFrame);

  // Description:
  // Get the progress gauge widget.  The progress gauge is displayed
  // in the Status frame on the bottom right of the window.
  vtkGetObjectMacro(ProgressGauge, vtkKWProgressGauge);
 
  // Description:
  // Will the window add a help menu?
  vtkSetClampMacro( SupportHelp, int, 0, 1 );
  vtkGetMacro( SupportHelp, int );
  vtkBooleanMacro( SupportHelp, int );

  // Description:
  // Will the window add print entries in the file menu?
  vtkSetClampMacro( SupportPrint, int, 0, 1 );
  vtkGetMacro( SupportPrint, int );
  vtkBooleanMacro( SupportPrint, int );

  // Description:
  // Get title of window.
  // Override superclass to use app name if the title was not set
  virtual char* GetTitle();

  // Description:
  // Call render on all views
  virtual void Render();

  // Description:
  // Save or retrieve color from registry. If color does not 
  // exist, it will retrieve -1, -1 ,-1 and return 0 (1 if success).
  void SaveColor(int level, const char*, float rgb[3]);
  void SaveColor(int level, const char*, double rgb[3]);
  int RetrieveColor(int level, const char*, float rgb[3]);
  int RetrieveColor(int level, const char*, double rgb[3]);

  // Description:
  // Save or retrieve the last path of the dialog to the registry.
  // The string argument is the registry key.
  void SaveLastPath(vtkKWLoadSaveDialog *, const char*);
  void RetrieveLastPath(vtkKWLoadSaveDialog *, const char*);

//ETX
  
  // Description:
  // Get the User Interface Manager.
  virtual vtkKWUserInterfaceManager* GetUserInterfaceManager()
    { return 0; };

  // Description:
  // Display the tcl interactor.
  void DisplayCommandPrompt();
  
  // Description:
  // Access to the Tcl interactor.
  vtkGetObjectMacro(TclInteractor, vtkKWTclInteractor);

  // Description:
  // Check if the application needs to abort.
  virtual int CheckForOtherAbort() { return 0; }
  
  // Description:
  // Static method that processes the event. First argument is  
  // the calling object, second is event id, third is a pointer to
  // the windows and last is the arguments of the event.
  static void ProcessEvent(vtkObject *, unsigned long, void *, void *);

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
  virtual void MainPanelVisibilityCallback();

protected:
  vtkKWWindow();
  ~vtkKWWindow();

  // Description:
  // Display the close dialog.
  // Return 1 if the user wants to close the window, 0 otherwise
  virtual int DisplayCloseDialog();

  // Description:
  // Update the image in the status frame. Usually a logo of some sort.
  virtual void UpdateStatusImage();

  // Description:
  // Add the toolbar to the menu alone.
  void AddToolbarToMenu(vtkKWToolbar* toolbar, const char* name, 
    vtkKWWidget* target, const char* command);

  void SetToolbarVisibilityInternal(vtkKWToolbar* toolbar,const char* name, int flag);

  // Recent files

  vtkKWMostRecentFilesManager *MostRecentFilesManager;

  // Description:
  // Process events
  virtual void InternalProcessEvent(
    vtkObject *, unsigned long, float *, void *);

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometry();
  virtual void RestoreWindowGeometry();

  vtkKWNotebook *MainNotebook;

  vtkKWSplitFrame *MainSplitFrame;

  vtkKWMenu *FileMenu;
  vtkKWMenu *EditMenu;
  vtkKWMenu *ViewMenu;
  vtkKWMenu *WindowMenu;
  vtkKWMenu *HelpMenu;
  vtkKWMenu *PageMenu;
  vtkKWMenu *ToolbarsMenu;

  vtkKWFrame *StatusFrameSeparator;
  vtkKWFrame *StatusFrame;
  vtkKWLabel *StatusImage;
  vtkKWLabel *StatusLabel;

  vtkKWFrame         *ProgressFrame;
  vtkKWProgressGauge *ProgressGauge;

  vtkKWFrame      *TrayFrame;
  vtkKWLabel      *TrayImageError;

  vtkKWToolbarSet *Toolbars;

  vtkKWFrame *MenuBarSeparatorFrame;


  float PrintTargetDPI;
  char  *ScriptExtension;
  char  *ScriptType;
  int   SupportHelp;
  int   SupportPrint;
  int   PromptBeforeClose;

  vtkKWTclInteractor *TclInteractor;

private:
  vtkKWWindow(const vtkKWWindow&); // Not implemented
  void operator=(const vtkKWWindow&); // Not implemented
};

#endif

