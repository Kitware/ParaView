/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSaveBatchScriptDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSaveBatchScriptDialog - Shows a text version of the timer log entries.
// .SECTION Description
// A widget to display timing information in the timer log.

#ifndef __vtkPVSaveBatchScriptDialog_h
#define __vtkPVSaveBatchScriptDialog_h

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWWindow;
class vtkKWEntry;
class vtkKWCheckButton;
class vtkPVApplication;

class VTK_EXPORT vtkPVSaveBatchScriptDialog : public vtkKWWidget
{
public:
  static vtkPVSaveBatchScriptDialog* New();
  vtkTypeRevisionMacro(vtkPVSaveBatchScriptDialog, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Display the interactor
  int Invoke();

  // Description:
  // Access to the results of the dialog.
  int GetOffScreen();
  const char* GetImagesFileName();
  const char* GetGeometryFileName();

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
  // Path and root to use as default file names.
  vtkSetStringMacro(FilePath);
  vtkSetStringMacro(FileRoot);

  // Description:
  // Callback from the accept/dismiss button that closes the window.
  void Accept();
  void Cancel();

  // Description:
  // Callback used by widgets.
  void SaveImagesCheckCallback();
  void SaveGeometryCheckCallback();
  void ImageFileNameBrowseButtonCallback();
  void GeometryFileNameBrowseButtonCallback();

  // Description:
  // A convenience method.
  vtkPVApplication *GetPVApplication();

protected:
  vtkPVSaveBatchScriptDialog();
  ~vtkPVSaveBatchScriptDialog();

  char* FilePath;
  char* FileRoot;
  
  vtkKWWindow*      MasterWindow;

  vtkKWCheckButton* OffScreenCheck;

  vtkKWCheckButton* SaveImagesCheck;
  vtkKWWidget*      ImageFileNameFrame;
  vtkKWEntry*       ImageFileNameEntry;
  vtkKWPushButton*  ImageFileNameBrowseButton;

  vtkKWCheckButton* SaveGeometryCheck;
  vtkKWWidget*      GeometryFileNameFrame;
  vtkKWEntry*       GeometryFileNameEntry;
  vtkKWPushButton*  GeometryFileNameBrowseButton;

  vtkKWWidget*      ButtonFrame;
  vtkKWPushButton*  AcceptButton;
  vtkKWPushButton*  CancelButton;
  int               AcceptedFlag;
  int               Exit;
    
  char*   Title;

private:
  vtkPVSaveBatchScriptDialog(const vtkPVSaveBatchScriptDialog&); // Not implemented
  void operator=(const vtkPVSaveBatchScriptDialog&); // Not implemented
};

#endif
