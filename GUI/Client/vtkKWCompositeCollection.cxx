/*=========================================================================

  Module:    vtkKWCompositeCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWCompositeCollection.h"
#include "vtkObjectFactory.h"

#include "vtkKWComposite.h"

vtkStandardNewMacro( vtkKWCompositeCollection );
vtkCxxRevisionMacro(vtkKWCompositeCollection, "1.1");

void vtkKWCompositeCollection::AddItem(vtkKWComposite *a) 
{
  this->Superclass::AddItem((vtkObject *)a);
}

void vtkKWCompositeCollection::RemoveItem(vtkKWComposite *a) 
{
  this->Superclass::RemoveItem((vtkObject *)a);
}

int vtkKWCompositeCollection::IsItemPresent(vtkKWComposite *a) 
{
  return this->Superclass::IsItemPresent((vtkObject *)a);
}

vtkKWComposite *vtkKWCompositeCollection::GetNextKWComposite() 
{ 
  return vtkKWComposite::SafeDownCast(this->GetNextItemAsObject());
}

vtkKWComposite *vtkKWCompositeCollection::GetLastKWComposite() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkKWComposite::SafeDownCast(this->Bottom->Item);
    }
}

//----------------------------------------------------------------------------
void vtkKWCompositeCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

