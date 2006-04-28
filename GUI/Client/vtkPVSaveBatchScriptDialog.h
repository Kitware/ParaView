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

#include "vtkKWDialog.h"

class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWWindow;
class vtkKWEntry;
class vtkKWCheckButton;
class vtkPVApplication;
class vtkKWFrame;

class VTK_EXPORT vtkPVSaveBatchScriptDialog : public vtkKWDialog
{
public:
  static vtkPVSaveBatchScriptDialog* New();
  vtkTypeRevisionMacro(vtkPVSaveBatchScriptDialog, vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Access to the results of the dialog.
  int GetOffScreen();
  const char* GetImagesFileName();
  const char* GetGeometryFileName();

  // Description:
  // Path and root to use as default file names.
  vtkSetStringMacro(FilePath);
  vtkSetStringMacro(FileRoot);

  // Description:
  // Callback used by widgets.
  void SaveImagesCheckCallback(int state);
  void SaveGeometryCheckCallback(int state);
  void ImageFileNameBrowseButtonCallback();
  void GeometryFileNameBrowseButtonCallback();

  // Description:
  // A convenience method.
  vtkPVApplication *GetPVApplication();

protected:
  vtkPVSaveBatchScriptDialog();
  ~vtkPVSaveBatchScriptDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  char* FilePath;
  char* FileRoot;
  
  vtkKWCheckButton* OffScreenCheck;

  vtkKWCheckButton* SaveImagesCheck;
  vtkKWFrame*      ImageFileNameFrame;
  vtkKWEntry*       ImageFileNameEntry;
  vtkKWPushButton*  ImageFileNameBrowseButton;

  vtkKWCheckButton* SaveGeometryCheck;
  vtkKWFrame*      GeometryFileNameFrame;
  vtkKWEntry*       GeometryFileNameEntry;
  vtkKWPushButton*  GeometryFileNameBrowseButton;

  vtkKWFrame*      ButtonFrame;
  vtkKWPushButton*  AcceptButton;
  vtkKWPushButton*  CancelButton;
    
private:
  vtkPVSaveBatchScriptDialog(const vtkPVSaveBatchScriptDialog&); // Not implemented
  void operator=(const vtkPVSaveBatchScriptDialog&); // Not implemented
};

#endif
