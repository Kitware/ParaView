/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkKWWindow
// .SECTION Description
// This class represents a top level window with menu bar and status
// line. It is designed to hold one or more vtkKWViews in it.

#ifndef __vtkKWWindow_h
#define __vtkKWWindow_h

#include "vtkKWWidget.h"
class vtkKWApplication;
#include "vtkKWViewCollection.h"
#include "vtkKWMenu.h"

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
  
  // Description::
  // Override Unregister since widgets have loops.
  void UnRegister(vtkObject *o);

  // Description::
  // Add to the menu a list of recently used files. Specify a key,
  // which if it is null it will just use the classname. The command
  // is the command to execute when a file is selected.
  virtual void AddRecentFilesToMenu(char *key, vtkKWObject *target, 
                                    const char *command);
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
  // This toolbar frame is below the menu. It is empty initially.
  // Subclasses can add toolbars buttons as necessary.
  vtkGetObjectMacro(ToolbarFrame, vtkKWWidget);
 
protected:
  vtkKWWindow();
  ~vtkKWWindow();
  vtkKWWindow(const vtkKWWindow&) {};
  void operator=(const vtkKWWindow&) {};

  void CreateStatusImage();
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
  char        *StatusImageName;
  vtkKWWidget *MiddleFrame; // Contains view frame and properties parent.
  vtkKWWidget *PropertiesParent;
  vtkKWWidget *ViewFrame;
  vtkKWWidget *ToolbarFrame;
};


#endif


