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
#include "vtkStructuredGrid.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGeoMapConvertFilter);

//------------------------------------------------------------------------------
int vtkGeoMapConvertFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
//------------------------------------------------------------------------------
void vtkGeoMapConvertFilter::SetPROJ4String(const char* projString)
{
  if (projString)
  {
    this->PROJ4String = projString;
  }
  else
  {
    this->PROJ4String.clear();
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkGeoMapConvertFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVectors, vtkInformationVector* outputVector)
{
  vtkImageData* input = vtkImageData::GetData(inputVectors[0]);
  vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);

  vtkNew<vtkImageToStructuredGrid> image2points;
  image2points->SetInputData(input);
  image2points->Update();

  output->ShallowCopy(image2points->GetOutput());
  vtkPoints* points = output->GetPoints();

  // see https://proj.org/usage/projections.html
  std::string proj4;

  vtkNew<vtkGeoProjection> proj;
  if (this->Projection == Orthographic)
  {
    proj4 = "+proj=ortho +ellps=GRS80";
  }
  else if (this->Projection == Lambert93)
  {
    proj4 =
      "+proj=lcc +ellps=GRS80 +lon_0=3 +lat_0=46.5 +lat_1=44 +lat_2=49 +x_0=700000 +y_0=6600000";
  }
  else
  {
    proj4 = this->PROJ4String;
  }

  proj->SetPROJ4String(proj4.c_str());

  vtkNew<vtkGeoTransform> transform;

  // source is implicit (lat/lon)
  transform->SetDestinationProjection(proj);

  transform->TransformPoints(points, points);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkGeoMapConvertFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Projection: " << this->Projection << "\n";
  os << "PROJ4String: " << this->PROJ4String << "\n";
}
