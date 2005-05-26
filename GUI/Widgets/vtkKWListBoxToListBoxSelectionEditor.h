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

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWListBox;
class vtkKWPushButton;

class KWWIDGETS_EXPORT vtkKWListBoxToListBoxSelectionEditor : public vtkKWWidget
{
public:
  static vtkKWListBoxToListBoxSelectionEditor* New();
  vtkTypeRevisionMacro(vtkKWListBoxToListBoxSelectionEditor,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

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
  // Callbacks.
  virtual void AddCallback();
  virtual void AddAllCallback();
  virtual void RemoveCallback();
  virtual void RemoveAllCallback();
  virtual void UpCallback();
  virtual void DownCallback();

  // Description:
  // Callback for ellipsis.
  void EllipsisCallback();
  vtkSetStringMacro(EllipsisCommand);
  vtkGetStringMacro(EllipsisCommand);
  void SetEllipsisCommand(vtkKWObject* obj, const char* command);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWListBoxToListBoxSelectionEditor();
  ~vtkKWListBoxToListBoxSelectionEditor();

  vtkKWListBox* SourceList;
  vtkKWListBox* FinalList;

  vtkKWPushButton* AddButton;
  vtkKWPushButton* AddAllButton;
  vtkKWPushButton* RemoveButton;
  vtkKWPushButton* RemoveAllButton;
  vtkKWPushButton* UpButton;
  vtkKWPushButton* DownButton;

  void MoveWholeList(vtkKWListBox* l1, vtkKWListBox* l2);
  void MoveSelectedList(vtkKWListBox* l1, vtkKWListBox* l2);
  void MoveList(vtkKWListBox* l1, vtkKWListBox* l2, const char* list);
  void ShiftItems(vtkKWListBox* l1, int down);
  void AddElement(vtkKWListBox* l1, vtkKWListBox* l2, const char* element, int force);

  char* EllipsisCommand;
  int EllipsisDisplayed;

  void DisplayEllipsis();
  void RemoveEllipsis();
  
private:
  vtkKWListBoxToListBoxSelectionEditor(const vtkKWListBoxToListBoxSelectionEditor&); // Not implemented
  void operator=(const vtkKWListBoxToListBoxSelectionEditor&); // Not Implemented
};


#endif



