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

#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkPVConnectivityFilter, "1.4");
vtkStandardNewMacro(vtkPVConnectivityFilter);

vtkPVConnectivityFilter::vtkPVConnectivityFilter()
{
  this->ExtractionMode = VTK_EXTRACT_ALL_REGIONS;
  this->ColorRegions = 1;
}

void vtkPVConnectivityFilter::Execute()
{
  this->Superclass::Execute();
  if (this->ColorRegions)
    {
    this->GetOutput()->GetPointData()->GetScalars()->SetName("RegionId");
    }
}

void vtkPVConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
