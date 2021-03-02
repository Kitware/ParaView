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
#include "vtkGeoMapFetcher.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <curl/curl.h>

#include <cassert>
#include <cmath>
#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGeoMapFetcher);

//-----------------------------------------------------------------------------
vtkGeoMapFetcher::vtkGeoMapFetcher()
{
  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
void vtkGeoMapFetcher::LatLngToPoint(double lat, double lng, double& x, double& y)
{
  const double mercRange = 256;

  x = mercRange * (0.5 + lng / 360.0);

  double siny = vtkMath::ClampValue(std::sin(vtkMath::RadiansFromDegrees(lat)), -0.9999, 0.9999);
  y = mercRange * 0.5 * (1.0 + std::log((1.0 + siny) / (1.0 - siny)) / (2.0 * vtkMath::Pi()));
}

//-----------------------------------------------------------------------------
void vtkGeoMapFetcher::PointToLatLng(double x, double y, double& lat, double& lng)
{
  const double mercRange = 256;

  lng = (x - 0.5 * mercRange) * 360.0 / mercRange;

  double latRadians = (y - 0.5 * mercRange) * (2.0 * vtkMath::Pi()) / mercRange;
  lat = vtkMath::DegreesFromRadians(2.0 * std::atan(std::exp(latRadians)) - vtkMath::Pi() * 0.5);
}

//-----------------------------------------------------------------------------
int vtkGeoMapFetcher::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int scale = (this->Upscale ? 2 : 1);

  int ext[6] = { 0, static_cast<int>(scale * this->Dimension[0] - 1), 0,
    static_cast<int>(scale * this->Dimension[1] - 1), 0, 0 };
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);

  return 1;
}

//-----------------------------------------------------------------------------
size_t vtkGeoMapFetcher::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  assert(size == 1);

  std::vector<char>* mem = reinterpret_cast<std::vector<char>*>(userp);

  char* newContents = reinterpret_cast<char*>(contents);
  mem->insert(mem->end(), newContents, newContents + nmemb);

  return nmemb;
}

//-----------------------------------------------------------------------------
bool vtkGeoMapFetcher::DownloadData(const std::string& url, std::vector<char>& buffer)
{
  CURL* curl_handle;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);

  // init the curl session
  curl_handle = curl_easy_init();

  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, vtkGeoMapFetcher::WriteCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20);

  // download data
  res = curl_easy_perform(curl_handle);

  // check for errors
  if (res != CURLE_OK)
  {
    vtkErrorMacro("curl_easy_perform() failed: " << curl_easy_strerror(res));
    return false;
  }

  // cleanup curl stuff
  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();

  return true;
}

