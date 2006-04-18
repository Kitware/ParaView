/*=========================================================================

  Module:    vtkKWApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWApplication - an application class
// .SECTION Description
// vtkKWApplication is the overall class that represents the entire 
// application. It is also responsible for managing the vtkKWWindowBase(s) 
// associated to the application.

#ifndef __vtkKWApplication_h
#define __vtkKWApplication_h

#include "vtkKWObject.h"

#include "vtkTcl.h" // Needed for Tcl_Interp
#include "vtkTk.h"  // Needed for Tk_Window

class vtkKWApplicationInternals;
class vtkKWBalloonHelpManager;
class vtkKWLabel;
class vtkKWLoadSaveDialog;
class vtkKWMessageDialog;
class vtkKWOptionDataBase;
class vtkKWRegistryHelper;
class vtkKWSplashScreen;
class vtkKWText;
class vtkKWTextWithScrollbars;
class vtkKWTheme;
class vtkKWWidget;
class vtkKWWindowBase;

class KWWidgets_EXPORT vtkKWApplication : public vtkKWObject
{
public:
  static vtkKWApplication* New();
  vtkTypeRevisionMacro(vtkKWApplication,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Override vtkKWObject's method. A vtkKWObject is associated to a
  // vtkKWApplication. Even if vtkKWApplication is a subclass of 
  // vtkKWObject, an application's application is actually 'itself', 
  // and it can not be reset.
  virtual vtkKWApplication *GetApplication()  { return this;  }
  virtual void SetApplication (vtkKWApplication*);
  
  // Description:
  // Start running the application, with or without arguments, and enter the
  // event loop. The application will exit the event loop once every 
  // windows has been closed.
  // As a convenience, if one (or more) window has been added to the 
  // application (using AddWindow()), and none of them has been mapped
  // on the screen yet, the application will automatically display the first
  // window by calling its Display() method.
  virtual void Start();
  virtual void Start(int argc, char *argv[]);

  // Description:
  // This method is invoked when the user exits the app
  // Return 1 if the app exited successfully, 0 otherwise (for example,
  // if some dialogs are still up, or the user did not confirm, etc).
  virtual int Exit();

  // Description:
  // Set/Get if a confirmation dialog should be displayed before the
  // application is exited.
  vtkSetMacro(PromptBeforeExit, int);
  vtkGetMacro(PromptBeforeExit, int);
  vtkBooleanMacro(PromptBeforeExit, int);

  // Description:
  // Set/Get the value returned by the application at exit.
  // This can be used from scripts to set an error status
  vtkSetMacro(ExitStatus, int);
  vtkGetMacro(ExitStatus, int);

  // Description:
  // Get when application is exiting (set to 1 as soon as Exit() is called).
  vtkGetMacro(InExit, int);

  // Description:
  // Add/Close a window to/of this application.
  // Note that AddWindow() will increase the reference count of the window
  // that is added, RemoveWindow() will decrease it. Once the last window is
  // closed, Exit() is called.
  // Return 1 if successful, 0 otherwise
  virtual int AddWindow(vtkKWWindowBase *w);
  virtual int RemoveWindow(vtkKWWindowBase *);

  // Description:
  // Get the number of windows, retrieve a window
  virtual int GetNumberOfWindows();
  virtual vtkKWWindowBase* GetNthWindow(int rank);

  // Description:
  // Set/Get the application name.
  // Also check the LimitedEditionModeName variable if you plan on running
  // the application in limited edition mode.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Get the major and minor application version.
  vtkSetMacro(MajorVersion, int);
  vtkGetMacro(MajorVersion, int);
  vtkSetMacro(MinorVersion, int);
  vtkGetMacro(MinorVersion, int);

  // Description:
  // Set/Get the application version name - this usually is the application 
  // name postfixed with the version number (major/minor).
  // It is typically used as the master key to store registry settings
  // (ex: VolView 3.0, ParaView1.1, etc.)
  // If it has not been set, it will use the value of Name and append
  // the major/minor version.
  vtkSetStringMacro(VersionName);
  virtual const char* GetVersionName();

  // Description:
  // Set/Get the application release name - this is the release of the 
  // application version (if any), typically: beta1, beta2, final, patch1, etc.
  vtkSetStringMacro(ReleaseName);
  vtkGetStringMacro(ReleaseName);

  // Description:
  // Get the "pretty" name of the application. 
  // This is typically used for windows or dialogs title, About boxes, etc. 
  // It combines the application name, its version, and other relevant
  // informations (like its limited edition mode).
  virtual const char* GetPrettyName();

  // Descrition:
  // Set/Get if the application is running in limited edition mode.
  // This can be used throughout the whole UI to enable or disable
  // features on the fly. Make sure it is *not* wrapped !
  //BTX 
  virtual void SetLimitedEditionMode(int arg);
  vtkBooleanMacro(LimitedEditionMode, int);
  vtkGetMacro(LimitedEditionMode, int);
  //ETX

  // Descrition:
  // Return the limited edition mode and optionally warn the user ; 
  // if the limited edition mode is true, display a popup warning stating
  // that 'feature' is not available in this mode.
  virtual int GetLimitedEditionModeAndWarn(const char *feature);

  // Descrition:
  // Set/Get the name of the application when it runs in limited edition mode.
  // This is used by GetPrettyName() for example, instead of the Name variable.
  // If it has not been set, it will use the value of Name and append
  // the "Limited Edition" to it.
  vtkSetStringMacro(LimitedEditionModeName);
  virtual const char *GetLimitedEditionModeName();

  // Description:
  // Set/Get the directory in which the application is supposed
  // to be installed. 
  vtkGetStringMacro(InstallationDirectory);
  vtkSetStringMacro(InstallationDirectory);
  
  // Description:
  // Set/Get the directory in which the application can store
  // user data. 
  virtual char* GetUserDataDirectory();
  vtkSetStringMacro(UserDataDirectory);
  
  // Description:
  // Load and evaluate a Tcl script from a file. 
  // Return 1 if successful, 0 otherwise
  virtual int LoadScript(const char* filename);

  // Description:
  // Set/Get the "exit after load script" flag. If this flag is set, then 
  // the application will automatically Exit() after a call to LoadScript(). 
  // This is mainly used for testing purposes. Even though a Tcl script
  // can end with an explicit call to Exit on the application Tcl object,
  // this call may never be reached it the script contains an error. Setting
  // this variable will make sure the application will exit anyway.
  vtkSetClampMacro(ExitAfterLoadScript, int, 0, 1);
  vtkBooleanMacro(ExitAfterLoadScript, int);
  vtkGetMacro(ExitAfterLoadScript, int);

  // Description:
  // Set/Get the print quality.
  vtkGetMacro(PrintTargetDPI, double);
  vtkSetMacro(PrintTargetDPI, double);
  
  // Description:
  // Get the Registry object.
  //BTX
  vtkKWRegistryHelper *GetRegistryHelper();
  //ETX

  // Description:
  // Set/Get the current registry level. 
  // When setting/retrieving a value in/from the registry a 'level' has
  // to be provided as part of the parameters. If this level is greater
  // than the current registry level, the operation will be ignored.
  // Set the registry level to -1 means to ignore all the registry operations.
  vtkSetClampMacro(RegistryLevel, int, -1, 10);
  vtkGetMacro(RegistryLevel, int);

  // Description:
  // Set/Get/Delete/Query a registry value for the application.
  // When storing multiple arguments, separate them with spaces.
  // Note that if the 'level' is greater than the current registry level, 
  // the operation will be ignored.
  //BTX
  virtual int SetRegistryValue(
    int level, const char* subkey, const char* key, 
    const char* format, ...);
  //ETX
  virtual int GetRegistryValue(
    int level, const char* subkey, const char* key, char* value);
  virtual int DeleteRegistryValue(
    int level, const char* subkey, const char* key);
  virtual int HasRegistryValue(
    int level, const char* subkey, const char* key);
  
  // Description:
  // Retrieve a value from the registry and convert it to a type
  // (boolean, float, int). 
  // Return 0 if the value was not found.
  // For GetBooleanRegistryValue(), perform a boolean check of the value in
  // the registry. If the value at the key is equal to 'trueval', then return
  // true, otherwise return false.
  virtual float GetFloatRegistryValue(
    int level, const char* subkey, const char* key);
  virtual int GetIntRegistryValue(
    int level, const char* subkey, const char* key);
  virtual int GetBooleanRegistryValue(
    int level, const char* subkey, const char* key, const char* trueval);
  
  // Description:
  // Save/retrieve color to/from the registry. 
  // If the color does not exist, it will retrieve -1, -1 ,-1 and return 0
  // (1 if success).
  // Note that the subkey used here is "Colors".
  virtual void SaveColorRegistryValue(
    int level, const char *key, double rgb[3]);
  virtual int RetrieveColorRegistryValue(
    int level, const char *key, double rgb[3]);

  // Descrition:
  // Save/Retrieve the application settings to/from registry.
  // Do not call that method before the application name is known and the
  // proper registry level set (if any).
  virtual void RestoreApplicationSettingsFromRegistry();
  virtual void SaveApplicationSettingsToRegistry();

  // Description:
  // Get the database option object.
  //BTX
  vtkKWOptionDataBase *GetOptionDataBase();
  //ETX

  // Description:
  // Set/Get if this application supports a splash screen
  vtkSetMacro(SupportSplashScreen, int);
  vtkGetMacro(SupportSplashScreen, int);
  vtkBooleanMacro(SupportSplashScreen, int);

  // Description:
  // Set/Get if this application should show the splash screen at startup
  vtkGetMacro(SplashScreenVisibility, int);
  vtkSetMacro(SplashScreenVisibility, int);
  vtkBooleanMacro(SplashScreenVisibility, int);

  // Description:
  // Retrieve the splash screen object
  // This will also call vtkKWSplashScreen::Create() to create the splash
  // screen widget itself.
  virtual vtkKWSplashScreen* GetSplashScreen();

  // Description:
  // Set/Get if the user interface geometry should be saved (to the registry,
  // for example).
  // This is more like a hint that many widgets can query to check if
  // they should save their own geometry (and restore it on startup). 
  // See vtkKWWindowBase for example.
  vtkGetMacro(SaveUserInterfaceGeometry, int);
  vtkSetMacro(SaveUserInterfaceGeometry, int);
  vtkBooleanMacro(SaveUserInterfaceGeometry, int);

  // Description:
  // Get/Set the internal character encoding of the application.
  virtual void SetCharacterEncoding(int val);
  vtkGetMacro(CharacterEncoding, int);
  
  // Description:
  // Get if we have some logic to check for application update online and
  // perform that check.
  virtual int HasCheckForUpdates();
  virtual void CheckForUpdates();

  // Description:
  // Get/Set the current theme. This will install the theme automatically.
  virtual void SetTheme(vtkKWTheme *theme);
  vtkGetObjectMacro(Theme, vtkKWTheme);
  
  // Description:
  // Get if we have some logic to report feedback by email and
  // email that feedback.
  // Set/Get the email address to send that feedback to.
  virtual int CanEmailFeedback();
  virtual void EmailFeedback();
  vtkSetStringMacro(EmailFeedbackAddress);
  vtkGetStringMacro(EmailFeedbackAddress);

  // Description:
  // Send email (win32 only for the moment, use MAPI).
  virtual int SendEmail(
    const char *to,
    const char *subject,
    const char *message,
    const char *attachment_filename,
    const char *extra_error_msg = NULL);

  // Description:
  // Display the on-line help for this application.
  // Optionally provide a master window this dialog should be the slave of.
  virtual void DisplayHelpDialog(vtkKWWindowBase *master);

  // Description:
  // Set/Get the help starting page.
  // If set to a CHM/HTML page, it will be opened automatically on Windows.
  vtkGetStringMacro(HelpDialogStartingPage);
  vtkSetStringMacro(HelpDialogStartingPage);

  // Description:
  // Display the about dialog for this application.
  // Optionally provide a master window this dialog should be the slave of.
  virtual void DisplayAboutDialog(vtkKWWindowBase *master);

  // Description:
  // Return the Balloon Help helper object. 
  vtkKWBalloonHelpManager *GetBalloonHelpManager();

  // Description:
  // Evaluate Tcl script/code and perform argument substitutions.
  //BTX
  virtual const char* Script(const char* format, ...);
  int EvaluateBooleanExpression(const char* format, ...);
  //ETX
  
  // Description:
  // Get the interpreter being used by this application
  Tcl_Interp *GetMainInterp() {return this->MainInterp;};

  // Description:
  // Initialize Tcl/Tk
  // Return NULL on error (eventually provides an ostream where detailed
  // error messages will be stored).
  // One method takes argc/argv and will create an internal Tcl interpreter
  // on the fly, the other takes a Tcl interpreter and uses it afterward
  // (this is mainly intended for initialization as a Tcl package)
  //BTX
  static Tcl_Interp *InitializeTcl(int argc, char *argv[], ostream *err = 0);
  static Tcl_Interp *InitializeTcl(Tcl_Interp *interp, ostream *err = 0);
  //ETX

  // Description:
  // Call RegisterDialogUp to notify the application that a modal dialog is up,
  // and UnRegisterDialogUp when it is not anymore. IsDialogUp will return
  // if any dialog is up. 
  // The parameter to pass is a pointer to the dialog/toplevel/widget that is
  // being registered/unregistered. If there is no such widget (say, if you
  // are calling a builtin Tk function that creates and pops-up a dialog), pass
  // the adress of the class that is invoking that call.
  // This is used to help preventing a window or an
  // application to exit while a dialog is still up. This is usually not
  // a problem on Win32, since a modal dialog will prevent the user from
  // interacting with the window and exit it, but this is not the case for
  // other operating system where the window manager is independent from the
  // window contents itself. In any case, inheriting from a vtkKWTopLevel
  // or vtkKWDialog should take care of calling this function for you.
  virtual void RegisterDialogUp(vtkKWWidget *ptr);
  virtual void UnRegisterDialogUp(vtkKWWidget *ptr);
  virtual int IsDialogUp();
  
  // Description:
  // Open a link (media).
  // On Win32, use ShellExecute to trigger the default viewers.
  static int OpenLink(const char *link);

  // Description:
  // Explore link.
  // On Win32, this will launch the Explorer, open it in the directory
  // of the link, and eventually select that link itself in the directory.
  static int ExploreLink(const char *link);

  // Description:
  // Process/update pending events. This method brings the 
  // application "up to date" by entering the event loop repeatedly until
  // all pending events (including idle callbacks) have been processed. 
  virtual void ProcessPendingEvents();

  // Description:
  // Process/update idle tasks. This causes operations that are normally 
  // deferred, such as display updates and window layout calculations, to be
  // performed immediately. 
  virtual void ProcessIdleTasks();

  // Description:
  // Install the Tcl background error callback. Individual applications
  // can define a background error command if they wish to handle background
  // errors. A background error is one that occurs in an event handler or
  // some other command that didn't originate with the application. For
  // example, if an error occurs while executing a command specified with
  // asynchronously. The default implementation is to feed the Tcl error
  // message to a vtkErrorMacro.
  virtual void InstallTclBgErrorCallback();

  // Description:
  // Some constants
  //BTX
  static const char *ExitDialogName;
  static const char *BalloonHelpVisibilityRegKey;
  static const char *SaveUserInterfaceGeometryRegKey;
  static const char *SplashScreenVisibilityRegKey;
  static const char *PrintTargetDPIRegKey;
  //ETX

  // Description:
  // Callbacks. Internal, do not use.
  virtual void TclBgErrorCallback(const char* message);

protected:
  vtkKWApplication();
  ~vtkKWApplication();

  Tcl_Interp *MainInterp;

  // Description:
  // Do one tcl event and enter the event loop, allowing the application
  // interface to actually run.
  virtual void DoOneTclEvent();

  // Description:
  // Application installation directory
  char *InstallationDirectory;
  virtual void FindInstallationDirectory();

  // Description:
  // User data directory
  char *UserDataDirectory;

  // Description:
  // Add email feedback body and subject to output stream.
  // Override this function in subclasses (and/or call the superclass) to
  // add more information.
  virtual void AddEmailFeedbackBody(ostream &);
  virtual void AddEmailFeedbackSubject(ostream &);
  char *EmailFeedbackAddress;

  // Description:
  // On-line help starting page
  char *HelpDialogStartingPage;

  // Description:
  // Display the exit dialog.
  // Optionally provide a master window this dialog should be the slave of.
  // Return 1 if the user wants to exit, 0 otherwise
  virtual int DisplayExitDialog(vtkKWWindowBase *master);

  // Description:
  // Value that is set after exit (status), flag stating that 
  // Exit was called, flag stating if application should exit after load script
  int ExitStatus;
  int InExit;
  int ExitAfterLoadScript;
  int PromptBeforeExit;

  // Description:
  // Number of dialog that are up. See Un/RegisterDialogUp().
  int DialogUp;

  // Description:
  // Registry level. If a call to Set/GetRegistryValue uses a level above
  // this ivar, the operation is ignored.
  int RegistryLevel;

  // Description:
  // Flag stating if application supports splash screen, and shows it
  int SupportSplashScreen;
  int SplashScreenVisibility;
  virtual void CreateSplashScreen() {};

  // Description:
  // Flag stating if the UI geometry should be saved before exiting
  int SaveUserInterfaceGeometry;

  // Description:
  // About dialog, add text and copyrights to the about dialog.
  // Override this function in subclasses (and/or call the superclass) to
  // add more information.
  virtual void ConfigureAboutDialog();
  virtual void AddAboutText(ostream &);
  virtual void AddAboutCopyrights(ostream &);
  vtkKWMessageDialog *AboutDialog;
  vtkKWLabel         *AboutDialogImage;
  vtkKWTextWithScrollbars *AboutRuntimeInfo;

  // Description:
  // Character encoding (is passed to Tcl)
  int CharacterEncoding;

  // Description:
  // Print DPI
  double PrintTargetDPI;

  // Description:
  // Current theme
  vtkKWTheme *Theme;

  // Description:
  // Check for an argument (example: --foo, /C, -bar, etc).
  // Return VTK_OK if found and set 'index' to the position of the 
  // argument in argv[].
  // Return VTK_ERROR if not found.
  static int CheckForArgument(
    int argc, char* argv[], const char *arg, int &index);

  // Description:
  // Check for a valued argument (example: --foo=bar, /C=bar, -bar=foo, etc).
  // Return VTK_OK if found and set 'index' to the position of the 
  // argument in argv[], 'value_pos' to the position right after the '='
  // in that argument.
  // Return VTK_ERROR if not found.
  static int CheckForValuedArgument(
    int argc, char* argv[], const char *arg, int &index, int &value_pos);

  // Description:
  // Try to find the path to the online updater (for example, WiseUpdt.exe)
  // and output that path to the ostream passed as parameter.
  virtual int GetCheckForUpdatesPath(ostream &path);

  // Description:
  // Deallocate/delete/reparent some internal objects in order to solve
  // reference loops that would prevent this instance from being deleted.
  virtual void PrepareForDelete();

  // PIMPL Encapsulation for STL containers

  vtkKWApplicationInternals *Internals;

private:

  vtkKWRegistryHelper *RegistryHelper;
  vtkKWOptionDataBase *OptionDataBase;
  vtkKWSplashScreen *SplashScreen;
  vtkKWBalloonHelpManager *BalloonHelpManager;

  // Description:
  // Application name and version
  char *Name;
  char *VersionName;
  char *ReleaseName;
  int MajorVersion;
  int MinorVersion;
  char *PrettyName;
  vtkSetStringMacro(PrettyName);

  // Description:
  // Limited edition mode, name of the application when in limited edition mode
  int LimitedEditionMode;
  char *LimitedEditionModeName;

  vtkKWApplication(const vtkKWApplication&);   // Not implemented.
  void operator=(const vtkKWApplication&);  // Not implemented.
};

#endif
