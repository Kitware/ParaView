/*=========================================================================

  Module:    vtkKWWindowCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWindowCollection - a collection of windows.
// .SECTION Description
// vtkKWWindowCollection represents and provides methods to manipulate a list 
// of windows. The list is unsorted and duplicate entries are not prevented.

// .SECTION See Also
// vtkKWWindow

#ifndef __vtkKWWindowC_h
#define __vtkKWWindowC_h

#include "vtkCollection.h"

class vtkKWWindow;

class VTK_EXPORT vtkKWWindowCollection : public vtkCollection
{
public:
  static vtkKWWindowCollection *New();
  vtkTypeRevisionMacro(vtkKWWindowCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an KWWindow to the list.
  void AddItem(vtkKWWindow *a);

  // Description:
  // Remove an KWWindow from the list.
  void RemoveItem(vtkKWWindow *a);

  // Description:
  // Determine whether a particular KWWindow is present. Returns its position
  // in the list.
  int IsItemPresent(vtkKWWindow *a);

  // Description:
  // Get the next KWWindow in the list.
  vtkKWWindow *GetNextKWWindow();

  // Description:
  // Get the last KWWindow in the list.
  vtkKWWindow *GetLastKWWindow();

protected:
  vtkKWWindowCollection() {};
  ~vtkKWWindowCollection() {};
private:
  vtkKWWindowCollection(const vtkKWWindowCollection&); // Not implemented
  void operator=(const vtkKWWindowCollection&); // Not implemented
};


#endif






