/*=========================================================================

  Module:    vtkKWViewCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWViewCollection - a collection of views
// .SECTION Description
// vtkKWViewCollection represents and provides methods to manipulate a list of
// views. The list is unsorted and duplicate entries are not prevented.

#ifndef __vtkKWViewC_h
#define __vtkKWViewC_h

#include "vtkCollection.h"

class vtkKWView;

class VTK_EXPORT vtkKWViewCollection : public vtkCollection
{
public:
  static vtkKWViewCollection *New();
  vtkTypeRevisionMacro(vtkKWViewCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an KWView to the list.
  void AddItem(vtkKWView *a);

  // Description:
  // Remove an KWView from the list.
  void RemoveItem(vtkKWView *a);

  // Description:
  // Determine whether a particular KWView is present. Returns its position
  // in the list.
  int IsItemPresent(vtkKWView *a);

  // Description:
  // Get the next KWView in the list.
  vtkKWView *GetNextKWView();

  // Description:
  // Get the last KWView in the list.
  vtkKWView *GetLastKWView();

protected:
  vtkKWViewCollection() {};
  ~vtkKWViewCollection() {};

private:
  vtkKWViewCollection(const vtkKWViewCollection&); // Not implemented
  void operator=(const vtkKWViewCollection&); // Not implemented
};


#endif






