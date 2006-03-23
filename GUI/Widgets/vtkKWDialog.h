/*=========================================================================

  Module:    vtkKWDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWDialog - dialog box superclass
// .SECTION Description
// A generic superclass for dialog boxes.
// This is a toplevel that is modal by default, and centered in its
// master window (or on screen)

#ifndef __vtkKWDialog_h
#define __vtkKWDialog_h

#include "vtkKWTopLevel.h"

class KWWidgets_EXPORT vtkKWDialog : public vtkKWTopLevel
{
public:
  static vtkKWDialog* New();
  vtkTypeRevisionMacro(vtkKWDialog,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Invoke the dialog, display it and enter an event loop until the user
  // confirms or cancels the dialog.
  // Note that a dialog is a modal toplevel by default.
  // This method returns a zero if the dialog was killed or 
  // canceled, nonzero otherwise. The status can be further refined
  // by querying GetStatus().
  virtual int Invoke();

  // Description:
  // Display the dialog. 
  // Note that a dialog is a modal toplevel by default.
  virtual void Display();

  // Description:
  // Status of the dialog (active e.g. displayed, canceled, OK'ed)
  //BTX
  enum 
  {
    StatusActive = 0,
    StatusCanceled = 1,
    StatusOK = 2
  };
  //ETX
  int GetStatus() { return this->Done; };

  // Description:
  // Return frame to pack into.
  vtkKWWidget* GetFrame() { return this; }

  // Description:
  // Play beep when the dialog is displayed
  vtkSetClampMacro(Beep, int, 0, 1);
  vtkBooleanMacro(Beep, int);
  vtkGetMacro(Beep, int);

  // Description:
  // Sets the beep type
  vtkSetMacro(BeepType, int);
  vtkGetMacro(BeepType, int);

  // Description:
  // Callback. Cancel the action and close this dialog
  virtual void Cancel();

  // Description:
  // Callback. Confirm the action and close this dialog
  virtual void OK();

  // Description:
  // Dialog can be also used by performing individual steps of Invoke. These
  // steps are initialize: PreInvoke(), finalize: PostInvoke(), and check if
  // user responded IsUserDoneWithDialog(). Use this method only if you
  // want to bypass the event loop used in Invoke() by creating your own
  // and checking for IsUserDoneWithDialog().
  virtual int PreInvoke();
  virtual void PostInvoke();
  virtual int IsUserDoneWithDialog();

protected:

  vtkKWDialog();
  ~vtkKWDialog() {};

  int Done;
  int Beep;
  int BeepType;

private:
  vtkKWDialog(const vtkKWDialog&); // Not implemented
  void operator=(const vtkKWDialog&); // Not Implemented
};

#endif
