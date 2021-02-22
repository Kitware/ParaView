/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentedDataInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRepresentedDataInformation.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"

vtkStandardNewMacro(vtkPVRepresentedDataInformation);
//----------------------------------------------------------------------------
vtkPVRepresentedDataInformation::vtkPVRepresentedDataInformation() = default;

//----------------------------------------------------------------------------
vtkPVRepresentedDataInformation::~vtkPVRepresentedDataInformation() = default;

//----------------------------------------------------------------------------
void vtkPVRepresentedDataInformation::CopyFromObject(vtkObject* object)
{
  vtkPVDataRepresentation* repr = vtkPVDataRepresentation::SafeDownCast(object);
  if (repr)
  {
    vtkDataObject* dobj = repr->GetRenderedDataObject(0);
    if (dobj)
    {
      this->Superclass::CopyFromObject(dobj);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentedDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
