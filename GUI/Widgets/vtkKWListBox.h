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
// A widget that can have a list of items with a scroll bar. 
// It uses a scrollbar (yscroll only) and listbox
// You can configure it into: single, browse, multiple, or extended
// Another important configuration option is: -exportselection 0
// Therefore it does not collide with let say tk_getSaveFile 
// selection

#ifndef __vtkKWListBox_h
#define __vtkKWListBox_h

#include "vtkKWWidget.h"

class vtkKWApplication;

class VTK_EXPORT vtkKWListBox : public vtkKWWidget
{
public:
  static vtkKWListBox* New();
  vtkTypeRevisionMacro(vtkKWListBox,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Get the current selected string in the list.  This is used when
  // Select mode is single or browse.
  const char *GetSelection();
  int GetSelectionIndex();
  void SetSelectionIndex(int);

  // Description:
  // When selectmode is multiple or extended, then these methods can be used to set
  // and query the selection.
  void SetSelectState(int idx, int state);
  int GetSelectState(int idx);
  
  // Description:
  // Add entries to an option menu, with or without a command.
  void InsertEntry(int index, const char *name);

  // Description:
  // Append a unique string to the list.  If the string exists,
  // it will not be appended
  int AppendUnique(const char* name);
  
  // Description:
  // Set callback for double click on a list item.
  void SetDoubleClickCallback(vtkKWObject* obj, const char* methodAndArgs);
  
  // Description:
  // Set callback for single click on a list item.
  void SetSingleClickCallback(vtkKWObject* obj, const char* methodAndArgs);
  
  // Description:
  // Get number of items in the list.
  int GetNumberOfItems();
  
  // Description: 
  // Get the item at the given index.
  const char* GetItem(int index);

  // Description:
  // Returns the index of the first given item.
  int GetItemIndex(const char* item);
  
  // Description:
  // Delete a range of items in the list.
  void DeleteRange(int start, int end);
  
  // Description:
  // Delete all items from the list.
  void DeleteAll();
  
  // Description: 
  // Set the width of the list box.  If the width is less than or equal to 0,
  // then the width is set to the size of the largest string.
  void SetWidth(int);

  // Description: 
  // Set the height of the list box.  If the height is less than or equal to 0,
  // then the height is set to the size of the number of items in the listbox.
  void SetHeight(int);

  // Description:
  // Setting this string enables balloon help for this widget.
  // Override to pass down to children for cleaner behavior
  virtual void SetBalloonHelpString(const char *str);
    
  // Description:
  // Specify whether you want a scrollbar (default on) before you call Create.
  void ScrollbarOn() {this->SetScrollbarFlag(1);}
  void ScrollbarOff() {this->SetScrollbarFlag(0);}

  // Description:
  // Get the listbox widget.
  vtkGetObjectMacro(Listbox, vtkKWWidget);
  
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
  
  vtkKWWidget *Scrollbar;
  vtkKWWidget *Listbox;
  int ScrollbarFlag;

  void SetScrollbarFlag(int v);
  
private:
  vtkKWListBox(const vtkKWListBox&); // Not implemented
  void operator=(const vtkKWListBox&); // Not implemented
};


#endif



