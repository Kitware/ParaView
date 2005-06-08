/*=========================================================================

  Module:    vtkKWRadioButtonSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRadioButtonSet - a concrete set of vtkKWRadioButton
// .SECTION Description
// A composite widget to conveniently store, allocate, create and pack a 
// set of vtkKWRadioButton. 
// Each vtkKWRadioButton is created, removed or queried based
// on a unique ID provided by the user (ids are *not* handled by the class
// since it is likely that they will be defined as enum's or #define by
// the user for easier retrieval).
// As a subclass of vtkKWWidgetSet, it inherits methods to set the widgets
// visibility individually, set the layout parameters, and query each widget.
// Widgets are packed (gridded) in the order they were added.
// .SECTION See Also
// vtkKWWidgetSet

#ifndef __vtkKWRadioButtonSet_h
#define __vtkKWRadioButtonSet_h

#include "vtkKWWidgetSet.h"

class vtkKWRadioButton;

class KWWIDGETS_EXPORT vtkKWRadioButtonSet : public vtkKWWidgetSet
{
public:
  static vtkKWRadioButtonSet* New();
  vtkTypeRevisionMacro(vtkKWRadioButtonSet,vtkKWWidgetSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a vtkKWRadioButton to the set.
  // The id has to be unique among the set.
  // Return a pointer to the vtkKWRadioButton, or NULL on error.
  virtual vtkKWRadioButton* AddWidget(int id);

  // Description:
  // Get a vtkKWRadioButton from the set, given its unique id.
  // Return a pointer to the vtkKWRadioButton, or NULL on error.
  virtual vtkKWRadioButton* GetWidget(int id);

protected:
  vtkKWRadioButtonSet();
  ~vtkKWRadioButtonSet() {};

  // Helper methods

  virtual vtkKWWidget* AllocateAndCreateWidget();

  // BTX
  virtual vtkKWWidget* AddWidgetInternal(int id);
  //ETX

private:
  vtkKWRadioButtonSet(const vtkKWRadioButtonSet&); // Not implemented
  void operator=(const vtkKWRadioButtonSet&); // Not implemented
};

#endif
