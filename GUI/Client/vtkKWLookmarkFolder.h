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
  // Create the widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Callback to double-clicking the lookmark container's label. Pressing 'Return' then calls ChangeName
  void EditCallback();
  void ChangeName();

  void RemoveFolder();

  // Description:
  // When a lmk container's checkbox is selected, we want all nested lmk items to also be selected (and vice versa)
  void SelectCallback();
  void ToggleNestedLabels(vtkKWWidget *prnt, int state);
  void ToggleNestedCheckBoxes(vtkKWWidget *prnt, int state);

  // Description:
  // Set/Get methods that hide underlying widgets
  void SetFolderName(const char *val);
  char *GetFolderName();

  // Direct Access to underlying widgets:
  vtkGetObjectMacro(LabelFrame,vtkKWFrameWithLabel);
  vtkGetObjectMacro(SeparatorFrame,vtkKWFrame);
  vtkGetObjectMacro(NestedSeparatorFrame,vtkKWFrame);

  vtkGetMacro(Location,int);
  vtkSetMacro(Location,int);

  virtual void Pack();

  void SetSelectionState(int state);
  int GetSelectionState();

  void DragAndDropStartCallback(int x, int y);
  void DragAndDropEndCallback(int x, int y);

  virtual void UpdateEnableState();

  void DragAndDropPerformCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor);
  void RemoveDragAndDropTargetCues();

  vtkSetMacro(MacroFlag,int);
  vtkGetMacro(MacroFlag,int);

  vtkGetObjectMacro(Checkbox,vtkKWCheckButton);

  vtkGetMacro(MainFrameCollapsedState,int);
  vtkSetMacro(MainFrameCollapsedState,int);

  void UpdateWidgetValues();
  void UpdateVariableValues();

protected:

  vtkKWLookmarkFolder();
  ~vtkKWLookmarkFolder();

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
