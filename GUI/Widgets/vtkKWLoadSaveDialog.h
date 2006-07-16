/*=========================================================================

  Module:    vtkKWLoadSaveDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLoadSaveDialog - dalog for loading or saving files
// .SECTION Description
// A dialog for loading or saving files. The file is returned through 
// GetFile method.

#ifndef __vtkKWLoadSaveDialog_h
#define __vtkKWLoadSaveDialog_h

#include "vtkKWDialog.h"

class vtkStringArray;

class KWWidgets_EXPORT vtkKWLoadSaveDialog : public vtkKWDialog
{
public:
  static vtkKWLoadSaveDialog* New();
  vtkTypeRevisionMacro(vtkKWLoadSaveDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Invoke the dialog, display it and enter an event loop until the user
  // confirm (OK) or cancel the dialog.
  // Note that a dialog is a modal toplevel by default.
  // Important: this implementation does not actually create any UI but use
  // a  native file browser instantiated and deleted in Invoke().
  // If you implement your own UI to create a file dialog in a subclass,
  // reimplement your own Invoke() and do not call this class's Invoke(), 
  // call vtkKWDialog::Invoke() instead.
  // This method returns a zero if the dialog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();

  // Description:
  // Set/Get the file types the dialog will open or save
  // Should be in TK format. Default is: "{{Text Document} {.txt}}"
  vtkSetStringMacro(FileTypes);
  vtkGetStringMacro(FileTypes);

  // Description:
  // Set/Get the file path that the user selected
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  // Description:
  // Set/Get default extension.
  vtkSetStringMacro(DefaultExtension);
  vtkGetStringMacro(DefaultExtension);
  
  // Description:
  // Set /Geta filename to be displayed in the dialog when it pops up
  vtkSetStringMacro(InitialFileName);
  vtkGetStringMacro(InitialFileName);

  // Description:
  // If set, the dialog will be a save file dialog, otherwise a load dialog
  vtkSetClampMacro(SaveDialog, int, 0, 1);
  vtkBooleanMacro(SaveDialog, int);
  vtkGetMacro(SaveDialog, int);

  // Description:
  // Set/Get the ChooseDirectory ivar.
  // If set to 1, the dialog will ask the user for a directory, not a file
  // If set to 0, the usual file dialog is displayed (default)
  vtkSetClampMacro(ChooseDirectory, int, 0, 1);
  vtkBooleanMacro(ChooseDirectory, int);
  vtkGetMacro(ChooseDirectory, int);

  // Description:
  // Set/Get the MultipleSelection ivar.
  // If set to 1, the dialog will allow multiple files to be selected.
  // If set to 0, only a single file can be selected (default)
  vtkSetClampMacro(MultipleSelection, int, 0, 1);
  vtkBooleanMacro(MultipleSelection, int);
  vtkGetMacro(MultipleSelection, int);

  // Description:
  // Get the number of filenames that are selected.  This is meant
  // for use with MultipleSelection mode.
  int GetNumberOfFileNames();

  // Description:
  // Get one file path from a multiple selection of file paths.  An
  // error will be generated if the index is out of range.
  const char *GetNthFileName(int i);

  // Description:
  // Get all of the selected filenames as a vtkStringArray.
  vtkGetObjectMacro(FileNames, vtkStringArray);  

  // Description:
  // Set/Get last path
  vtkGetStringMacro(LastPath);
  vtkSetStringMacro(LastPath);

  // Description:
  // Figure out last path out of the file name.
  const char* GenerateLastPath(const char* path);

  // Description:
  // Save/retrieve the last path to/from the registry.
  // Note that the subkey used here is "RunTime".
  virtual void SaveLastPathToRegistry(const char *key);
  virtual void RetrieveLastPathFromRegistry(const char *key);

protected:
  vtkKWLoadSaveDialog();
  ~vtkKWLoadSaveDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  char *FileTypes;
  char *InitialFileName;
  char *FileName;
  char *DefaultExtension;
  char *LastPath;
  int SaveDialog;
  int ChooseDirectory;
  int MultipleSelection;
  vtkStringArray *FileNames;

private:
  vtkKWLoadSaveDialog(const vtkKWLoadSaveDialog&); // Not implemented
  void operator=(const vtkKWLoadSaveDialog&); // Not implemented
};


#endif



