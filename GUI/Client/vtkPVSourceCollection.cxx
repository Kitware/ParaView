/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVSourceCollection.h"
#include "vtkPVSource.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkPVSourceCollection );
vtkCxxRevisionMacro(vtkPVSourceCollection, "1.6");

vtkPVSource *vtkPVSourceCollection::GetNextPVSource() 
{ 
  return vtkPVSource::SafeDownCast(this->GetNextItemAsObject());
}

vtkPVSource *vtkPVSourceCollection::GetLastPVSource() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkPVSource::SafeDownCast(this->Bottom->Item);
    }
}

void vtkPVSourceCollection::AddItem(vtkPVSource *a) 
{
  this->vtkCollection::AddItem(static_cast<vtkObject *>(a));
}

void vtkPVSourceCollection::RemoveItem(vtkPVSource *a) 
{
  this->vtkCollection::RemoveItem(static_cast<vtkObject *>(a));
}

int vtkPVSourceCollection::IsItemPresent(vtkPVSource *a) 
{
  return this->vtkCollection::IsItemPresent(static_cast<vtkObject *>(a));
}

//----------------------------------------------------------------------------
void vtkPVSourceCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