//-----------------------------------------------------------------------------
int vtkGeoMapFetcher::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->APIKey.empty())
  {
    vtkErrorMacro("An API key must be provided.");
    return 0;
  }

  // get the data object
  vtkImageData* output = vtkImageData::GetData(outputVector);

  // build url
  std::stringstream url;

  if (this->Provider == GoogleMap)
  {
    url << "http://maps.googleapis.com/maps/api/staticmap?format=png&key=" << this->APIKey;
    url << "&maptype=";
    switch (this->Type)
    {
      case Road:
        url << "roadmap";
        break;
      case Hybrid:
        url << "hybrid";
        break;
      case Satellite:
        url << "satellite";
        break;
      case Alternative:
        url << "terrain";
        break;
    }
    url << "&size=" << this->Dimension[0] << "x" << this->Dimension[1];
    if (this->Upscale)
    {
      url << "&scale=2";
    }
  }
  else
  {
    url << "http://www.mapquestapi.com/staticmap/v5/map?format=PNG&margin=0&key=" << this->APIKey;
    url << "&type=";
    switch (this->Type)
    {
      case Road:
        url << "map";
        break;
      case Hybrid:
        url << "hyb";
        break;
      case Satellite:
        url << "sat";
        break;
      case Alternative:
        url << "light";
        break;
    }
    url << "&size=" << this->Dimension[0] << "," << this->Dimension[1];
    if (this->Upscale)
    {
      url << "@2x";
    }
  }

  if (this->FetchingMethod == BoundingBox && this->Provider == MapQuest)
  {
    double* bb = this->MapBoundingBox;
    url << "&boundingBox=" << bb[1] << "," << bb[2] << "," << bb[0] << "," << bb[3];
    url << "&margin=0";

    if (!this->FetchVtkPNG(url.str(), output))
    {
      vtkErrorMacro(<< "Couldn't fetch geographical image.");
      return 0;
    }

    int* dims = output->GetDimensions();

    double spacing[2] = { (bb[1] - bb[0]) / dims[1], (bb[3] - bb[2]) / dims[0] };
    output->SetSpacing(spacing[1], spacing[0], 1.0);
    output->SetOrigin(bb[2], bb[0], 0.0);
  }
  else
  {
    int currentZoomLevel = this->ZoomLevel;
    double currentCenter[2] = { this->Center[0], this->Center[1] };
    if (this->FetchingMethod == BoundingBox)
    {
      vtkWarningMacro(<< "Using the Bounding Box option with the GoogleMap API leads to "
                      << "approximation as it is not supported natively by the API.");
      currentCenter[0] = (this->MapBoundingBox[0] + this->MapBoundingBox[1]) * 0.5;
      currentCenter[1] = (this->MapBoundingBox[2] + this->MapBoundingBox[3]) * 0.5;
      currentZoomLevel = static_cast<int>(std::round(
        std::log2(this->Dimension[1] / (2 * (this->MapBoundingBox[3] - currentCenter[1])))));
    }
    url << "&center=" << currentCenter[0] << "," << currentCenter[1];
    url << "&zoom=" << currentZoomLevel;

    if (!this->FetchVtkPNG(url.str(), output))
    {
      vtkErrorMacro(<< "Couldn't fetch geographical image.");
      return 0;
    }

    // Reproject image
    int* dims = output->GetDimensions();
    double origin[2];
    vtkGeoMapFetcher::LatLngToPoint(currentCenter[0], currentCenter[1], origin[0], origin[1]);

    // shift to corner
    int scale = (1 << (currentZoomLevel + static_cast<int>(this->Upscale)));
    origin[0] -= 0.5 * dims[0] / scale;
    origin[1] -= 0.5 * dims[1] / scale;

    vtkGeoMapFetcher::PointToLatLng(origin[0], origin[1], origin[0], origin[1]);

    // swap lat/lon in order to have lon=X and lat=Y
    double spacing[2] = { 2.0 * (currentCenter[0] - origin[0]) / dims[1],
      2.0 * (currentCenter[1] - origin[1]) / dims[0] };

    output->SetSpacing(spacing[1], spacing[0], 1.0);
    output->SetOrigin(origin[1], origin[0], 0.0);
  }

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkGeoMapFetcher::FetchVtkPNG(const std::string& url, vtkImageData* dest)
{
  // download buffer
  std::vector<char> buffer;
  if (!this->DownloadData(url, buffer))
  {
    return 0;
  }

  // check if we got an image
  const char* pngMagic = "\x89PNG\x0D\x0A\x1A\x0A";

  if (strncmp(pngMagic, buffer.data(), 8) != 0)
  {
    vtkErrorMacro("Download failed: " << buffer.data());
    return false;
  }

  // convert to image
  vtkNew<vtkPNGReader> pngReader;
  pngReader->SetMemoryBufferLength(static_cast<vtkIdType>(buffer.size()));
  pngReader->SetMemoryBuffer(buffer.data());
  pngReader->Update();
  dest->ShallowCopy(pngReader->GetOutput());

  int* dims = dest->GetDimensions();
  if (dims[0] < 1 || dims[1] < 1)
  {
    vtkErrorMacro("Error during PNG read.");
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkGeoMapFetcher::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Center: " << this->Center[0] << "," << this->Center[1] << "\n";
  os << "Dimension: " << this->Dimension[0] << "," << this->Dimension[1] << "\n";
  os << "ZoomLevel: " << this->ZoomLevel << "\n";
  os << "APIKey: " << this->APIKey << "\n";
  os << "Provider: " << this->Provider << "\n";
  os << "Type: " << this->Type << "\n";
  os << "Upscale: " << (this->Upscale ? "true" : "false") << "\n";
}
