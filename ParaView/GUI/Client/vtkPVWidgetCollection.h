/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWidgetCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWidgetCollection - a collection of widgets
// .SECTION DescriptionCollection
// vtkPVWidgetCollection represents and provides methods to manipulate a list 
// of widgets. The list is unsorted and duplicate entries are not prevented.

#ifndef __vtkPVWidgetC_h
#define __vtkPVWidgetC_h

#include "vtkCollection.h"
class vtkPVWidget;

class VTK_EXPORT vtkPVWidgetCollection : public vtkCollection
{
public:
  static vtkPVWidgetCollection *New();
  vtkTypeRevisionMacro(vtkPVWidgetCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an PVWidget to the list.
  void AddItem(vtkPVWidget *a);

  // Description:
  // Remove an PVWidget from the list.
  void RemoveItem(vtkPVWidget *a);

  // Description:
  // Determine whether a particular PVWidget is present. 
  // Returns its position in the list.
  int IsItemPresent(vtkPVWidget *a);

  // Description:
  // Get the next PVWidget in the list.
  vtkPVWidget *GetNextPVWidget();

  // Description:
  // Get the last PVWidget in the list.
  vtkPVWidget *GetLastPVWidget();

protected:
  vtkPVWidgetCollection() {};
  ~vtkPVWidgetCollection() {};

private:
  vtkPVWidgetCollection(const vtkPVWidgetCollection&); // Not implemented
  void operator=(const vtkPVWidgetCollection&); // Not implemented
};

#endif





