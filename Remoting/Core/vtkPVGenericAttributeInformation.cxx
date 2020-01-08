/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericAttributeInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGenericAttributeInformation.h"

#include "vtkClientServerStream.h"
#include "vtkGenericAttribute.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVGenericAttributeInformation);

//----------------------------------------------------------------------------
vtkPVGenericAttributeInformation::vtkPVGenericAttributeInformation()
{
}

//----------------------------------------------------------------------------
vtkPVGenericAttributeInformation::~vtkPVGenericAttributeInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVGenericAttributeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVGenericAttributeInformation::CopyFromObject(vtkObject* obj)
{
  vtkGenericAttribute* array = vtkGenericAttribute::SafeDownCast(obj);
  if (!array)
  {
    vtkErrorMacro("Cannot downcast to generic attribute.");
  }

  double range[2];
  double* ptr;
  int idx;

  this->SetName(array->GetName());
  this->DataType = array->GetComponentType();
  this->SetNumberOfComponents(array->GetNumberOfComponents());
  ptr = this->Ranges;
  if (this->NumberOfComponents > 1)
  {
    // First store range of vector magnitude.
    array->GetRange(-1, range);
    *ptr++ = range[0];
    *ptr++ = range[1];
  }
  for (idx = 0; idx < this->NumberOfComponents; ++idx)
  {
    array->GetRange(idx, range);
    *ptr++ = range[0];
    *ptr++ = range[1];
  }
}
