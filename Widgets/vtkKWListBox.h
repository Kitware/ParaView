/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWListBox.h
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
// .NAME vtkKWListBox
// .SECTION Description
// A widget that can have a list of items with a scroll bar.

#ifndef __vtkKWListBox_h
#define __vtkKWListBox_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWListBox : public vtkKWWidget
{
public:
  static vtkKWListBox* New();
  vtkTypeMacro(vtkKWListBox,vtkKWWidget);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // Get the current selected string in the list
  const char *GetSelection();
  int GetSelectionIndex();
  
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
  // Get number of items in the list.
  int GetNumberOfItems();
  
  // Description: 
  // Get the item at the given index.
  const char* GetItem(int index);
  
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

protected:
  vtkKWListBox();
  ~vtkKWListBox();
  vtkKWListBox(const vtkKWListBox&) {};
  void operator=(const vtkKWListBox&) {};
  char* CurrentSelection;	// store last call of CurrentSelection
  char* Item;			// store last call of GetItem
};


#endif


