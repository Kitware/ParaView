/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderGroupDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderGroupDialog - Shows a text version of the timer log entries.
// .SECTION Description
// A widget to display timing information in the timer log.

#ifndef __vtkPVRenderGroupDialog_h
#define __vtkPVRenderGroupDialog_h

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWWindow;
class vtkKWEntry;
class vtkKWCheckButton;

class VTK_EXPORT vtkPVRenderGroupDialog : public vtkKWWidget
{
public:
  static vtkPVRenderGroupDialog* New();
  vtkTypeRevisionMacro(vtkPVRenderGroupDialog, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Display the interactor
  void Invoke();

  // Description:
  // Callback from the dismiss button that closes the window.
  void Accept();

  // Description:
  // Set the title of the TclInteractor to appear in the titlebar
  vtkSetStringMacro(Title);
  
  // Description:
  // Set the window to which the dialog will be slave.
  // If set, this dialog will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this)
  void SetMasterWindow(vtkKWWindow* win);
  
  // Description:
  // Access to the result of the dialog.
  void SetNumberOfProcessesInGroup(int val);
  vtkGetMacro(NumberOfProcessesInGroup, int);

  // Description:
  // A callbacks from the UI.
  void NumberEntryCallback();

  // Description:
  // Initialize the display strings, or Get the desplay strings
  // Chosen by the user.  The first string cannot b e modified
  // by the user.  The display strings entry is not created
  // unless the first display string is initialized.
  void SetDisplayString(int idx, const char* str);
  const char* GetDisplayString(int idx); 

protected:
  vtkPVRenderGroupDialog();
  ~vtkPVRenderGroupDialog();

  // Returns 1 if first display is OK. 0 if user has modified the display.
  void Update();
  void ComputeDisplayStringRoot(const char* str);

  void Append(const char*);
  
  vtkKWWindow*      MasterWindow;

  vtkKWWidget*      ControlFrame;
  vtkKWPushButton*  SaveButton;
  vtkKWPushButton*  ClearButton;
  vtkKWLabel*       NumberLabel;
  vtkKWEntry*       NumberEntry;

  int               DisplayFlag;
  vtkKWWidget*      DisplayFrame;
  vtkKWLabel*       Display0Label;
  vtkKWEntry**      DisplayEntries;
  char*             DisplayStringRoot;

  vtkKWWidget*      ButtonFrame;
  vtkKWPushButton*  AcceptButton;
  int AcceptedFlag;

    
  char*   Title;
  int     Writable;
  int     NumberOfProcessesInGroup;

private:
  vtkPVRenderGroupDialog(const vtkPVRenderGroupDialog&); // Not implemented
  void operator=(const vtkPVRenderGroupDialog&); // Not implemented
};

#endif
