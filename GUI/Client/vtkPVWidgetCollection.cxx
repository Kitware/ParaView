/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWidgetCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVWidgetCollection.h"
#include "vtkPVWidget.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkPVWidgetCollection );
vtkCxxRevisionMacro(vtkPVWidgetCollection, "1.4");

vtkPVWidget *vtkPVWidgetCollection::GetNextPVWidget() 
{ 
  return vtkPVWidget::SafeDownCast(this->GetNextItemAsObject());
}

vtkPVWidget *vtkPVWidgetCollection::GetLastPVWidget() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkPVWidget::SafeDownCast(this->Bottom->Item);
    }
}

void vtkPVWidgetCollection::AddItem(vtkPVWidget *a) 
{
  if (a == NULL)
    {
    vtkErrorMacro("NULL Widget.");
    return;
    }
  this->vtkCollection::AddItem(static_cast<vtkObject *>(a));
}

void vtkPVWidgetCollection::RemoveItem(vtkPVWidget *a) 
{
  this->vtkCollection::RemoveItem(static_cast<vtkObject *>(a));
}

int vtkPVWidgetCollection::IsItemPresent(vtkPVWidget *a) 
{
  return this->vtkCollection::IsItemPresent(static_cast<vtkObject *>(a));
}

//----------------------------------------------------------------------------
void vtkPVWidgetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
