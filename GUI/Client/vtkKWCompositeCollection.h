/*=========================================================================

  Module:    vtkKWCompositeCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCompositeCollection - a collection of composites
// .SECTION Description
// vtkKWCompositeCollection represents and provides methods to manipulate a 
// list of composites (i.e., vtkKWComposite and subclasses). The list is 
// unsorted and duplicate entries are not prevented.


#ifndef __vtkKWCompositeC_h
#define __vtkKWCompositeC_h

#include "vtkCollection.h"

class vtkKWComposite;

class VTK_EXPORT vtkKWCompositeCollection : public vtkCollection
{
public:
  static vtkKWCompositeCollection *New();
  vtkTypeRevisionMacro(vtkKWCompositeCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an KWComposite to the list.
  void AddItem(vtkKWComposite *a);

  // Description:
  // Remove an KWComposite from the list.
  void RemoveItem(vtkKWComposite *a);

  // Description:
  // Determine whether a particular KWComposite is present. 
  // Returns its position in the list.
  int IsItemPresent(vtkKWComposite *a);

  // Description:
  // Get the next KWComposite in the list.
  vtkKWComposite *GetNextKWComposite();

  // Description:
  // Get the last KWComposite in the list.
  vtkKWComposite *GetLastKWComposite();

protected:
  vtkKWCompositeCollection() {};
  ~vtkKWCompositeCollection() {};

private:
  vtkKWCompositeCollection(const vtkKWCompositeCollection&); // Not implemented
  void operator=(const vtkKWCompositeCollection&); // Not Implemented
};

#endif






