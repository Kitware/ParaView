/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWidgetCollection.h
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
// .NAME vtkKWWidget
// .SECTION DescriptionCollection
// A simple collection class for holding vtkKWWidgets.

#ifndef __vtkKWWidgetC_h
#define __vtkKWWidgetC_h

#include "vtkCollection.h"
class vtkKWWidget;

class VTK_EXPORT vtkKWWidgetCollection : public vtkCollection
{
 public:
  static vtkKWWidgetCollection *New();
  const char *GetClassName() {return "vtkKWWidgetCollection";};

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
};

inline void vtkKWWidgetCollection::AddItem(vtkKWWidget *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkKWWidgetCollection::RemoveItem(vtkKWWidget *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkKWWidgetCollection::IsItemPresent(vtkKWWidget *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkKWWidget *vtkKWWidgetCollection::GetNextKWWidget() 
{ 
  return (vtkKWWidget *)(this->GetNextItemAsObject());
}

inline vtkKWWidget *vtkKWWidgetCollection::GetLastKWWidget() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkKWWidget *)(this->Bottom->Item);
    }
}

#endif





