/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWViewCollection.h
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
// .NAME vtkKWViewCollection
// .SECTION Description
// A collection class for vtkKWViews.

#ifndef __vtkKWViewC_h
#define __vtkKWViewC_h

#include "vtkCollection.h"
class vtkKWView;

class VTK_EXPORT vtkKWViewCollection : public vtkCollection
{
public:
  static vtkKWViewCollection *New();
  vtkTypeMacro(vtkKWViewCollection,vtkCollection);

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
  vtkKWViewCollection(const vtkKWViewCollection&) {};
  void operator=(const vtkKWViewCollection&) {};

};

inline void vtkKWViewCollection::AddItem(vtkKWView *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkKWViewCollection::RemoveItem(vtkKWView *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkKWViewCollection::IsItemPresent(vtkKWView *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkKWView *vtkKWViewCollection::GetNextKWView() 
{ 
  return (vtkKWView *)(this->GetNextItemAsObject());
}

inline vtkKWView *vtkKWViewCollection::GetLastKWView() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkKWView *)(this->Bottom->Item);
    }
}

#endif





