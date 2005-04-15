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
// application. It is also responsible for managing the vtkKWWindow(s) 
// associated to the application.

#ifndef __vtkKWApplication_h
#define __vtkKWApplication_h

#include "vtkKWObject.h"

#include "vtkTcl.h" // Needed for Tcl_Interp
#include "vtkTk.h"  // Needed for Tk_Window

class vtkKWLabel;
class vtkKWMessageDialog;
class vtkKWRegistryHelper;
class vtkKWBalloonHelpManager;
class vtkKWSplashScreen;
class vtkKWWidget;
class vtkKWWindow;
class vtkKWText;
class vtkKWApplicationInternals;

class VTK_EXPORT vtkKWApplication : public vtkKWObject
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
  // Start running the application, with or without arguments.
  virtual void Start();
  virtual void Start(int argc, char *argv[]);

  // Description:
  // This method is invoked when the user exits the app
  virtual void Exit();

  // Description:
  // Set/Get the value returned by the application at exit.
  // This can be used from scripts to set an error status
  vtkSetMacro(ExitStatus, int);
  vtkGetMacro(ExitStatus, int);

  // Description:
  // Get when application is exiting (set to 1 as soon as Exit() is called).
  vtkGetMacro(InExit, int);

  // Description:
  // Add/Close a window to this application.
  // Note that AddWindow() will increase the reference count of the window
  // that is added, CloseWindow() will decrease it. Once the last window is
  // closed, Exit() is called.
  virtual void AddWindow(vtkKWWindow *w);
  virtual void CloseWindow(vtkKWWindow *);

  // Description:
  // Get the number of windows, retrieve a window
  virtual int GetNumberOfWindows();
  virtual vtkKWWindow* GetNthWindow(int rank);

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
  // Set/Get the application version name - this is the application name
  // postfixed with the version number (major/minor), no spaces.
  // It is typically used as the master key to store registry settings
  // (ex: VolView20, ParaView1.1, etc.)
  vtkSetStringMacro(VersionName);
  vtkGetStringMacro(VersionName);

  // Description:
  // Set/Get the application release name - this is the release of the 
  // application version (if any), typically: beta1, beta2, final, patch1, etc.
  vtkSetStringMacro(ReleaseName);
  vtkGetStringMacro(ReleaseName);

  // Description:
  // Convenience method to get the "pretty" name of the application. 
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
  // Convenience method that will return the limited edition mode and 
  // optionally warn the user ; if the limited edition mode is true, 
  // it will display a popup warning stating that 'feature' is not available
  // in this mode.
  virtual int GetLimitedEditionModeAndWarn(const char *feature);

  // Descrition:
  // Set/Get the name of the application when it runs in limited edition mode.
  // This is used by GetPrettyName() for example, instead of the Name variable.
  vtkSetStringMacro(LimitedEditionModeName);
  vtkGetStringMacro(LimitedEditionModeName);

  // Description:
  // Set/Get the directory in which the current application is supposed
  // to be installed. 
  vtkGetStringMacro(InstallationDirectory);
  vtkSetStringMacro(InstallationDirectory);
  
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
  // Set/get/delete/query a registry value for the application.
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
  // Convenience methods to retrieve a value from the registry and convert
  // it to a type (boolean, float, int). 
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
  
  // Descrition:
  // Get the application settings that are stored in the registry.
  // Do not call that method before the application name is known and the
  // proper registry level set (if any).
  virtual void GetApplicationSettingsFromRegistry();

  // Description:
  // Query if this application supports a splash screen
  vtkGetMacro(HasSplashScreen, int);

  // Description:
  // Set/Get if this application should show the splash screen at startup
  vtkGetMacro(ShowSplashScreen, int);
  vtkSetMacro(ShowSplashScreen, int);
  vtkBooleanMacro(ShowSplashScreen, int);

  // Description:
  // Retrieve the splash screen object
  virtual vtkKWSplashScreen* GetSplashScreen();

  // Description:
  // Set/Get if the windows geometry should be saved (to registry).
  vtkGetMacro(SaveWindowGeometry, int);
  vtkSetMacro(SaveWindowGeometry, int);
  vtkBooleanMacro(SaveWindowGeometry, int);

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
  // Get if we have some logic to report feedback by email and
  // email that feedback.
  // Set/Get the email address to send that feedback to.
  virtual int CanEmailFeedback();
  virtual void EmailFeedback();
  vtkSetStringMacro(EmailFeedbackAddress);
  vtkGetStringMacro(EmailFeedbackAddress);

  // Description:
  // Display the on-line help for this application.
  // Optionally provide a master window this dialog should be the slave of.
  virtual void DisplayHelp(vtkKWWindow *master);

  // Description:
  // Set/Get the the on-line help starting page
  vtkGetStringMacro(DisplayHelpStartingPage);
  vtkSetStringMacro(DisplayHelpStartingPage);

  // Description:
  // Display the about dialog for this application.
  // Optionally provide a master window this dialog should be the slave of.
  virtual void DisplayAbout(vtkKWWindow *master);

  // Description:
  // Return the Balloon Help helper object. 
  vtkKWBalloonHelpManager *GetBalloonHelpManager();

  // Description:
  // Convenience methods to evaluate Tcl script/code and
  // perform argument substitutions.
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
  //BTX
  static Tcl_Interp *InitializeTcl(int argc, char *argv[], ostream *err = 0);
  //ETX

  // Description:
  // Call RegisterDialogUp to notify the application that a modal dialog is up,
  // and UnRegisterDialogUp when it is not anymore. GetDialogUp will return
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
  vtkGetMacro(DialogUp, int);
  
