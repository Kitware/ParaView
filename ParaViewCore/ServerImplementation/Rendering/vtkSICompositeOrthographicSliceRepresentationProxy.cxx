/*=========================================================================

  Program:   ParaView
  Module:    vtkSICompositeOrthographicSliceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSICompositeOrthographicSliceRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVCompositeOrthographicSliceRepresentation.h"

vtkStandardNewMacro(vtkSICompositeOrthographicSliceRepresentationProxy);
//----------------------------------------------------------------------------
vtkSICompositeOrthographicSliceRepresentationProxy::vtkSICompositeOrthographicSliceRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSICompositeOrthographicSliceRepresentationProxy::~vtkSICompositeOrthographicSliceRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSICompositeOrthographicSliceRepresentationProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  vtkPVCompositeOrthographicSliceRepresentation* pvrepresentation =
    vtkPVCompositeOrthographicSliceRepresentation::SafeDownCast(this->GetVTKObject());
  pvrepresentation->SetSliceRepresentation(0,
    vtkPVDataRepresentation::SafeDownCast(
      this->GetSubSIProxy("GeometrySliceRepresentationX")->GetVTKObject()));
  pvrepresentation->SetSliceRepresentation(1,
    vtkPVDataRepresentation::SafeDownCast(
      this->GetSubSIProxy("GeometrySliceRepresentationY")->GetVTKObject()));
  pvrepresentation->SetSliceRepresentation(2,
    vtkPVDataRepresentation::SafeDownCast(
      this->GetSubSIProxy("GeometrySliceRepresentationZ")->GetVTKObject()));
  return this->Superclass::ReadXMLAttributes(element);
}

//----------------------------------------------------------------------------
void vtkSICompositeOrthographicSliceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
