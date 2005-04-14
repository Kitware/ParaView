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
  int LoadScript(const char* filename);

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
  // When a modal dialog is up, this flag should be set.
  // vtkKWWindow will check this at exit and if set,
  // it will refuse to exit
  vtkSetMacro(DialogUp, int);
  vtkGetMacro(DialogUp, int);

  // Description:
  // This method returns a message dialog response.
  // The string is the name of dialog and the return is:
  // 0 for no response (display dialog), 1 for response ok/yes
  // and -1 for response cancel/no.
  int GetMessageDialogResponse(const char* dialogname);
  
  // Description:
  // Set the message dialog response. Set 1 for ok/yes and -1 for 
  // cancel/no.
  void SetMessageDialogResponse(const char* dialogname, int response);

  // Description:
  // Return the Registry object. It is created on first use
  // and deleted on exiting the application.
  //BTX
  vtkKWRegistryHelper *GetRegistryHelper();
  //ETX

  // Description:
  // Set/get the current registry level. If the called
  // registry level is lower than current one, the operation is 
  // successfull. Registry level -1 means to ignore all the 
  // registry operations.
  vtkSetClampMacro(RegistryLevel, int, -1, 10);
  vtkGetMacro(RegistryLevel, int);

  //Description:
  // Set or get the registry value for the application.
  // When storing multiple arguments, separate with spaces.
  // If the level is lower than current registry level, operation 
  // will be successfull.
  //BTX
  int SetRegistryValue(int level, const char* subkey, const char* key, 
                        const char* format, ...);
  //ETX
  int GetRegistryValue(int level, const char* subkey, const char* key, 
                        char* value);
  int DeleteRegistryValue(int level, const char* subkey, const char* key);
  int HasRegistryValue(int level, const char* subkey, const char* key);
  
  // Description:
  // Perform a boolean check of the value in registry. If the value
  // at the key is trueval, then return true, otherwise return false.
  int BooleanRegistryCheck(int level, const char* subkey, const char* key, 
                            const char* trueval);

  // Description:
  // Get float registry value (zero if not found).
  // If the level is lower than current registry level, operation 
  // will be successfull.
  float GetFloatRegistryValue(int level, const char* subkey, 
                               const char* key);
  int   GetIntRegistryValue(int level, const char* subkey, const char* key);
  
  // Descrition:
  // Get those application settings that are stored in the registry
  // Should be called once the application name is known (and the registry
  // level set).
  virtual void GetApplicationSettingsFromRegistry();

  // Description:
  // Get the splash screen, if this app have/show a splash screen.
  virtual vtkKWSplashScreen* GetSplashScreen();
  vtkGetMacro(HasSplashScreen, int);
  vtkGetMacro(ShowSplashScreen, int);
  vtkSetMacro(ShowSplashScreen, int);
  vtkBooleanMacro(ShowSplashScreen, int);

  // Description:
  // Save windows geometry.
  vtkGetMacro(SaveWindowGeometry, int);
  vtkSetMacro(SaveWindowGeometry, int);
  vtkBooleanMacro(SaveWindowGeometry, int);

  // Description:
  // At the end of Exit(), this is set to true. Other objects
  // can use this to cleanup properly.
  vtkGetMacro(ApplicationExited, int);

  // Description:
  // This return 1 when application is exiting.
  vtkGetMacro(InExit, int);

  // Description:
  // Get/Set the internal character encoding of the application.
  virtual void SetCharacterEncoding(int val);
  vtkGetMacro(CharacterEncoding, int);
  
  // Description:
  // Test if we have some logic to check for application update and
  // eventually perform that check.
  virtual int HasCheckForUpdates();
  virtual void CheckForUpdates();

  // Description:
  // Test if we have some logic to report a bug and
  // eventually report a bug check.
  virtual int CanEmailFeedback();
  virtual void EmailFeedback();
  vtkSetStringMacro(EmailFeedbackAddress);
  vtkGetStringMacro(EmailFeedbackAddress);

  // Description:
  // Display the on-line help and about dialog for this application.
  virtual void DisplayHelp(vtkKWWindow *master);
  virtual void DisplayAbout(vtkKWWindow *master);

  // Description:
  // Convenience method to call UpdateEnableState on all windows
  virtual void UpdateEnableStateForAllWindows();

  // Description:
  // Convenience method to get the operating system version
  virtual int GetSystemVersion(ostream &os);

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

  virtual void AddEmailFeedbackBody(ostream &);
  virtual void AddEmailFeedbackSubject(ostream &);
  char *EmailFeedbackAddress;

  int ApplicationExited;

  vtkGetStringMacro(DisplayHelpStartingPage);
  vtkSetStringMacro(DisplayHelpStartingPage);
  char *DisplayHelpStartingPage;

  int InExit;
  int DialogUp;

  int ExitStatus;
  int LimitedEditionMode;
  char *LimitedEditionModeName;

  int RegistryLevel;

  int UseMessageDialogs;

  int ExitAfterLoadScript;

  // Splash screen

  virtual void CreateSplashScreen() {};
  int HasSplashScreen;
  int ShowSplashScreen;

  int SaveWindowGeometry;

  // About dialog

  virtual void ConfigureAbout();
  virtual void AddAboutText(ostream &);
  virtual void AddAboutCopyrights(ostream &);
  vtkKWMessageDialog *AboutDialog;
  vtkKWLabel         *AboutDialogImage;
  vtkKWText          *AboutRuntimeInfo;

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