protected:
  vtkKWApplication();
  ~vtkKWApplication();

  Tk_Window MainWindow;
  Tcl_Interp *MainInterp;

  // Description:
  // Do one tcl event and enter the event loop, allowing the application
  // interface to actually run.
  virtual void DoOneTclEvent();

  // Description:
  // Cleanup everything before exiting.
  virtual void Cleanup() { };

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
  // Application installation directory
  char *InstallationDirectory;
  virtual void FindInstallationDirectory();

  // Description:
  // Add email feedback body and subject to output stream.
  // Override this function in subclasses (and/or call the superclass) to
  // add more information.
  virtual void AddEmailFeedbackBody(ostream &);
  virtual void AddEmailFeedbackSubject(ostream &);
  char *EmailFeedbackAddress;

  // Description:
  // On-line help starting page
  char *DisplayHelpStartingPage;

  // Description:
  // Value that is set after exit (status), flag stating that 
  // Exit was called, flag stating if application should exit after load script
  int ExitStatus;
  int InExit;
  int ExitAfterLoadScript;

  // Description:
  // Number of dialog that are up. See Un/RegisterDialogUp().
  int DialogUp;

  // Description:
  // Limited edition mode, name of the application when in limited edition mode
  int LimitedEditionMode;
  char *LimitedEditionModeName;

  // Description:
  // Registry level. If a call to Set/GetRegistryValue uses a level above
  // this ivar, the operation is ignored.
  int RegistryLevel;

  // Description:
  // Flag stating if application supports splash screen, and shows it
  int HasSplashScreen;
  int ShowSplashScreen;
  virtual void CreateSplashScreen() {};

  // Description:
  // Flag stating if the window geometry should be saved before exiting
  int SaveWindowGeometry;

  // Description:
  // About dialog, add text and copyrights to the about dialog.
  // Override this function in subclasses (and/or call the superclass) to
  // add more information.
  virtual void ConfigureAbout();
  virtual void AddAboutText(ostream &);
  virtual void AddAboutCopyrights(ostream &);
  vtkKWMessageDialog *AboutDialog;
  vtkKWLabel         *AboutDialogImage;
  vtkKWText          *AboutRuntimeInfo;

  // Description:
  // Character encoding (is passed to Tcl)
  int CharacterEncoding;

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

  // PIMPL Encapsulation for STL containers

  vtkKWApplicationInternals *Internals;

private:

  vtkKWRegistryHelper *RegistryHelper;
  vtkKWSplashScreen *SplashScreen;
  vtkKWBalloonHelpManager *BalloonHelpManager;

  vtkKWApplication(const vtkKWApplication&);   // Not implemented.
  void operator=(const vtkKWApplication&);  // Not implemented.
};

#endif
