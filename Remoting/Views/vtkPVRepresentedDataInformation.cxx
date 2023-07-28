// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
