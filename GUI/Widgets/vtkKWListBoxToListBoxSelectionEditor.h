/*=========================================================================

  Module:    vtkKWListBoxToListBoxSelectionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWListBoxToListBoxSelectionEditor - a composite dual-listbox selection editor
// .SECTION Description
// This composite widget is used to manage a selection of text entries between
// two listboxes. The source listbox lists the available elements that can
// be add/removed/sorted to form a selection inside a target/final listbox.

#ifndef __vtkKWListBoxToListBoxSelectionEditor_h
#define __vtkKWListBoxToListBoxSelectionEditor_h

#include "vtkKWCompositeWidget.h"

class vtkKWListBoxWithScrollbars;
class vtkKWPushButton;
class vtkKWListBox;

class KWWidgets_EXPORT vtkKWListBoxToListBoxSelectionEditor : public vtkKWCompositeWidget
{
public:
  static vtkKWListBoxToListBoxSelectionEditor* New();
  vtkTypeRevisionMacro(vtkKWListBoxToListBoxSelectionEditor,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a string element to the source list if it is not already there or on
  // the final list. The optional argument force will make sure the item is
  // added to the source list and removed from final if it is already there.
  virtual void AddSourceElement(const char*, int force = 0);

  // Description:
  // Add a string element to the final list if it is not already there or on
  // the final list. The optional argument force will make sure the item is
  // added to the final list and removed from source if it is already there.
  virtual void AddFinalElement(const char*, int force = 0);

  // Description:
  // Get the number of elements on the final list.
  virtual int GetNumberOfElementsOnSourceList();
  virtual int GetNumberOfElementsOnFinalList();

  // Description:
  // Get the element from the list.
  virtual const char* GetElementFromSourceList(int idx);
  virtual const char* GetElementFromFinalList(int idx);

  // Description:
  // Get the index of the item.
  virtual int GetElementIndexFromSourceList(const char* element);
  virtual int GetElementIndexFromFinalList(const char* element);

  // Description:
  // Remove items from the list.
  virtual void RemoveItemsFromSourceList();
  virtual void RemoveItemsFromFinalList();

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the ellipsis button is pressed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetEllipsisCommand(vtkObject *obj, const char *method);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void AddCallback();
  virtual void AddAllCallback();
  virtual void RemoveCallback();
  virtual void RemoveAllCallback();
  virtual void UpCallback();
  virtual void DownCallback();
  virtual void EllipsisCallback();

protected:
  vtkKWListBoxToListBoxSelectionEditor();
  ~vtkKWListBoxToListBoxSelectionEditor();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWListBoxWithScrollbars* SourceList;
  vtkKWListBoxWithScrollbars* FinalList;

  vtkKWPushButton* AddButton;
  vtkKWPushButton* AddAllButton;
  vtkKWPushButton* RemoveButton;
  vtkKWPushButton* RemoveAllButton;
  vtkKWPushButton* UpButton;
  vtkKWPushButton* DownButton;

  virtual void MoveWholeList(vtkKWListBox* l1, vtkKWListBox* l2);
  virtual void MoveSelectedList(vtkKWListBox* l1, vtkKWListBox* l2);
  virtual void MoveList(vtkKWListBox* l1, vtkKWListBox* l2, const char* list);
  virtual void ShiftItems(vtkKWListBox* l1, int down);
  virtual void AddElement(vtkKWListBox* l1, vtkKWListBox* l2, const char* element, int force);

  char* EllipsisCommand;
  virtual void InvokeEllipsisCommand();

  int EllipsisDisplayed;

  virtual void DisplayEllipsis();
  virtual void RemoveEllipsis();
  
private:
  vtkKWListBoxToListBoxSelectionEditor(const vtkKWListBoxToListBoxSelectionEditor&); // Not implemented
  void operator=(const vtkKWListBoxToListBoxSelectionEditor&); // Not Implemented
};


#endif



