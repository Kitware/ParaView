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
#include "vtkPVSource.h"

vtkStandardNewMacro(vtkPVExtentWidgetProperty);
vtkCxxRevisionMacro(vtkPVExtentWidgetProperty, "1.4");

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

void vtkPVExtentWidgetProperty::SetAnimationTimeInBatch(
  ofstream *file, float val)
{
  if (this->Widget->GetPVSource())
    {
    vtkPVExtentEntry *entry = vtkPVExtentEntry::SafeDownCast(this->Widget);

    int axis = entry->GetAnimationAxis();

    if (entry)
      {
      *file << "[$pvTemp" << entry->GetPVSource()->GetVTKSourceID(0)
            << " GetProperty " << entry->GetVariableName()
            << "] SetElement " << 2*axis << " [expr round("
            << val << ")]" << endl;
      *file << "[$pvTemp" << entry->GetPVSource()->GetVTKSourceID(0)
            << " GetProperty " << entry->GetVariableName()
            << "] SetElement " << 2*axis+1 << " [expr round("
            << val << ")]" << endl;
      }
    *file << "$pvTemp" << this->Widget->GetPVSource()->GetVTKSourceID(0)
          << " UpdateVTKObjects" << endl;
    }
}

void vtkPVExtentWidgetProperty::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
