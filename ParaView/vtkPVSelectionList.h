/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSelectionList.h
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
// .NAME vtkPVSelectionList - Another form of radio buttons.
// .SECTION Description
// This widget produces a button that displays a selection from a list.
// When the button is pressed, the list is displayed in the form of a menu.
// The user can select a new value from the menu.

#ifndef __vtkPVSelectionList_h
#define __vtkPVSelectionList_h

#include "vtkKWWidget.h"


class VTK_EXPORT vtkPVSelectionList : public vtkKWWidget
{
public:
  static vtkPVSelectionList* New();
  vtkTypeMacro(vtkPVSelectionList, vtkKWWidget);
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  int Create(vtkKWApplication *app);

  // Add items to the possible selection.
  // The string name is displayed in the list, and the integer value
  // is used to set and get the current selection programmatically.
  void AddItem(char *name, int value);
  void SetValue(int value);
  int GetValue();
  
  // Description:
  // A label is placed in from of the menu button.
  void SetLabel(char *label);
  char *GetLabel() {return this->Label;}
  
protected:
  vtkPVSelectionList();
  ~vtkPVSelectionList();
  vtkPVSelectionList(const vtkPVSelectionList&) {};
  void operator=(const vtkPVSelectionList&) {};
  
  int Value;
  char *Label;
};

#endif
