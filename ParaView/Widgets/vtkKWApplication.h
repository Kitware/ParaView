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
class vtkKWEventNotifier;

class VTK_EXPORT vtkKWApplication : public vtkKWObject
{
public:
  static vtkKWApplication* New();
  vtkTypeMacro(vtkKWApplication,vtkKWObject);
  
  virtual vtkKWApplication *GetApplication()  { return this;  }
  virtual void SetApplication (vtkKWApplication* arg) 
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
  virtual void DisplayHelp();
  virtual void DisplayAbout(vtkKWWindow *);

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
  
//BTX
  // Description:
  // A convienience method to invoke some tcl script code and
  // perform arguement substitution.
  void Script(const char *EventString, ...);
  void SimpleScript(char *EventString);
  void SimpleScript(const char *EventString);
//ETX

  // Description:
  // Internal Balloon help callbacks.
  void BalloonHelpTrigger(vtkKWWidget *widget);
  void BalloonHelpDisplay(vtkKWWidget *widget);
  void BalloonHelpCancel();
  void BalloonHelpWithdraw();

  // Description:
  // This variable can be used to hide the user interface.  
  // When WidgetVisibility is off, The cherat methods of vtkKWWidgets 
  // should not create the TK widgets.
  static void SetWidgetVisibility(int v);
  static int GetWidgetVisibility();
  vtkBooleanMacro(WidgetVisibility, int);
  
  // Description:
  // Get the event notifier so that callback can be set or events invoked.
  vtkGetObjectMacro( EventNotifier, vtkKWEventNotifier );

//BTX
  // Description:
  // This can be esed to trace the actions of the user.
  // It only works when the TraceFile has been set.
  void AddTraceEntry(char *format, ...);
//ETX

protected:
  vtkKWApplication();
  ~vtkKWApplication();
  vtkKWApplication(const vtkKWApplication&) {};
  void operator=(const vtkKWApplication&) {};

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

  virtual int GetApplicationKey() {return -1;};

  static int WidgetVisibility;
  int InExit;
  
  vtkKWEventNotifier *EventNotifier;

  ofstream *TraceFile;
};

#endif






