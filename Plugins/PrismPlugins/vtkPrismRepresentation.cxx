/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrismRepresentation.h"

#include "vtkDoubleArray.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPrismView.h"
#include "vtkPVGeometryFilter.h"

vtkStandardNewMacro(vtkPrismRepresentation);

//----------------------------------------------------------------------------
vtkPrismRepresentation::vtkPrismRepresentation()
{

}

//----------------------------------------------------------------------------
vtkPrismRepresentation::~vtkPrismRepresentation()
{

}

//----------------------------------------------------------------------------
bool vtkPrismRepresentation::GenerateMetaData(vtkInformation *inInfo, vtkInformation* outInfo)
{
  bool ret_val = this->Superclass::GenerateMetaData(inInfo, outInfo);

  if (!ret_val ||
    this->GeometryFilter->GetTotalNumberOfInputConnections() == 0)
    {
    return false;
    }

  //we need to verify the input object has the correct field data "key"
  //Only those with the key are valid items to be used to determine
  //the prism world bound size and resulting scale in vtkPrismView.
  vtkDataObject* input = this->GeometryFilter->GetOutputDataObject(0);
  if (!input->GetFieldData()->HasArray("PRISM_GEOMETRY_BOUNDS"))
    {
    //object doesn't have the key, no need to do anything else
    return true;
    }
  
  vtkDoubleArray *b = vtkDoubleArray::SafeDownCast(
      input->GetFieldData()->GetArray("PRISM_GEOMETRY_BOUNDS"));
  double *bounds = b->GetPointer(0);
  if (vtkMath::AreBoundsInitialized(bounds))
    {
    outInfo->Set(vtkPrismView::PRISM_GEOMETRY_BOUNDS(), bounds, 6);
    }
  
  b = vtkDoubleArray::SafeDownCast(
      input->GetFieldData()->GetArray("PRISM_THRESHOLD_BOUNDS"));
  if ( !b )
    {
    b = vtkDoubleArray::SafeDownCast(
      input->GetFieldData()->GetArray("PRISM_GEOMETRY_BOUNDS"));
    }
  bounds = b->GetPointer(0);
  if (vtkMath::AreBoundsInitialized(bounds))
    {
    outInfo->Set(vtkPrismView::PRISM_THRESHOLD_BOUNDS(), bounds, 6);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPrismRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
