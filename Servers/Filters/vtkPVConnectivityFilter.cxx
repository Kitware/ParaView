/*=========================================================================

  Program:   ParaView
  Module:    vtkPVConnectivityFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConnectivityFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkPVConnectivityFilter, "1.6");
vtkStandardNewMacro(vtkPVConnectivityFilter);

vtkPVConnectivityFilter::vtkPVConnectivityFilter()
{
  this->ExtractionMode = VTK_EXTRACT_ALL_REGIONS;
  this->ColorRegions = 1;
}

int vtkPVConnectivityFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int retVal =
    this->Superclass::RequestData(request, inputVector, outputVector);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (this->ColorRegions)
    {
    vtkDataArray* scalars = output->GetPointData()->GetScalars();
    if (scalars)
      {
      scalars->SetName("RegionId");
      }
    }
  return retVal;
}

void vtkPVConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
