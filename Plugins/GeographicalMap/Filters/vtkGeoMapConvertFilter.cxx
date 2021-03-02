/*=========================================================================

  Program:   ParaView
  Module:    vtkGeoMapConvertFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeoMapConvertFilter.h"

#include "vtkGeoProjection.h"
#include "vtkGeoTransform.h"
#include "vtkImageData.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkStructuredGrid.h"

//-----------------------------------------------------------------------------
constexpr std::array<const char*, static_cast<int>(vtkGeoMapConvertFilter::Custom)>
  vtkGeoMapConvertFilter::PROJ4Strings;
vtkStandardNewMacro(vtkGeoMapConvertFilter);

//------------------------------------------------------------------------------
int vtkGeoMapConvertFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkGeoMapConvertFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVectors, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVectors[0]);
  vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);

  if (vtkImageData* img = vtkImageData::SafeDownCast(input))
  {
    vtkNew<vtkImageToStructuredGrid> image2points;
    image2points->SetInputData(img);
    image2points->Update();
    output->ShallowCopy(image2points->GetOutput());
  }
  else if (vtkRectilinearGrid* grid = vtkRectilinearGrid::SafeDownCast(input))
  {
    vtkNew<vtkRectilinearGridToPointSet> grid2points;
    grid2points->SetInputData(grid);
    grid2points->Update();
    output->ShallowCopy(grid2points->GetOutput());
  }
  else
  {
    output->DeepCopy(input);
  }
  vtkPoints* points = output->GetPoints();

  // see https://proj.org/usage/projections.html
  vtkNew<vtkGeoProjection> destProj;
  if (this->DestProjection == Custom)
  {
    destProj->SetPROJ4String(this->CustomDestProjection.c_str());
  }
  else if (this->DestProjection != LatLong)
  {
    destProj->SetPROJ4String(vtkGeoMapConvertFilter::PROJ4Strings[this->DestProjection]);
  }
  vtkNew<vtkGeoProjection> sourceProj;
  if (this->SourceProjection == Custom)
  {
    sourceProj->SetPROJ4String(this->CustomDestProjection.c_str());
  }
  else if (this->SourceProjection != LatLong)
  {
    sourceProj->SetPROJ4String(vtkGeoMapConvertFilter::PROJ4Strings[this->DestProjection]);
  }

  vtkNew<vtkGeoTransform> transform;
  transform->SetSourceProjection(sourceProj);
  transform->SetDestinationProjection(destProj);
  transform->TransformPoints(points, points);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkGeoMapConvertFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Source Projection: " << this->SourceProjection << "\n";
  os << "Source PROJ4 String: " << this->CustomSourceProjection << "\n";
  os << "Destination Projection: " << this->DestProjection << "\n";
  os << "Destination PROJ4 String: " << this->CustomDestProjection << "\n";
}
