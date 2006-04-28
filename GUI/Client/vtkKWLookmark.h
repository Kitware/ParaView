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

#include "vtkKWCompositeWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWFrameWithLabel;
class vtkKWCheckButton;
class vtkKWText;
class vtkKWPushButton;

class VTK_EXPORT vtkKWLookmark : public vtkKWCompositeWidget
{
public:
  static vtkKWLookmark* New();
  vtkTypeRevisionMacro(vtkKWLookmark,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Access to underlying widgets
  vtkGetObjectMacro(SeparatorFrame,vtkKWFrame);
  vtkGetObjectMacro(Checkbox,vtkKWCheckButton);

  // Description:
  // When EditLookmarkCallback is called, an editable text widget appears in place of the label and contains the old name.
  // The user edits this as appropriate and pressing 'Return' then calls ChangeLookmarkName
  void EditLookmarkCallback();
  void ChangeLookmarkName();

  // Description:
  // Updates the comments frame label to display the first few words when modified
  void CommentsModifiedCallback();

  // Description:
  // Breaks up the dataset string into an array
  void CreateDatasetList();

  // Description:
  // Convenience method for encapsulating and reusing code that packs this widget
  void Pack();

  // Description:
  // Drag and Drop functionality - highlight frame when moused over
  void DragAndDropPerformCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor);
  void RemoveDragAndDropTargetCues();

  // Description:
  // Update the widget based on either its widget values or its variable values
  void UpdateVariableValues();

  vtkSetMacro(MacroFlag,int);
  vtkGetMacro(MacroFlag,int);

  vtkGetMacro(MainFrameCollapsedState,int);
  vtkSetMacro(MainFrameCollapsedState,int);
  vtkGetMacro(CommentsFrameCollapsedState,int);
  vtkSetMacro(CommentsFrameCollapsedState,int);

  vtkSetMacro(PixelSize,int);
  vtkGetMacro(PixelSize,int);
  vtkSetMacro(Width,int);
  vtkGetMacro(Width,int);
  vtkSetMacro(Height,int);
  vtkGetMacro(Height,int);

  // Description:
  // Uses the vtkKWIcon to initialize the lookmark's Icon
  void SetIcon(vtkKWIcon *icon);

  virtual void UpdateEnableState();

protected:
  vtkKWLookmark();
  ~vtkKWLookmark();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWFrame *LeftFrame;
  vtkKWFrame *RightFrame;
  vtkKWFrame *Frame;
  vtkKWFrameWithLabel *MainFrame;
  vtkKWFrameWithLabel *CommentsFrame;
  vtkKWLabel *DatasetLabel;
  vtkKWFrame *DatasetFrame;
  vtkKWPushButton *Icon;
  vtkKWText *CommentsText;
  vtkKWText *NameField;
  vtkKWFrame *SeparatorFrame;
  vtkKWCheckButton *Checkbox;

  char* Dataset;
  char** DatasetList;
  char* Name;
  char* Comments;
  int Width;
  int Height;
  int PixelSize;

  int SelectionFlag;
  int MacroFlag;
  int MainFrameCollapsedState;
  int CommentsFrameCollapsedState;

private:
  vtkKWLookmark(const vtkKWLookmark&); // Not implemented
  void operator=(const vtkKWLookmark&); // Not implemented
};

#endif
