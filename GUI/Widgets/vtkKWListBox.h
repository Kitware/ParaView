/*=========================================================================

  Module:    vtkKWListBox.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWListBox - List Box
// .SECTION Description
// A widget that can have a list of items. 
// Use vtkKWListBoxWithScrollbars if you need scrollbars.
// You can configure it into: single, browse, multiple, or extended
// Another important configuration option is: -exportselection 0
// Therefore it does not collide with let say tk_getSaveFile 
// selection
// .SECTION See Also
// vtkKWListBoxWithScrollbars

#ifndef __vtkKWListBox_h
#define __vtkKWListBox_h

#include "vtkKWCoreWidget.h"

class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWListBox : public vtkKWCoreWidget
{
public:
  static vtkKWListBox* New();
  vtkTypeRevisionMacro(vtkKWListBox,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set/Get the one of several styles for manipulating the selection. 
  // Valid constants can be found in vtkKWTkOptions::SelectionModeType.
  virtual void SetSelectionMode(int);
  virtual int GetSelectionMode();
  virtual void SetSelectionModeToSingle() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeSingle); };
  virtual void SetSelectionModeToBrowse() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeBrowse); };
  virtual void SetSelectionModeToMultiple() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeMultiple); };
  virtual void SetSelectionModeToExtended() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeExtended); };

  // Description:
  // Specifies whether or not a selection in the widget should also be the X
  // selection. If the selection is exported, then selecting in the widget
  // deselects the current X selection, selecting outside the widget deselects
  // any widget selection, and the widget will respond to selection retrieval
  // requests when it has a selection.  
  virtual void SetExportSelection(int);
  virtual int GetExportSelection();
  vtkBooleanMacro(ExportSelection, int);
  
  // Description:
  // Get the current selected string in the list.  This is used when
  // Select mode is single or browse.
  virtual const char *GetSelection();
  virtual int GetSelectionIndex();
  virtual void SetSelectionIndex(int);

  // Description:
  // When selectmode is multiple or extended, then these methods can
  // be used to set and query the selection.
  virtual void SetSelectState(int idx, int state);
  virtual int GetSelectState(int idx);
  
  // Description:
  // Add an entry.
  virtual void InsertEntry(int index, const char *name);

  // Description:
  // Append a unique string to the list. If the string exists,
  // it will not be appended
  virtual int AppendUnique(const char* name);
  
  // Description:
  // Set callback for single and double click on a list item.
  virtual void SetDoubleClickCommand(vtkObject *obj, const char *method);
  virtual void SetSingleClickCommand(vtkObject *obj, const char *method);
  
  // Description:
  // Get number of items in the list.
  virtual int GetNumberOfItems();
  
  // Description: 
  // Get the item at the given index.
  virtual const char* GetItem(int index);

  // Description:
  // Returns the index of the first given item.
  virtual int GetItemIndex(const char* item);
  
  // Description:
  // Delete a range of items in the list.
  virtual void DeleteRange(int start, int end);
  
  // Description:
  // Delete all items from the list.
  virtual void DeleteAll();
  
  // Description: 
  // Set the width of the list box. If the width is less than or equal to 0,
  // then the width is set to the size of the largest string.
  virtual void SetWidth(int);

  // Description: 
  // Set the height of the list box. If the height is less than or equal to 0,
  // then the height is set to the size of the number of items in the listbox.
  virtual void SetHeight(int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkKWListBox();
  ~vtkKWListBox();

  char* CurrentSelection;       // store last call of CurrentSelection
  char* Item;                   // store last call of GetItem
  
private:
  vtkKWListBox(const vtkKWListBox&); // Not implemented
  void operator=(const vtkKWListBox&); // Not implemented
};


#endif



