/*=========================================================================

  Module:    vtkKWWidgetCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWidgetCollection - a collection of widgets
// .SECTION DescriptionCollection
// vtkKWWidgetCollection represents and provides methods to manipulate a list 
// of widgets. The list is unsorted and duplicate entries are not prevented.

#ifndef __vtkKWWidgetC_h
#define __vtkKWWidgetC_h

#include "vtkCollection.h"
class vtkKWWidget;

class VTK_EXPORT vtkKWWidgetCollection : public vtkCollection
{
public:
  static vtkKWWidgetCollection *New();
  vtkTypeRevisionMacro(vtkKWWidgetCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an KWWidget to the list.
  void AddItem(vtkKWWidget *a);

  // Description:
  // Remove an KWWidget from the list.
  void RemoveItem(vtkKWWidget *a);

  // Description:
  // Determine whether a particular KWWidget is present. 
  // Returns its position in the list.
  int IsItemPresent(vtkKWWidget *a);

  // Description:
  // Get the next KWWidget in the list.
  vtkKWWidget *GetNextKWWidget();

  // Description:
  // Get the last KWWidget in the list.
  vtkKWWidget *GetLastKWWidget();

protected:
  vtkKWWidgetCollection() {};
  ~vtkKWWidgetCollection() {};

private:
  vtkKWWidgetCollection(const vtkKWWidgetCollection&); // Not implemented
  void operator=(const vtkKWWidgetCollection&); // Not implemented
};

#endif






