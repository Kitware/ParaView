/*=========================================================================

  Module:    vtkKWWidgetCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidgetCollection.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWWidgetCollection );
vtkCxxRevisionMacro(vtkKWWidgetCollection, "1.9");

vtkKWWidget *vtkKWWidgetCollection::GetNextKWWidget() 
{ 
  return vtkKWWidget::SafeDownCast(this->GetNextItemAsObject());
}

vtkKWWidget *vtkKWWidgetCollection::GetLastKWWidget() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkKWWidget::SafeDownCast(this->Bottom->Item);
    }
}

void vtkKWWidgetCollection::AddItem(vtkKWWidget *a) 
{
  this->Superclass::AddItem(static_cast<vtkObject *>(a));
}

void vtkKWWidgetCollection::RemoveItem(vtkKWWidget *a) 
{
  this->Superclass::RemoveItem(static_cast<vtkObject *>(a));
}

int vtkKWWidgetCollection::IsItemPresent(vtkKWWidget *a) 
{
  return this->Superclass::IsItemPresent(static_cast<vtkObject *>(a));
}

//----------------------------------------------------------------------------
void vtkKWWidgetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

