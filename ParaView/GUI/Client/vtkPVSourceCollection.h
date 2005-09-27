/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSourceCollection - a collection of widgets
// .SECTION DescriptionCollection
// vtkPVSourceCollection represents and provides methods to manipulate a list 
// of widgets. The list is unsorted and duplicate entries are not prevented.

#ifndef __vtkPVSourceC_h
#define __vtkPVSourceC_h

#include "vtkCollection.h"
class vtkPVSource;

class VTK_EXPORT vtkPVSourceCollection : public vtkCollection
{
public:
  static vtkPVSourceCollection *New();
  vtkTypeRevisionMacro(vtkPVSourceCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an PVSource to the list.
  void AddItem(vtkPVSource *a);

  // Description:
  // Remove an PVSource from the list.
  void RemoveItem(vtkPVSource *a);

  // Description:
  // Determine whether a particular PVSource is present. 
  // Returns its position in the list.
  int IsItemPresent(vtkPVSource *a);

  // Description:
  // Get the next PVSource in the list.
  vtkPVSource *GetNextPVSource();

  // Description:
  // Get the last PVSource in the list.
  vtkPVSource *GetLastPVSource();

protected:
  vtkPVSourceCollection() {};
  ~vtkPVSourceCollection() {};

private:
  vtkPVSourceCollection(const vtkPVSourceCollection&); // Not implemented
  void operator=(const vtkPVSourceCollection&); // Not implemented
};

#endif





