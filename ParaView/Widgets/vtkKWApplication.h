/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWApplication.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWApplication - manage all the windows in an application
// .SECTION Description
// vtkKWApplication is the overall class that represents the entire 
// applicaiton. It is a fairly small class that is primarily responsible
// for managing the all the vtkKWWindows in the application. 


#ifndef __vtkKWApplication_h
#define __vtkKWApplication_h

#include "vtkKWObject.h"
#include "tcl.h"
#include "tk.h"

class vtkKWWindowCollection;
class vtkKWWindow;
class vtkKWWidget;
class vtkKWRegisteryUtilities;

//BTX
template<class KeyType,class DataType> class vtkAbstractMap;
//ETX

class VTK_EXPORT vtkKWApplication : public vtkKWObject
{
public:
  static vtkKWApplication* New();
  vtkTypeMacro(vtkKWApplication,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual vtkKWApplication *GetApplication()  { return this;  }
  virtual void SetApplication (vtkKWApplication*) 
    { 
    vtkErrorMacro( << "Do not set the Application on an Application" << endl ); 
    }
  
  
  // Description:
  // Start running the main application.
  virtual void Start();
  virtual void Start(char *arg);
  virtual void Start(int argc, char *argv[]);

  // Description:
  // class static method to initialize Tcl/Tk
  static Tcl_Interp *InitializeTcl(int argc, char *argv[]);
  
  // Description:
  // Get the interpreter being used by this application
  Tcl_Interp *GetMainInterp() {return this->MainInterp;};

  // Description:
  // The method to invoke when the user exits the app
  virtual void Exit();

  // Description:
  // This method is invoked when a window closes
  virtual void Close(vtkKWWindow *);

  // Description:
  // Display the on-line help and about dialog for this application.
  virtual void DisplayHelp(vtkKWWindow *master);
  virtual void DisplayAbout(vtkKWWindow *master);

  // Description:
  // Set or get the ExitOnReturn flag. If this flag is set, then 
  // the next tcl script will make application exit when return.
  // Useful for tests. 
  vtkSetClampMacro(ExitOnReturn, int, 0, 1);
  vtkBooleanMacro(ExitOnReturn, int);
  vtkGetMacro(ExitOnReturn, int);

  // Description:
  // Add a window to this application.
  void AddWindow(vtkKWWindow *w);
  vtkKWWindowCollection *GetWindows();
  
  // Description:
  // Set/Get the ApplicationName
  void SetApplicationName(const char *);
  vtkGetStringMacro(ApplicationName);

  // Description:
  // Set/Get the ApplicationVersionName - this is the name + version number
  void SetApplicationVersionName(const char *);
  vtkGetStringMacro(ApplicationVersionName);

  // Description:
  // Set/Get the ApplicationReleaseName - this is the release of the 
  // application version, typically beta 1, beta 2, final, patch 1, patch 2
  void SetApplicationReleaseName(const char *);
  vtkGetStringMacro(ApplicationReleaseName);
  
  // Description:
  // Load script from a file. Resturn if script was successful.
  int LoadScript(const char* filename);

//BTX
  // Description:
  // A convienience method to invoke some tcl script code and
  // perform arguement substitution.
  const char* Script(const char *EventString, ...);
  const char* SimpleScript(const char *EventString);
  const char* EvaluateString(const char *String, ...);
  const char* ExpandFileName(const char *String, ...);
  int EvaluateBooleanExpression(const char*Expression, ...);
//ETX

  // Description:
  // Test some of the features that cannot be tested from the tcl.
  int SelfTest();

  // Description:
  // Internal Balloon help callbacks.
  void BalloonHelpTrigger(vtkKWWidget *widget);
  void BalloonHelpDisplay(vtkKWWidget *widget);
  void BalloonHelpCancel();
  void BalloonHelpWithdraw();
  void SetBalloonHelpWidget(vtkKWWidget *widget);

  // Description:
  // Set the delay for the balloon help in seconds.
  // To disable balloon help, set it to 0.
  vtkSetClampMacro(BalloonHelpDelay, int, 0, 5);
  vtkGetMacro(BalloonHelpDelay, int);

  // Description:
  // This variable can be used to hide the user interface.  
  // When WidgetVisibility is off, The cherat methods of vtkKWWidgets 
  // should not create the TK widgets.
  static void SetWidgetVisibility(int v);
  static int GetWidgetVisibility();
  vtkBooleanMacro(WidgetVisibility, int);
  
  // Description:
  // This can be used to trace the application.
  // Look at vtkKWWidgets to see how it is used.
  ofstream *GetTraceFile() {return this->TraceFile;}
//BTX
  void AddTraceEntry(const char *format, ...);
//ETX
  virtual int GetApplicationKey() {return -1;};

  // Description:
  // Return the Registery object. It is created on first use
  // and deleted on exiting the application.
  vtkKWRegisteryUtilities *GetRegistery( const char* toplevel);
  vtkKWRegisteryUtilities *GetRegistery( );

  // Description:
  // Set or get the current registery level. If the called
  // registery level is lower than current one, the operation is 
  // successfull. Registery level -1 means to ignore all the 
  // registery operations.
  vtkSetClampMacro(RegisteryLevel, int, -1, 10);
  vtkGetMacro(RegisteryLevel, int);

  // Description:
  // This value will be returned by application1 at exit.
  // Use this from scripts if you want ParaView exit with an
  // error status (for example to indicate that a regression test 
  // failed)
  vtkSetMacro(ExitStatus, int);
  vtkGetMacro(ExitStatus, int);

  // Description:
  // When a modal dialog is up, this flag should be set.
  // vtkKWWindow will check this at exit and if set,
  // it will refuse to exit
  vtkSetMacro(DialogUp, int);
  vtkGetMacro(DialogUp, int);

  // Description:
  // This method will suppress all message dialogs.
  // There is no way back for now.
  void SupressMessageDialogs() { this->UseMessageDialogs = 0; }
  vtkGetMacro(UseMessageDialogs, int);

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

  //Description:
  // Set or get the registery value for the application.
  // When storing multiple arguments, separate with spaces.
  // If the level is lower than current registery level, operation 
  // will be successfull.
//BTX
  int SetRegisteryValue(int level, const char* subkey, const char* key, 
                        const char* format, ...);
//ETX
  int GetRegisteryValue(int level, const char* subkey, const char* key, 
                        char* value);
  int DeleteRegisteryValue(int level, const char* subkey, const char* key);
  
  // Description:
  // Perform a boolean check of the value in registery. If the value
  // at the key is trueval, then return true, otherwise return false.
  int BooleanRegisteryCheck(int level, const char* key, 
                            const char* trueval);

  // Description:
  // Get float registery value (zero if not found).
  // If the level is lower than current registery level, operation 
  // will be successfull.
  float GetFloatRegisteryValue(int level, const char* subkey, 
                               const char* key);
  int   GetIntRegisteryValue(int level, const char* subkey, const char* key);
  
protected:
  vtkKWApplication();
  ~vtkKWApplication();

  Tk_Window MainWindow;
  Tcl_Interp *MainInterp;
  vtkKWWindowCollection *Windows;
  char *ApplicationName;
  char *ApplicationVersionName;
  char *ApplicationReleaseName;

  // For Balloon help
  vtkKWWidget *BalloonHelpWindow;
  vtkKWWidget *BalloonHelpLabel;
  char *BalloonHelpPending;
  vtkSetStringMacro(BalloonHelpPending);
  vtkKWWidget *BalloonHelpWidget;

  static int WidgetVisibility;
  int InExit;
  int DialogUp;

  int ExitStatus;
  
  ofstream *TraceFile;

  vtkKWRegisteryUtilities *Registery;
  int RegisteryLevel;
  int BalloonHelpDelay;

  int UseMessageDialogs;

  int ExitOnReturn;

private:
  vtkKWApplication(const vtkKWApplication&);   // Not implemented.
  void operator=(const vtkKWApplication&);  // Not implemented.
};

#endif






