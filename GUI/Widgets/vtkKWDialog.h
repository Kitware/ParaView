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

#ifndef __vtkKWDialog_h
#define __vtkKWDialog_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWWindow;

class VTK_EXPORT vtkKWDialog : public vtkKWWidget
{
public:
  static vtkKWDialog* New();
  vtkTypeRevisionMacro(vtkKWDialog,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();

  // Description:
  // Display the dialog in a non-modal manner.
  virtual void Display();

  // Description:
  // Close this Dialog
  virtual void Cancel();

  // Description:
  // Close this Dialog
  virtual void OK();

  // Description:
  // Returns 0 if the dialog is active e.g. displayed
  // 1 if it was Canceled 2 if it was OK.
  int GetStatus() {return this->Done;};

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
  // Invoke the dialog centered at the mouse pointer position (default is
  // either screen center or window center)
  vtkSetClampMacro(InvokeAtPointer, int, 0, 1);
  vtkBooleanMacro(InvokeAtPointer, int);
  vtkGetMacro(InvokeAtPointer, int);

  // Description:
  // Set the title of the dialog. Default is "Kitware Dialog".
  void SetTitle(const char *);

  // Description:
  // Set the window to which the dialog will be slave.
  // If set, this dialog will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this)
  void SetMasterWindow(vtkKWWindow* win);
  vtkKWWindow *GetMasterWindow();

  // Description:
  // Convenience method to guess the width/height of the dialog.
  virtual int GetWidth();
  virtual int GetHeight();

protected:
  // Description:
  // Set the title string of the dialog window. Should be called before
  // create otherwise it will have no effect.
  vtkSetStringMacro(TitleString);

  vtkKWDialog();
  ~vtkKWDialog();

  vtkKWWindow* MasterWindow;

  char *TitleString;
  int Done;
  int Beep;
  int BeepType;
  int InvokeAtPointer;
  int GrabDialog;

private:
  vtkKWDialog(const vtkKWDialog&); // Not implemented
  void operator=(const vtkKWDialog&); // Not Implemented
};


#endif



