/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTraceFileDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTraceFileDialog - asks the user if she wants to save the old trace
// .SECTION Description
// This is a dialog with three buttons which should normally be:
// Delete, Do Nothing, Save. Use this to ask the user whether
// he wants to save or delete an old tracefile. Invoke returns
// 0 for do nothing, 1 for delete and 2 for save.
// .SECTION See Also
// vtkPVApplication

#ifndef __vtkPVTraceFileDialog_h
#define __vtkPVTraceFileDialog_h

#include "vtkKWMessageDialog.h"

class vtkKWPushButton;
class vtkKWFrame;

class VTK_EXPORT vtkPVTraceFileDialog : public vtkKWMessageDialog
{
public:
  static vtkPVTraceFileDialog* New();
  vtkTypeRevisionMacro(vtkPVTraceFileDialog,vtkKWMessageDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Sets the save flag.
  virtual void Save();
  virtual void Retrace();

protected:
  vtkPVTraceFileDialog();
  ~vtkPVTraceFileDialog();

  vtkKWPushButton *SaveButton;
  vtkKWFrame  *SaveFrame;
  vtkKWPushButton *RetraceButton;
  vtkKWFrame  *RetraceFrame;

private:
  vtkPVTraceFileDialog(const vtkPVTraceFileDialog&); // Not implemented
  void operator=(const vtkPVTraceFileDialog&); // Not implemented
};


#endif


