/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWizard.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWizard - dialog box superclass
// .SECTION Description
// A generic superclass for dialog boxes.

#ifndef __vtkPVWizard_h
#define __vtkPVWizard_h

#include "vtkKWWidget.h"

class vtkCollection;
class vtkKWApplication;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWWindow;
class vtkPVFileEntry;
class vtkPVWindow;
class vtkRectilinearGrid;

class VTK_EXPORT vtkPVWizard : public vtkKWWidget
{
public:
  static vtkPVWizard* New();
  vtkTypeRevisionMacro(vtkPVWizard,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke(vtkPVWindow *pvWin);

  // Description::
  // Called by the next button.
  virtual void NextCallback();

  // Description::
  // Called by the calcel button.
  virtual void CancelCallback();

  // Description:
  // Used internally to wee if the file entered is valid.
  void CheckForValidFile(int gate);

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
  // A hack to get the data from the reader through Tcl.
  virtual void SetData(vtkRectilinearGrid*);
  vtkGetObjectMacro(Data, vtkRectilinearGrid);
  vtkSetStringMacro(String);
  vtkGetStringMacro(String);

protected:
  vtkPVWizard();
  ~vtkPVWizard();

  // Description:
  // Set the title string of the dialog window. Should be called before
  // create otherwise it will have no effect.
  vtkSetStringMacro(TitleString);

  // Description:
  // The series of steps to gather information.
  void QueryFirstFileName();
  void QueryLastFileName();
  void QueryStride();
  void QueryMaterials();
  void QueryColorVariable();
  void QueryColoredMaterials();
  void SetupPipeline(vtkPVWindow *pvWin);

  // Description:
  // This extracts the file pattern and numbers from the first and last file.
  // If there is an error, then done is set to 0.
  void CheckForFilePattern();

  vtkKWWidget *WizardFrame;
  vtkKWLabel *Label;

  vtkKWWidget *ButtonFrame;
  vtkKWPushButton *NextButton;
  vtkKWPushButton *CancelButton;

  vtkKWWindow* MasterWindow;

  char *TitleString;
  int Done;

  vtkSetStringMacro(FirstFileName);
  char *FirstFileName;
  int   FirstFileNumber;
  vtkSetStringMacro(LastFileName);
  char *LastFileName;
  int   LastFileNumber;
  vtkSetStringMacro(FilePattern);
  char *FilePattern;
  int Stride;
  vtkCollection *MaterialChecks;
  vtkSetStringMacro(ColorArrayName);
  char *ColorArrayName;

  // Hack to get information.
  vtkRectilinearGrid *Data;
  char *String;

  // I hate doing this.
  // If we move this wizard over into the module, 
  // we can create the reader directly.
  char *ReaderTclName;
  vtkSetStringMacro(ReaderTclName);
  vtkPVFileEntry *FileEntry;

private:
  vtkPVWizard(const vtkPVWizard&); // Not implemented
  void operator=(const vtkPVWizard&); // Not Implemented
};


#endif


