/*=========================================================================

  Program:   ParaView
  Module:    vtkGeoMapFetcher.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeoMapFetcherMesh.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

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

  const int scale = (this->Fetcher->GetUpscale() ? 2 : 1);
  int ext[6] = { 0, static_cast<int>(scale * this->Fetcher->GetDimension()[0] - 1), 0,
    static_cast<int>(scale * this->Fetcher->GetDimension()[1] - 1), 0, 0 };
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
