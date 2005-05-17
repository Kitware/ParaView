/*=========================================================================

  Program:   ParaView
  Module:    vtkKWLookmark.h

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


// .NAME vtkKWLookmark - An interface widget for a lookmark.
// .SECTION Description
//
// This is the interface widget for lookmarks. It is made up of a labeled frame, checkbox, icon, and text widget for comments
// 
// .SECTION See Also
// vtkKWLookmarkContainer vtkPVLookmarkManager vtkPVLookmark

#ifndef __vtkKWLookmark_h
#define __vtkKWLookmark_h

#include "vtkKWWidget.h"

class vtkKWCheckButtonLabeled;
class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWFrameLabeled;
class vtkKWCheckButton;
class vtkPVCameraIcon;
class vtkKWText;
class vtkKWCheckButtonLabeled;
class vtkKWRadioButtonSet;


class VTK_EXPORT vtkKWLookmark : public vtkKWWidget
{
public:
  static vtkKWLookmark* New();
  vtkTypeRevisionMacro(vtkKWLookmark,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // The name of the lookmark. Always the same as the one displayed in the lookmark widget.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // This is only allocated and written to right before a lookmark is about to be written to a lookmark file.
  // The newlines contained herein are encoded to '~' before writing because they are lost when the ->SetObject(vtkPVLookmark) method is called
  vtkGetStringMacro(Comments);
  vtkSetStringMacro(Comments);

  // Description:
  // The full path to the lookmark's 'default dataset'. This is originally just set to the dataset from which the lookmark was created but can later be set to 
  // a different one by turning 'Use default dataset' option OFF in the loookmark manager and setting the 'use as default dataset' option in the dialog box
  vtkGetStringMacro(Dataset);
  vtkSetStringMacro(Dataset);

  // Description:
  // Access to the state of the checkbutton
  void SetSelectionState(int state);
  int GetSelectionState();

  // Description:
  // Made available for vtkPVLookmarkManager's management of drag-and-drop targets
  vtkGetObjectMacro(SeparatorFrame,vtkKWFrame);

  // Description:
  // If TRUE, use the default dataset when loading lookmark, else use the currently selected one in source list
  int IsLockedToDataset();
  
  // Description:
  // When EditLookmarkCallback is called, an editable text widget appears in place of the label and contains the old name.
  // The user edits this as appropriate and pressing 'Return' then calls ChangeLookmarkName
  void EditLookmarkCallback();
  void ChangeLookmarkName();

  // Description:
  // COnvenience method for encapsulating and reusing code that packs this widget
  // the argument tells Pack() whether to display the hidden widgets or not
  void Pack();

  void DragAndDropPerformCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor);
  void RemoveDragAndDropTargetCues();

  vtkSetMacro(PixelSize,int);
  vtkGetMacro(PixelSize,int);
  vtkSetMacro(Width,int);
  vtkGetMacro(Width,int);
  vtkSetMacro(Height,int);
  vtkGetMacro(Height,int);

  // Description:
  // Uses the name and comments widget values (which may have been modified) to initialize LookmarkName and Comments variables
  void UpdateVariableValues();
  void UpdateWidgetValues();

  virtual void UpdateEnableState();

protected:

  vtkKWLookmark();
  ~vtkKWLookmark();

  vtkKWFrame *LmkLeftFrame;
  vtkKWFrame *LmkRightFrame;
  vtkKWFrame *LmkFrame;
  vtkKWFrameLabeled *LmkMainFrame;
  vtkKWFrameLabeled *LmkCommentsFrame;
  vtkKWLabel *LmkDatasetLabel;
  vtkKWCheckButtonLabeled *LmkDatasetCheckbox;
  vtkKWFrame *LmkDatasetFrame;
  vtkKWLabel *LmkIcon;
  vtkKWText *LmkCommentsText;
  vtkKWText *LmkNameField;
  vtkKWFrame *SeparatorFrame;
  vtkKWCheckButton *Checkbox;

  char* Dataset;
  char* Name;
  char* Comments;
  int Width;
  int Height;
  int PixelSize;

  int SelectionFlag;

private:
  vtkKWLookmark(const vtkKWLookmark&); // Not implemented
  void operator=(const vtkKWLookmark&); // Not implemented
};

#endif
