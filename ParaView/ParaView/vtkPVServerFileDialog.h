/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVServerFileDialog.h
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
// .NAME vtkPVServerFileDialog - For opening remote files on server.
// .SECTION Description
// A dialog to replace Tk's Open and Save file dialogs.
// We will develop the dialog for local (opening) files first ...

#ifndef __vtkPVServerFileDialog_h
#define __vtkPVServerFileDialog_h

#include "vtkKWLoadSaveDialog.h"
#include "vtkClientServerID.h" // Need vtkClientServerID.
class vtkKWApplication;
class vtkPVApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWListBox;
class vtkKWWindow;
class vtkKWMenuButton;
class vtkIntArray;
class vtkStringList;

class VTK_EXPORT vtkPVServerFileDialog : public vtkKWLoadSaveDialog
{
public:
  static vtkPVServerFileDialog* New();
  vtkTypeRevisionMacro(vtkPVServerFileDialog, vtkKWLoadSaveDialog);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set the window to which the dialog will be slave.
  // If set, this dialog will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this)
  void SetMasterWindow(vtkKWWindow* win);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();
  
  // Description:
  // Button callbacks;
  void LoadSaveCallback();
  void CancelCallback();
  void SelectFile(const char* name, const char* id);
  void SelectDirectory(const char* name, const char* id);
  void DownDirectoryCallback();
  void ExtensionsMenuButtonCallback(int typeIdx);

  // Description:
  // Cast vtkKWApplication to vtkPVApplication.
  vtkPVApplication* GetPVApplication();

  // Description:
  // This method is called when canvas size changes.
  virtual void Reconfigure();
 
protected:
  vtkPVServerFileDialog();
  ~vtkPVServerFileDialog();

  void Update();
  int Insert(const char* name, int y, int directory);

  // Get rid of backslashes.
  void ConvertLastPath();

  int ReturnValue;
  
  vtkKWWindow*      MasterWindow;

  vtkKWWidget*      TopFrame;
  vtkKWFrame*       MiddleFrame;
  vtkKWWidget*      FileList;
  vtkKWWidget*      BottomFrame;

  vtkKWLabel*       DirectoryDisplay;
  vtkKWMenuButton*  DirectoryMenuButton;

  vtkKWLabel*       FileNameLabel;
  vtkKWEntry*       FileNameEntry;
  vtkKWMenuButton*  FileNameMenuButton;

  vtkKWLabel*       ExtensionsLabel;
  vtkKWWidget*      ExtensionsDisplayFrame;
  vtkKWLabel*       ExtensionsDisplay;
  vtkKWMenuButton*  ExtensionsMenuButton;

  vtkKWPushButton*  LoadSaveButton;
  vtkKWPushButton*  CancelButton;

  vtkKWLabel*       DownDirectoryButton;
    
  char*             SelectBoxId;
  vtkSetStringMacro(SelectBoxId);

  char*             SelectedDirectory;
  vtkSetStringMacro(SelectedDirectory);

  void UpdateExtensionsMenu();
  vtkStringList*    FileTypeStrings;
  vtkStringList*    FileTypeDescriptions;
  vtkStringList*    ExtensionStrings;
  int               CheckExtension(const char* name);

  // Server-side helper.
  vtkClientServerID ServerSideID;
  void CreateServerSide();

  vtkKWWidget* ScrollBar;
  // Description:
  // This method calculates the bounding box of object "name". 
  void CalculateBBox(vtkKWWidget* canvas, const char* name, int bbox[4]);

private:
  vtkPVServerFileDialog(const vtkPVServerFileDialog&); // Not implemented
  void operator=(const vtkPVServerFileDialog&); // Not implemented
};

#endif
