/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArraySelection.h
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
// .NAME vtkPVArraySelection - widget to select data arrays
// .SECTION Description
// vtkPVArraySelection is used for selecting data arrays to use in filters
// in ParaView.

#ifndef __vtkPVArraySelection_h
#define __vtkPVArraySelection_h

#include "vtkKWOptionMenu.h"
#include "vtkDataSet.h"
#include "vtkKWLabel.h"
#include "vtkPVWidget.h"

class vtkKWRadioButton;

class VTK_EXPORT vtkPVArraySelection : public vtkPVWidget
{
public:
  static vtkPVArraySelection* New();
  vtkTypeMacro(vtkPVArraySelection, vtkPVWidget);
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);
  
  // Description:
  // Set the number of components in the arrays listed in the menu.
  // Defaults to 1.
  vtkSetMacro(NumberOfComponents, int);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // If the vtkPVSource does not have PVInputs (e.g., vtkPVProbe), set
  // the vtkDataSet to get the data arrays from.
  void SetVTKData(vtkDataSet *VTKData);
  vtkGetObjectMacro(VTKData, vtkDataSet);
  
  // Description:
  // Flag to determine whether to use point data or cell data.
  // Defaults to UsePointDataOn.
  void SetUsePointData(int val);
  vtkGetMacro(UsePointData, int);
  vtkBooleanMacro(UsePointData, int);

  // Description:
  // Set the command to call when a menu entry is selected.
  // This command needs to be a method of the PVSource that has been set.
  void SetMenuEntryCommand(const char *methodString);
  
  // Description:
  // Fill the menu with array names
  void FillMenu();
  
  // Description:
  // Get the value of the current menu entry
  char* GetValue() { return this->ArraySelectionMenu->GetValue(); }

  // Description:
  // Set the label for the array menu.
  void SetLabel(const char *label)
    { this->ArraySelectionLabel->SetLabel(label); }
  const char* GetLabel() { return this->ArraySelectionLabel->GetLabel(); }
  
protected:
  vtkPVArraySelection();
  ~vtkPVArraySelection();
  vtkPVArraySelection(const vtkPVArraySelection&) {};
  void operator=(const vtkPVArraySelection&) {};

  int NumberOfComponents;
  int UsePointData;
  
  char *EntryCallback;
  vtkDataSet *VTKData;
  
  vtkSetStringMacro(EntryCallback);
  
  vtkKWWidget *ArraySelectionFrame;
  vtkKWLabel *ArraySelectionLabel;
  vtkKWOptionMenu *ArraySelectionMenu;
};

#endif
