/*=========================================================================

  Module:    vtkKWOKCancelDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWOKCancelDialog - a dialog box with an OK and Cancel button
// .SECTION Description
// A class for OKCancelDialog boxes.

#ifndef __vtkKWOKCancelDialog_h
#define __vtkKWOKCancelDialog_h

#include "vtkKWDialog.h"

class vtkKWApplication;
class vtkKWPushButton;
class vtkKWLabel;
class vtkKWFrame;

class VTK_EXPORT vtkKWOKCancelDialog : public vtkKWDialog
{
public:
  static vtkKWOKCancelDialog* New();
  vtkTypeRevisionMacro(vtkKWOKCancelDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the text of the message
  void SetText(const char *);
  
protected:
  vtkKWOKCancelDialog();
  ~vtkKWOKCancelDialog();

  vtkKWLabel *Message;
  vtkKWFrame *ButtonFrame;
  vtkKWPushButton *OKButton;
  vtkKWPushButton *CancelButton;

private:
  vtkKWOKCancelDialog(const vtkKWOKCancelDialog&); // Not implemented
  void operator=(const vtkKWOKCancelDialog&); // Not implemented
};


#endif



