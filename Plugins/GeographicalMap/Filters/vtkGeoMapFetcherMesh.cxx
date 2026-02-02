// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeoMapFetcherMesh.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGeoMapFetcherMesh);

//------------------------------------------------------------------------------
int vtkGeoMapFetcherMesh::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkGeoMapFetcherMesh::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int scale = this->Fetcher->GetUpscale() ? 2 : 1;
  int width = static_cast<int>(this->Fetcher->GetDimension()[0]);
  int height = static_cast<int>(this->Fetcher->GetDimension()[1]);
  if (this->Fetcher->GetProvider() == vtkGeoMapFetcher::OpenStreetMap)
  {
    // For OSM: compute mosaic extent in BoundingBox mode, else single 256x256
    scale = 1;
    if (this->Fetcher->GetFetchingMethod() == vtkGeoMapFetcher::BoundingBox)
    {
      int currentZoomLevel = this->Fetcher->GetZoomLevel();
      const double* bb = this->Fetcher->GetMapBoundingBox();
      double centerLat = (bb[0] + bb[1]) * 0.5;
      double centerLon = (bb[2] + bb[3]) * 0.5;
      double deltaLat = std::abs(2.0 * (bb[1] - centerLat));
      double deltaLon = std::abs(2.0 * (bb[3] - centerLon));
      if (!(deltaLat == 0.0 && deltaLon == 0.0))
      {
        if (deltaLat > deltaLon)
        {
          currentZoomLevel = static_cast<int>(std::round(std::log2(width / deltaLat)));
        }
        else
        {
          currentZoomLevel = static_cast<int>(std::round(std::log2(height / deltaLon)));
        }
      }
      const int tilesPerAxis = (1 << currentZoomLevel);
      auto lat2y = [&](double latDeg)
      {
        double latRadians = vtkMath::RadiansFromDegrees(latDeg);
        return static_cast<int>(std::floor(
          (1.0 - std::log(std::tan(latRadians) + 1.0 / std::cos(latRadians)) / vtkMath::Pi()) *
          0.5 * tilesPerAxis));
      };
      auto lon2x = [&tilesPerAxis](double lonDeg)
      { return static_cast<int>(std::floor((lonDeg + 180.0) / 360.0 * tilesPerAxis)); };

      const double latMin = std::min(bb[0], bb[1]);
      const double latMax = std::max(bb[0], bb[1]);
      const double lonMin = std::min(bb[2], bb[3]);
      const double lonMax = std::max(bb[2], bb[3]);

      int xMin = lon2x(lonMin);
      int xMax = lon2x(lonMax);
      int yTop = lat2y(latMax);
      int yBottom = lat2y(latMin);

      int tilesX = xMax - xMin + 1;
      int tilesY = yBottom - yTop + 1;
      width = tilesX * 256;
      height = tilesY * 256;
    }
    else
    {
      width = 256;
      height = 256;
    }
  }
  int ext[6] = { 0, static_cast<int>(scale * width - 1), 0, static_cast<int>(scale * height - 1), 0,
    0 };
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGeoMapFetcherMesh::RequestData(
  vtkInformation* request, vtkInformationVector** inputVectors, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVectors[0]);

  // If needed project the input mesh to get the bounds in the LatLong projection
  double meshBounds[6];
  if (this->MeshProjection != vtkGeoMapConvertFilter::LatLong)
  {
    vtkNew<vtkGeoMapConvertFilter> converter;
    converter->SetSourceProjection(this->MeshProjection);
    converter->SetCustomSourceProjection(this->CustomMeshProjection);
    converter->SetDestProjection(vtkGeoMapConvertFilter::LatLong);
    converter->SetInputData(input);
    converter->Update();
    converter->GetOutput()->GetBounds(meshBounds);
  }
  else
  {
    input->GetBounds(meshBounds);
  }
  // X:=longitude and Y:=Latitude but vtkGeoMapFetcher expect a Lat/Long order so swap
  double bounds[4] = { meshBounds[2], meshBounds[3], meshBounds[0], meshBounds[1] };
  double center[2] = { (bounds[0] + bounds[1]) * 0.5, (bounds[2] + bounds[3]) * 0.5 };

  // Fetch the image and
  // if needed re project the fetched image into its original projection
  this->Fetcher->SetMapBoundingBox(bounds);
  this->Fetcher->SetCenter(center);
  this->Fetcher->Update();
  vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
  if (this->MeshProjection != vtkGeoMapConvertFilter::LatLong)
  {
    vtkNew<vtkGeoMapConvertFilter> converter;
    converter->SetSourceProjection(vtkGeoMapConvertFilter::LatLong);
    converter->SetDestProjection(this->MeshProjection);
    converter->SetCustomDestProjection(this->CustomMeshProjection);
    converter->SetInputData(this->Fetcher->GetOutput());
    converter->Update();

    output->ShallowCopy(converter->GetOutput());
  }
  else
  {
    vtkNew<vtkImageToStructuredGrid> image2points;
    image2points->SetInputData(this->Fetcher->GetOutput());
    image2points->Update();

    output->ShallowCopy(image2points->GetOutput());
  }

  output->GetPointData()->SetActiveScalars("PNGImage");

  return 1;
}

//-----------------------------------------------------------------------------
void vtkGeoMapFetcherMesh::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MeshProjection: " << this->MeshProjection << "\n";
  os << indent << "CustomMeshProjection: " << this->CustomMeshProjection << "\n";
  os << indent << "Fetcher: " << std::endl;
  this->Fetcher->PrintSelf(os, indent.GetNextIndent());
}
