/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLoadSaveDialog.h
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
// .NAME vtkKWLoadSaveDialog - dalog for loading or saving files
// .SECTION Description
// A dialog for loading or saving files. The file is returned through 
// GetFile method.

#ifndef __vtkKWLoadSaveDialog_h
#define __vtkKWLoadSaveDialog_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWLoadSaveDialog : public vtkKWWidget
{
public:
  static vtkKWLoadSaveDialog* New();
  vtkTypeMacro(vtkKWLoadSaveDialog,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();

  // Description:
  // Returns 0 if the dialog is active e.g. displayed
  // 1 if it was Canceled 2 if it was OK.
  int GetStatus() 
    { return this->Done; };

  // Description:
  // Set the file types the dialog will open or save
  // Should be in TK format
  vtkSetStringMacro( FileTypes );

  // Description:
  // Set the initial directory for the dialog.
  vtkSetStringMacro(InitialDir);

  // Description:
  // Retrieve the file path that the user selected
  vtkGetStringMacro(OpenFile);

  // Description:
  // Set default extention.
  vtkSetStringMacro(DefaultExt);

  // Description:
  // Set or reset the SaveDialog. If set, the dialog will be
  // save file dialog. If reset, the dialog will be load 
  // dialog
  vtkSetClampMacro( SaveDialog, int, 0, 1 );
  vtkBooleanMacro( SaveDialog, int );
  vtkGetMacro( SaveDialog, int );

  // Description:
  // Set the title string of the dialog window. Should be called before
  // create otherwise it will have no effect.
  vtkSetStringMacro(Title);

protected:

  // Description:
  // Set the file path that the user selected
  vtkSetStringMacro(OpenFile);

  vtkKWLoadSaveDialog();
  ~vtkKWLoadSaveDialog();

  vtkKWLoadSaveDialog(const vtkKWLoadSaveDialog&) {};
  void operator=(const vtkKWLoadSaveDialog&) {};

    char *FileTypes;
    char *InitialDir;  
    char *Title;
    char *OpenFile;
    char *DefaultExt;

    int SaveDialog;
    int Done;
};


#endif


