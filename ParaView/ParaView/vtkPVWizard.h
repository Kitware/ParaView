/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWizard.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVWizard - dialog box superclass
// .SECTION Description
// A generic superclass for dialog boxes.

#ifndef __vtkPVWizard_h
#define __vtkPVWizard_h

#include "vtkKWWidget.h"
#include "vtkRectilinearGrid.h"
class vtkKWApplication;
class vtkKWWindow;
class vtkPVWindow;
class vtkPVFileEntry;
class vtkKWPushButton;
class vtkKWLabel;
class vtkCollection;

class VTK_EXPORT vtkPVWizard : public vtkKWWidget
{
public:
  static vtkPVWizard* New();
  vtkTypeMacro(vtkPVWizard,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

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
  vtkSetObjectMacro(Data, vtkRectilinearGrid);
  vtkSetStringMacro(String);

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
  vtkSetStringMacro(LastFileName);
  char *LastFileName;
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


