/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtentWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVExtentEntry.h"

vtkStandardNewMacro(vtkPVExtentWidgetProperty);
vtkCxxRevisionMacro(vtkPVExtentWidgetProperty, "1.3");

void vtkPVExtentWidgetProperty::SetAnimationTime(float time)
{
  vtkPVExtentEntry *entry = vtkPVExtentEntry::SafeDownCast(this->Widget);
  if (!entry)
    {
    return;
    }
  
  int axis = entry->GetAnimationAxis();
  
  this->Scalars[2*axis] = this->Scalars[2*axis+1] = time;
  entry->ModifiedCallback();
  entry->Reset();
}

void vtkPVExtentWidgetProperty::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
