/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCompositeCollection.h
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

#ifndef __vtkKWCompositeC_h
#define __vtkKWCompositeC_h

#include "vtkCollection.h"
#include "vtkKWComposite.h"

class VTK_EXPORT vtkKWCompositeCollection : public vtkCollection
{
 public:
  static vtkKWCompositeCollection *New();
  const char *GetClassName() {return "vtkKWCompositeCollection";};

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
};

inline void vtkKWCompositeCollection::AddItem(vtkKWComposite *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkKWCompositeCollection::RemoveItem(vtkKWComposite *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkKWCompositeCollection::IsItemPresent(vtkKWComposite *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkKWComposite *vtkKWCompositeCollection::GetNextKWComposite() 
{ 
  return (vtkKWComposite *)(this->GetNextItemAsObject());
}

inline vtkKWComposite *vtkKWCompositeCollection::GetLastKWComposite() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkKWComposite *)(this->Bottom->Item);
    }
}

#endif





