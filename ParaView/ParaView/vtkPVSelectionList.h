/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.h
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
// .NAME vtkPVSelectionList - Another form of radio buttons.
// .SECTION Description
// This widget produces a button that displays a selection from a list.
// When the button is pressed, the list is displayed in the form of a menu.
// The user can select a new value from the menu.

// This might do better as a subclass of vtkPVMenuButton.

#ifndef __vtkPVSelectionList_h
#define __vtkPVSelectionList_h

#include "vtkPVWidget.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"

class vtkStringList;

class VTK_EXPORT vtkPVSelectionList : public vtkPVWidget
{
public:
  static vtkPVSelectionList* New();
  vtkTypeMacro(vtkPVSelectionList, vtkPVWidget);
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  int Create(vtkKWApplication *app);

  // Add items to the possible selection.
  // The string name is displayed in the list, and the integer value
  // is used to set and get the current selection programmatically.
  void AddItem(const char *name, int value);
  
  // Description:
  // Set the label of the menu.
  void SetLabel(const char *label);
  const char *GetLabel() {return this->Label->GetLabel();}

  // Description:
  // Called when accept button is pushed.  Just adds to trace
  // file and calls supperclass accept.
  virtual void Accept();

  // Description:
  // This is how the user can query the state of the selection.
  // Warning:  Setting the current value will not change vtk ivar.
  vtkGetMacro(CurrentValue, int);
  void SetCurrentValue(int val);
  vtkGetStringMacro(CurrentName);
  
  // Description:
  // This method gets called when the user selects an entry.
  // Use this method if you want to programmatically change the selection.
  void SelectCallback(const char *name, int value);
  
protected:
  vtkPVSelectionList();
  ~vtkPVSelectionList();
  vtkPVSelectionList(const vtkPVSelectionList&) {};
  void operator=(const vtkPVSelectionList&) {};

  vtkKWLabel *Label;
  vtkKWMenuButton *MenuButton;
  char *Command;

  int CurrentValue;
  char *CurrentName;
  // Using this list as an array of strings.
  vtkStringList *Names;

  vtkSetStringMacro(CurrentName);
  
};

#endif
