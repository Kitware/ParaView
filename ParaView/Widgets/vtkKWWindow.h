/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWindow.h
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
// .NAME vtkKWWindow - a window superclass which holds one or more views
// .SECTION Description
// This class represents a top level window with menu bar and status
// line. It is designed to hold one or more vtkKWViews in it.

#ifndef __vtkKWWindow_h
#define __vtkKWWindow_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWSplitFrame;
class vtkKWNotebook;
class vtkKWViewCollection;
class vtkKWMenu;
class vtkKWProgressGauge;
class vtkKWView;

class VTK_EXPORT vtkKWWindow : public vtkKWWidget
{
public:
  static vtkKWWindow* New();
  vtkTypeMacro(vtkKWWindow,vtkKWWidget);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description::
  // Exit this application closing all windows.
  virtual void Exit();

  // Description::
  // Close this window, possibly exiting the application if no more
  // windows are open.
  virtual void Close();

  // Description::
  // Display help info for this window.
  virtual void DisplayHelp();

  // Description::
  // Display about info for this window.
  virtual void DisplayAbout();

  // Description:
  // Set the text for the status bar of this window.
  void SetStatusText(const char *);
  
  // Description:
  // Load in a Tcl based script to drive the application. If called
  // without an argument it will open a file dialog.
  void LoadScript();
  void LoadScript(char *name);
  
  // Description:
  // Allow windows to get at the different menu entries. In some
  // cases the menu entry may be created if it doesn't already
  // exist.
  vtkGetObjectMacro(Menu,vtkKWMenu);
  vtkGetObjectMacro(MenuFile,vtkKWMenu);
  vtkKWMenu *GetMenuEdit();
  vtkKWMenu *GetMenuView();
  vtkKWMenu *GetMenuProperties();
  
  // Description:
  // Operations on the views.
  void AddView(vtkKWView *);
  void RemoveView(vtkKWView *);
  void SetSelectedView(vtkKWView *);
  vtkGetObjectMacro(SelectedView,vtkKWView);
  vtkKWViewCollection *GetViews() {return this->Views;};
  vtkGetObjectMacro(ViewFrame,vtkKWWidget);
  
  // Description:
  // Proiperties may be bound to the window or view or
  // something else. The CreateDefaultPropertiesParent method
  // will create an attachment point for the properties at
  // the window level.
  vtkGetObjectMacro(PropertiesParent,vtkKWWidget);
  vtkSetObjectMacro(PropertiesParent,vtkKWWidget);
  void CreateDefaultPropertiesParent();
  void HideProperties();
  void ShowProperties();
  
  // Description::
  // Override Unregister since widgets have loops.
  void UnRegister(vtkObject *o);

  // Description::
  // Add to the menu a list of recently used files. Specify a key,
  // which if it is null it will just use the classname. The command
  // is the command to execute when a file is selected.
  virtual void AddRecentFilesToMenu(char *key, vtkKWObject *target);
  virtual void AddRecentFile(char *key, char *name, vtkKWObject *target,
                             const char *command);
  
  // Description:
  // Return the index of the entry above the MRU File list
  // in the file menu. This is useful because most menu options
  // go above the MRU list, hence above this index.
  int GetFileMenuIndex();

  // Description:
  // Install a menu bar into this window.
  void InstallMenu(vtkKWMenu* menu);

  // Description:
  // Callbacks used to set the print quality
  void OnPrint1();
  void OnPrint2();
  void OnPrint3();
  vtkGetMacro(PrintTargetDPI,float);
  
  // Description:
  // Allow access to the notebook object.
  vtkGetObjectMacro(Notebook,vtkKWNotebook);

  // Description:
  // This toolbar frame is below the menu. It is empty initially.
  // Subclasses can add toolbars buttons as necessary.
  vtkGetObjectMacro(ToolbarFrame, vtkKWWidget);

  // Description:
  // Get the progress gauge widget.  The progress gauge is displayed
  // in the Status frame on the bottom right of the window.
  vtkGetObjectMacro(ProgressGauge, vtkKWProgressGauge);
 
  // Description:
  // Will the window add a help menu?
  vtkSetClampMacro( SupportHelp, int, 0, 1 );
  vtkGetMacro( SupportHelp, int );
  vtkBooleanMacro( SupportHelp, int );

  // Description:
  // Class of the window. Passed to the toplevel command.
  vtkSetStringMacro(WindowClass);
  vtkGetStringMacro(WindowClass);

protected:
  vtkKWWindow();
  ~vtkKWWindow();
  vtkKWWindow(const vtkKWWindow&) {};
  void operator=(const vtkKWWindow&) {};
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  vtkKWNotebook *Notebook;
  virtual void CreateStatusImage();
  int NumberOfMRUFiles;
  vtkKWView *SelectedView;
  vtkKWViewCollection *Views;
  vtkKWMenu *Menu;
  vtkKWMenu *MenuFile;
  vtkKWMenu *MenuProperties;
  vtkKWMenu *MenuEdit;
  vtkKWMenu *MenuView;
  vtkKWMenu *MenuHelp;
  vtkKWWidget *StatusFrame;
  vtkKWWidget *StatusImage;
  vtkKWWidget *StatusLabel;
  vtkKWProgressGauge* ProgressGauge;
  vtkKWWidget* ProgressFrame;
  char        *StatusImageName;
  vtkKWSplitFrame *MiddleFrame; // Contains view frame and properties parent.
  vtkKWWidget *PropertiesParent;
  vtkKWWidget *ViewFrame;
  vtkKWWidget *ToolbarFrame;
  float      PrintTargetDPI;
  vtkKWMenu *PageMenu;

  int SupportHelp;
  char *WindowClass;
};


#endif


