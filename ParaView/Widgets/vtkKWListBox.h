/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWListBox.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  virtual void Create(vtkKWApplication *app, const char *args);

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

  // Description:
  // Setting this string enables balloon help for this widget.
  // Override to pass down to children for cleaner behavior
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification( int j );
    
protected:
  vtkKWListBox();
  ~vtkKWListBox();
  char* CurrentSelection;       // store last call of CurrentSelection
  char* Item;                   // store last call of GetItem
  
  vtkKWWidget *Scrollbar;
  vtkKWWidget *Listbox;
  
private:
  vtkKWListBox(const vtkKWListBox&); // Not implemented
  void operator=(const vtkKWListBox&); // Not implemented
};


#endif


