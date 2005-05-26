/*=========================================================================

  Module:    vtkKWTclInteractor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTclInteractor - a KW version of interactor.tcl
// .SECTION Description
// A widget to interactively execute Tcl commands

#ifndef __vtkKWTclInteractor_h
#define __vtkKWTclInteractor_h

#include "vtkKWTopLevel.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWText;

class KWWIDGETS_EXPORT vtkKWTclInteractor : public vtkKWTopLevel
{
public:
  static vtkKWTclInteractor* New();
  vtkTypeRevisionMacro(vtkKWTclInteractor, vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create tje widget
  virtual void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Display the interactor
  // Override superclass to set position automatically
  virtual void Display();

  // Description:
  // Evaluate the tcl string
  virtual void Evaluate();

  // Description:
  // Callback for the down arrow key
  virtual void DownCallback();
  
  // Description:
  // Callback for the up arrow key
  virtual void UpCallback();

  // Description:
  // Append text to the display window. Can be used for sending
  // debugging information to the command prompt when no standard
  // output is available.
  virtual void AppendText(const char* text);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWTclInteractor();
  ~vtkKWTclInteractor();

  vtkKWFrame      *ButtonFrame;
  vtkKWPushButton *DismissButton;
  vtkKWFrame      *CommandFrame;
  vtkKWLabel      *CommandLabel;
  vtkKWEntry      *CommandEntry;
  vtkKWText       *DisplayText;
  
  int TagNumber;
  int CommandIndex;

private:
  vtkKWTclInteractor(const vtkKWTclInteractor&); // Not implemented
  void operator=(const vtkKWTclInteractor&); // Not implemented
};

#endif

