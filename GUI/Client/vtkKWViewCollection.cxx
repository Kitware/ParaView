/*=========================================================================

  Module:    vtkKWViewCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWViewCollection.h"
#include "vtkObjectFactory.h"
#include "vtkKWView.h"

vtkStandardNewMacro( vtkKWViewCollection );
vtkCxxRevisionMacro(vtkKWViewCollection, "1.1");

void vtkKWViewCollection::AddItem(vtkKWView *a) 
{
  this->Superclass::AddItem((vtkObject *)a);
}

void vtkKWViewCollection::RemoveItem(vtkKWView *a) 
{
  this->Superclass::RemoveItem((vtkObject *)a);
}

int vtkKWViewCollection::IsItemPresent(vtkKWView *a) 
{
  return this->Superclass::IsItemPresent((vtkObject *)a);
}

vtkKWView *vtkKWViewCollection::GetNextKWView() 
{ 
  return vtkKWView::SafeDownCast(this->GetNextItemAsObject());
}

vtkKWView *vtkKWViewCollection::GetLastKWView() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkKWView::SafeDownCast(this->Bottom->Item);
    }
}

//----------------------------------------------------------------------------
void vtkKWViewCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

