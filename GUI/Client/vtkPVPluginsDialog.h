/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVPluginsDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPluginsDialog - ParaView Plugins user interface.
// .SECTION Description
// An object of this class represents a dialog box for managing available
// plug-ins.
// 
// The caption is "Plug-ins". The bottom consists of 3 buttons: "Add a
// plug-in" to open a browsering dialog box to load a plug-in  manually,
// and a "Help" and "Close" button.
// The main window consists of a list of plug-ins. Each row is composed of
// six columns: name, type, loaded, auto load, path and information. The first
// row displays title for each column.
//
// .SECTION See Also
// vtkPVWindow

#ifndef __vtkPVPluginsDialog_h
#define __vtkPVPluginsDialog_h

#include "vtkKWDialog.h"

class vtkKWFrameLabeled;
class vtkKWPushButton;
class vtkKWFrame;

class VTK_EXPORT vtkPVPluginsDialog : public vtkKWDialog
{
 public:
  static vtkPVPluginsDialog* New();
  vtkTypeRevisionMacro(vtkPVPluginsDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app,
                      const char *args);
  
  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dialog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();
  
 protected:
  // Description:
  // Default constructor.
  vtkPVPluginsDialog();
  // Description:
  // Destructor.
  virtual ~vtkPVPluginsDialog();
  
  vtkKWFrameLabeled *PluginsFrame;
  vtkKWPushButton *NameButton;
  vtkKWPushButton *TypeButton;
  vtkKWPushButton *LoadedButton;
  vtkKWPushButton *AutoLoadButton;
  vtkKWPushButton *PathButton;
  vtkKWPushButton *InfoButton;
  
  vtkKWFrame *ButtonsFrame;
  vtkKWPushButton *AddPluginButton;
  vtkKWPushButton *HelpButton;
  vtkKWPushButton *CloseButton;
  
 private:
  vtkPVPluginsDialog(const vtkPVPluginsDialog&); // Not implemented
  void operator=(const vtkPVPluginsDialog&); // Not implemented
};

#endif // #ifndef __vtkPVPluginsDialog_h
