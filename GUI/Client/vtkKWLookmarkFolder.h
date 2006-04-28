/*=========================================================================

  Program:   ParaView
  Module:    vtkKWLookmarkFolder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkKWLookmarkFolder - An interface widget for a container of lookmarks in the Lookmark Manager
// .SECTION Description
//
// .SECTION See Also
// vtkKWLookmark vtkPVLookmarkManager vtkPVLookmark

#ifndef __vtkKWLookmarkFolder_h
#define __vtkKWLookmarkFolder_h

#include "vtkKWCompositeWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWFrameWithLabel;
class vtkKWCheckButton;
class vtkKWText;
class vtkKWLabel;

class VTK_EXPORT vtkKWLookmarkFolder : public vtkKWCompositeWidget
{
public:
  static vtkKWLookmarkFolder* New();
  vtkTypeRevisionMacro(vtkKWLookmarkFolder,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callback to menu item in lookmark manager to rename a folder. Pressing 'Return' calls ChangeName
  void EditCallback();
  void ChangeName();

  // Description:
  // Deletes folder
  void RemoveFolder();

  // Description:
  // When a folder's checkbox is selected, we want all nested lmk items to also be selected (and vice versa)
  // When a folders lable is pressed and highlighted, we want all nested lmk items to also be highlighted
  void SelectCallback();
  void ToggleNestedLabels(vtkKWWidget *prnt, int state);
  void ToggleNestedCheckBoxes(vtkKWWidget *prnt, int state);

  // Description:
  // Set/Get methods that hide underlying widgets
  void SetFolderName(const char *val);
  char *GetFolderName();
  void SetSelectionState(int state);
  int GetSelectionState();

  vtkSetMacro(MacroFlag,int);
  vtkGetMacro(MacroFlag,int);

  vtkGetMacro(MainFrameCollapsedState,int);
  vtkSetMacro(MainFrameCollapsedState,int);

  vtkGetMacro(Location,int);
  vtkSetMacro(Location,int);

  // Direct Access to underlying widgets:
  vtkGetObjectMacro(LabelFrame,vtkKWFrameWithLabel);
  vtkGetObjectMacro(SeparatorFrame,vtkKWFrame);
  vtkGetObjectMacro(NestedSeparatorFrame,vtkKWFrame);
  vtkGetObjectMacro(Checkbox,vtkKWCheckButton);

  virtual void Pack();

  // Description:
  // Update the widget based on either its widget values or its variable values
  void UpdateWidgetValues();
  void UpdateVariableValues();

  // Description:
  // Drag and Drop routines
  void DragAndDropStartCallback(int x, int y);
  void DragAndDropEndCallback(int x, int y);
  void DragAndDropPerformCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor);
  void RemoveDragAndDropTargetCues();

  virtual void UpdateEnableState();

protected:
  vtkKWLookmarkFolder();
  ~vtkKWLookmarkFolder();

  // Description:
  // Create the widget
  virtual void CreateWidget();

  vtkKWFrame *MainFrame;
  vtkKWFrameWithLabel *LabelFrame;
  vtkKWFrame *SeparatorFrame;
  vtkKWFrame *NestedSeparatorFrame;
  vtkKWCheckButton *Checkbox;

  vtkKWText *NameField;
  int SelectionFlag;
  int MacroFlag;

  // This lmk container's location amongst its sibling lmk items
  int Location;
  int MainFrameCollapsedState;

private:
  vtkKWLookmarkFolder(const vtkKWLookmarkFolder&); // Not implemented
  void operator=(const vtkKWLookmarkFolder&); // Not implemented
};

#endif
