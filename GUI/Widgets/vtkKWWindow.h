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

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWApplicationSettingsInterface;
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
class vtkKWMostRecentFilesUtilities;

#define VTK_KW_PAGE_SETUP_MENU_LABEL      "Page Setup"
#define VTK_KW_RECENT_FILES_MENU_LABEL    "Open Recent File"
#define VTK_KW_EXIT_DIALOG_NAME           "ExitApplication"
#define VTK_KW_WINDOW_GEOMETRY_REG_KEY    "WindowGeometry"
#define VTK_KW_WINDOW_FRAME1_SIZE_REG_KEY "WindowFrame1Size"

class VTK_EXPORT vtkKWWindow : public vtkKWWidget
{
public:
  static vtkKWWindow* New();
  vtkTypeRevisionMacro(vtkKWWindow,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description::
  // Exit this application closing all windows.
  virtual void Exit();

  // Description::
  // Close this window, possibly exiting the application if no more
  // windows are open.
  virtual void Close();
  virtual void CloseNoPrompt();

  // Description::
  // Display help info for this window.
  virtual void DisplayHelp();

  // Description::
  // Display about info for this window.
  virtual void DisplayAbout();

  // Description:
  // Set the text for the status bar of this window.
  void SetStatusText(const char *);
  const char *GetStatusText();
  
  // Description:
  // Load in a Tcl based script to drive the application. If called
  // without an argument it will open a file dialog.
  virtual void LoadScript();
  virtual void LoadScript(const char *name);

  // Description:
  // Popup the vtk warning message
  virtual void WarningMessage(const char* message);
  
  // Description:
  // Popup the vtk error message
  virtual void ErrorMessage(const char* message);

  // Description:
  // Show or hide the error / warning icon in the tray.
  // 2 - red icon, 1 - icon, 0 - hide.
  virtual void SetErrorIcon(int);
  vtkBooleanMacro(ErrorIcon, int); 

  // Description:
  // Process the click on the error icon.
  virtual void ProcessErrorClick();
  
  // Description:
  // Allow windows to get at the different menu entries. In some
  // cases the menu entry may be created if it doesn't already
  // exist.
  vtkGetObjectMacro(Menu,vtkKWMenu);
  vtkGetObjectMacro(MenuFile,vtkKWMenu);
  vtkKWMenu *GetMenuEdit();
  vtkKWMenu *GetMenuView();
  vtkKWMenu *GetMenuWindow();
  
  // Description:
  // Operations on the views.
  vtkGetObjectMacro(ViewFrame,vtkKWFrame);
  
  // Description:
  // Proiperties may be bound to the window or view or
  // something else. The CreateDefaultPropertiesParent method
  // will create an attachment point for the properties at
  // the window level.
  vtkGetObjectMacro(PropertiesParent,vtkKWWidget);
  void SetPropertiesParent(vtkKWWidget*);
  void CreateDefaultPropertiesParent();

  // Description:
  // Provide hide/show functionality of properties
  int GetPropertiesVisiblity();
  void SetPropertiesVisiblity(int);
  void HideProperties() { this->SetPropertiesVisiblity(0); };
  void ShowProperties() { this->SetPropertiesVisiblity(1); };
  void TogglePropertiesVisibilityCallback();
  
  // Description:
  // Callback to display window properties (usually, application settings)
  void ShowWindowProperties();

  // Description::
  // Override Unregister since widgets have loops.
  virtual void UnRegister(vtkObjectBase *o);

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
  // Install a menu bar into this window.
  void InstallMenu(vtkKWMenu* menu);

  // Description:
  // Callbacks used to set the print quality.
  void OnPrint(int propagate, int resolution);
  vtkGetMacro(PrintTargetDPI,float);
  
  // Description:
  // Allow access to the notebook object.
  vtkGetObjectMacro(Notebook,vtkKWNotebook);
  virtual void ShowMostRecentPanels(int);

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
  // Class of the window. Passed to the toplevel command.
  vtkSetStringMacro(WindowClass);
  vtkGetStringMacro(WindowClass);

  // Description:
  // Title of the window (if empty, try to use the app name). 
  virtual void SetTitle(const char*);
  virtual char* GetTitle();

  //Description:
  // Set/Get PromptBeforeClose
  vtkSetMacro(PromptBeforeClose, int);
  vtkGetMacro(PromptBeforeClose, int);

  // Description:
  // The extension used in LoadScript. Default is .tcl.
  vtkSetStringMacro(ScriptExtension);
  vtkGetStringMacro(ScriptExtension);

  // Description:
  // The type name used in LoadScript. Default is Tcl.
  vtkSetStringMacro(ScriptType);
  vtkGetStringMacro(ScriptType);

  // Description:
  // Call render on all views
  virtual void Render();

  //BTX
  //Description:
  // Set or get the registry value for the application.
  // When storing multiple arguments, separate with spaces.
  // If the level is lower than current registry level, operation 
  // will be successfull.
  /*
  int SetWindowRegistryValue(int level, const char* subkey, const char* key, 
                        const char* format, ...);
  int GetWindowRegistryValue(int level, const char* subkey, const char* key, 
                        char*value);
  int DeleteRegistryValue(int level, const char* subkey, const char* key);
  */
  
  // Description:
  // Get float registry value (zero if not found).
  // If the level is lower than current registry level, operation 
  // will be successfull.
  float GetFloatRegistryValue(int level, const char* subkey, 
                               const char* key);
  int   GetIntRegistryValue(int level, const char* subkey, const char* key);

  // Description:
  // Perform a boolean check of the value in registry. If the value 
  // at the key is trueval, then return true, otherwise return false.
  int BooleanRegistryCheck(int level, const char* subkey, const char* key, const char* trueval);
  
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
  // Get/Show the Application Settings Interface. 
  virtual vtkKWApplicationSettingsInterface* GetApplicationSettingsInterface() 
    { return 0; };
  int ShowApplicationSettingsInterface();

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
  // Update the toolbar aspect once the toolbar settings have been changed
  virtual void UpdateToolbarAspect();

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
  // Get rid of all references we own.
  virtual void PrepareForDelete() {}

protected:
  vtkKWWindow();
  ~vtkKWWindow();

  // Description:
  // Add the toolbar to the menu alone.
  void AddToolbarToMenu(vtkKWToolbar* toolbar, const char* name, 
    vtkKWWidget* target, const char* command);

  void SetToolbarVisibilityInternal(vtkKWToolbar* toolbar,const char* name, int flag);

  // Recent files

  vtkKWMostRecentFilesUtilities *MostRecentFilesUtilities;

  // Description:
  // Display the exit dialog.
  int ExitDialog();

  // Description:
  // Process events
  virtual void InternalProcessEvent(
    vtkObject *, unsigned long, float *, void *);

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometry();
  virtual void RestoreWindowGeometry();

  vtkKWNotebook *Notebook;

  vtkKWSplitFrame     *MiddleFrame; // Contains view frame & properties parent.

  vtkKWMenu *Menu;
  vtkKWMenu *MenuFile;
  vtkKWMenu *MenuEdit;
  vtkKWMenu *MenuView;
  vtkKWMenu *MenuWindow;
  vtkKWMenu *MenuHelp;
  vtkKWMenu *PageMenu;
  vtkKWMenu *ToolbarsMenu;

  vtkKWFrame *StatusFrameSeparator;
  vtkKWFrame *StatusFrame;
  vtkKWLabel *StatusImage;

  vtkKWProgressGauge *ProgressGauge;
  vtkKWFrame         *ProgressFrame;

  vtkKWFrame      *TrayFrame;
  vtkKWLabel      *TrayImageError;

  vtkKWLabel *StatusLabel;
  char       *StatusImageName;
  vtkSetStringMacro(StatusImageName);

  virtual void CreateStatusImage() {};

  vtkKWWidget *PropertiesParent;
  vtkKWFrame *ViewFrame;
  vtkKWToolbarSet *Toolbars;

  vtkKWFrame *MenuBarSeparatorFrame;

  vtkKWMessageDialog *ExitDialogWidget;

  float PrintTargetDPI;
  char  *ScriptExtension;
  char  *ScriptType;
  int   SupportHelp;
  int   SupportPrint;
  char  *WindowClass;
  char  *Title;
  int   PromptBeforeClose;
  int   InExit;

  vtkKWTclInteractor *TclInteractor;

private:
  vtkKWWindow(const vtkKWWindow&); // Not implemented
  void operator=(const vtkKWWindow&); // Not implemented
};

#endif

