/*=========================================================================

  Module:    vtkKWWindowCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWindowCollection.h"

#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKWWindowCollection);
vtkCxxRevisionMacro(vtkKWWindowCollection, "1.9");

void vtkKWWindowCollection::AddItem(vtkKWWindow *a) 
{
  this->Superclass::AddItem((vtkObject *)a);
}

void vtkKWWindowCollection::RemoveItem(vtkKWWindow *a) 
{
  this->Superclass::RemoveItem((vtkObject *)a);
}

int vtkKWWindowCollection::IsItemPresent(vtkKWWindow *a) 
{
  return this->Superclass::IsItemPresent((vtkObject *)a);
}

vtkKWWindow *vtkKWWindowCollection::GetNextKWWindow() 
{ 
  return vtkKWWindow::SafeDownCast(this->GetNextItemAsObject());
}

vtkKWWindow *vtkKWWindowCollection::GetLastKWWindow() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkKWWindow::SafeDownCast(this->Bottom->Item);
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

