/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWPopupButton - a button that triggers a load/save dialog
// .SECTION Description
// The vtkKWPopupButton class creates a push button that
// will popup a vtkKWLoadSaveDialog and display the chosen filename as
// the button label.

#ifndef __vtkKWLoadSaveButton_h
#define __vtkKWLoadSaveButton_h

#include "vtkKWPushButton.h"

class vtkKWLoadSaveDialog;

class VTK_EXPORT vtkKWLoadSaveButton : public vtkKWPushButton
{
public:
  static vtkKWLoadSaveButton* New();
  vtkTypeRevisionMacro(vtkKWLoadSaveButton, vtkKWPushButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Access to sub-widgets.
  vtkGetObjectMacro(LoadSaveDialog, vtkKWLoadSaveDialog);

  // Description:
  // Convenience method to retrieve the filename.
  virtual char* GetFileName();

  // Description:
  // Set/Get the length of the filename when displayed in the button.
  // If set to 0, do not shorten the filename.
  virtual void SetMaximumFileNameLength(int);
  vtkGetMacro(MaximumFileNameLength, int);

  // Description:
  // Set/Get if the path of the filename should be trimmed when displayed in
  // the button.
  virtual void SetTrimPathFromFileName(int);
  vtkBooleanMacro(TrimPathFromFileName, int);
  vtkGetMacro(TrimPathFromFileName, int);
  
  // Description:
  // Override vtkKWWidget's SetCommand so that the button command callback
  // will invoke the load/save dialog, then invoke a user-defined command.
  virtual void SetCommand(vtkKWObject *object, const char *method);

  // Description:
  // Callbacks.
  virtual void InvokeLoadSaveDialogCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLoadSaveButton();
  ~vtkKWLoadSaveButton();

  vtkKWLoadSaveDialog *LoadSaveDialog;

  int TrimPathFromFileName;
  int MaximumFileNameLength;
  virtual void UpdateFileName();

  char *UserCommand;
  vtkSetStringMacro(UserCommand);
  vtkGetStringMacro(UserCommand);

private:
  vtkKWLoadSaveButton(const vtkKWLoadSaveButton&); // Not implemented
  void operator=(const vtkKWLoadSaveButton&); // Not implemented
};

#endif

