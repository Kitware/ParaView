/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWindowCollection.h
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
// .NAME vtkKWWindowCollection
// .SECTION Description
// A simple colleciton of vtkKWWindows.

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
  const char *GetClassName() {return "vtkKWWindowCollection";};

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
};

inline void vtkKWWindowCollection::AddItem(vtkKWWindow *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkKWWindowCollection::RemoveItem(vtkKWWindow *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkKWWindowCollection::IsItemPresent(vtkKWWindow *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkKWWindow *vtkKWWindowCollection::GetNextKWWindow() 
{ 
  return (vtkKWWindow *)(this->GetNextItemAsObject());
}

inline vtkKWWindow *vtkKWWindowCollection::GetLastKWWindow() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkKWWindow *)(this->Bottom->Item);
    }
}

#endif





