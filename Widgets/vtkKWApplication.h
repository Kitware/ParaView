/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWApplication.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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
};

#endif






