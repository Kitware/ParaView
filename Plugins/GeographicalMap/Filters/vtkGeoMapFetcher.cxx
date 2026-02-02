// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeoMapFetcher.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMemoryResourceStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"

#include <curl/curl.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <sstream>

namespace
{
constexpr const char* GoogleStaticBaseURL = "https://maps.googleapis.com/maps/api/staticmap";
constexpr const char* MapQuestStaticBaseURL = "https://www.mapquestapi.com/staticmap/v5/map";
constexpr const char* OSMBaseURL = "https://tile.openstreetmap.org/";

//-----------------------------------------------------------------------------
// Return whether the libcurl in use has SSL/TLS support enabled.
bool HasCurlSSLSupport()
{
  const curl_version_info_data* vi = curl_version_info(CURLVERSION_NOW);
  if (!vi)
  {
    return false;
  }
  return (vi->features & CURL_VERSION_SSL) != 0;
}

//-----------------------------------------------------------------------------
// Ref: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames (Lon./lat. to tile numbers)
long OSMLonToTileX(double lonDeg, int zoom)
{
  return static_cast<long>(std::floor((lonDeg + 180.0) / 360.0 * static_cast<double>(1 << zoom)));
}

//-----------------------------------------------------------------------------
long OSMLatToTileY(double latDeg, int zoom)
{
  // clamp latitude to WebMercator bounds before conversion
  // 85.05112878 is the maximum latitude representable in the Web Mercator
  // projection. It corresponds to arctan(sinh(π)) and ensures that the projected
  // Y coordinate remains finite (y ∈ [−π, π]) for the OSM / XYZ tile system.
  constexpr double maxLat = 85.05112878;
  if (latDeg < -maxLat)
  {
    latDeg = -maxLat;
  }
  else if (latDeg > maxLat)
  {
    latDeg = maxLat;
  }

  double latRadians = vtkMath::RadiansFromDegrees(latDeg);
  return static_cast<long>(
    std::floor((1.0 - std::log(std::tan(latRadians) + 1.0 / std::cos(latRadians)) / vtkMath::Pi()) *
      0.5 * static_cast<double>(1 << zoom)));
}

//-----------------------------------------------------------------------------
// Ref: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames (Tile numbers to lon./lat.)
double OSMTileXToLon(long tx, int zoom)
{
  return (static_cast<double>(tx) / static_cast<double>(1 << zoom)) * 360.0 - 180.0;
}

//-----------------------------------------------------------------------------
double OSMTileYToLat(long ty, int zoom)
{
  double normalizedMercatorY =
    1.0 - 2.0 * (static_cast<double>(ty) / static_cast<double>(1 << zoom));
  return vtkMath::DegreesFromRadians(std::atan(std::sinh(vtkMath::Pi() * normalizedMercatorY)));
}

//-----------------------------------------------------------------------------
// Compute OSM tile range and output size for a geographic bbox. Determines zoom (from bbox size and
// target dimensions if possible), then converts lat/lon bbox into inclusive OSM tile index range
// and returns final image size in pixels.
void ComputeOSMTileRangeFromBBox(const double bbox[4], const int dimension[2], int requestedZoom,
  long& xMin, long& xMax, long& yTop, long& yBottom, int& outZoom, int& outWidth, int& outHeight)
{
  // Derive zoom similar to previous logic
  outZoom = requestedZoom;
  const double centerLat = (bbox[0] + bbox[1]) * 0.5;
  const double centerLon = (bbox[2] + bbox[3]) * 0.5;
  const double deltaLat = std::abs(2.0 * (bbox[1] - centerLat));
  const double deltaLon = std::abs(2.0 * (bbox[3] - centerLon));

  if (!(deltaLat == 0.0 && deltaLon == 0.0))
  {
    if (deltaLat > deltaLon)
    {
      outZoom =
        static_cast<int>(std::round(std::log2(static_cast<double>(dimension[0]) / deltaLat)));
    }
    else
    {
      outZoom =
        static_cast<int>(std::round(std::log2(static_cast<double>(dimension[1]) / deltaLon)));
    }
  }

  const double latMin = std::min(bbox[0], bbox[1]);
  const double latMax = std::max(bbox[0], bbox[1]);
  const double lonMin = std::min(bbox[2], bbox[3]);
  const double lonMax = std::max(bbox[2], bbox[3]);

  xMin = OSMLonToTileX(lonMin, outZoom);
  xMax = OSMLonToTileX(lonMax, outZoom);
  yTop = OSMLatToTileY(latMax, outZoom);
  yBottom = OSMLatToTileY(latMin, outZoom);

  const long tilesX = xMax - xMin + 1;
  const long tilesY = yBottom - yTop + 1;
  outWidth = static_cast<int>(tilesX * 256);
  outHeight = static_cast<int>(tilesY * 256);
}
} // end namespace

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
  int width = static_cast<int>(this->Dimension[0]);
  int height = static_cast<int>(this->Dimension[1]);
  if (this->Provider == OpenStreetMap)
  {
    scale = 1; // ignore Upscale for OSM
    if (this->FetchingMethod == BoundingBox)
    {
      // Compute OSM tile mosaic size from bounding box
      long xMin = 0, xMax = 0, yTop = 0, yBottom = 0;
      int outZoom = this->ZoomLevel;
      int outW = 256, outH = 256;
      int dim[2] = { static_cast<int>(this->Dimension[0]), static_cast<int>(this->Dimension[1]) };
      double bbox[4] = { this->MapBoundingBox[0], this->MapBoundingBox[1], this->MapBoundingBox[2],
        this->MapBoundingBox[3] };
      ::ComputeOSMTileRangeFromBBox(
        bbox, dim, this->ZoomLevel, xMin, xMax, yTop, yBottom, outZoom, outW, outH);
      width = outW;
      height = outH;
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
  curl_easy_setopt(
    curl_handle, CURLOPT_USERAGENT, "ParaView-GeoMapFetcher (+https://www.paraview.org)");
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
// Overlay the copyright attribution text in the bottom-right corner of the downloaded OSM map
// image. Ref: https://operations.osmfoundation.org/policies/tiles/ (Attribution and licensing)
void vtkGeoMapFetcher::OverlayOSMAttribution(vtkImageData* img)
{
  if (this->Provider != OpenStreetMap || img == nullptr)
  {
    return;
  }

  int* dims = img->GetDimensions();
  vtkDataArray* scalars = img->GetPointData()->GetScalars();
  int comps = scalars->GetNumberOfComponents();
  vtkTextRenderer* tr = vtkTextRenderer::GetInstance();
  vtkNew<vtkTextProperty> tprop;

  tprop->SetFontSize(12);
  tprop->SetColor(1.0, 1.0, 1.0);
  tprop->ShadowOn();
  tprop->SetShadowOffset(1, 1);

  std::string text = "\xC2\xA9 OpenStreetMap contributors"; // UTF-8 ©

  int textDims[2] = { 0, 0 };
  vtkNew<vtkImageData> textImg;
  if (!tr->RenderString(tprop, text, textImg.GetPointer(), textDims, 96))
  {
    return;
  }

  int textExt[6];
  textImg->GetExtent(textExt); // may be non-zero-based due to shadow/offset
  int xmin = textExt[0], xmax = textExt[1];
  int ymin = textExt[2], ymax = textExt[3];
  int tw = xmax - xmin + 1;
  int th = ymax - ymin + 1;

  int margin = 4;
  int dstX0 = std::max(0, dims[0] - tw - margin); // right
  int dstY0 = std::max(0, margin);                // bottom (y=0 at bottom)

  // textImg is RGBA
  vtkDataArray* tscalars = textImg->GetPointData()->GetScalars();
  if (!tscalars || tscalars->GetNumberOfComponents() < 4)
  {
    return;
  }

  for (int y = ymin; y <= ymax; ++y)
  {
    int dy = dstY0 + (y - ymin);
    if (dy < 0 || dy >= dims[1])
    {
      continue;
    }

    for (int x = xmin; x <= xmax; ++x)
    {
      int dx = dstX0 + (x - xmin);
      if (dx < 0 || dx >= dims[0])
      {
        continue;
      }

      unsigned char* src = static_cast<unsigned char*>(textImg->GetScalarPointer(x, y, 0));
      unsigned char* dst = static_cast<unsigned char*>(img->GetScalarPointer(dx, dy, 0));
      // src RGBA
      double a = src[3] / 255.0;
      if (a <= 0.0)
      {
        continue;
      }
      // simple over blend onto RGB or RGBA
      for (int c = 0; c < 3 && c < comps; ++c)
      {
        dst[c] = static_cast<unsigned char>(src[c] * a + dst[c] * (1.0 - a));
      }
    }
  }
}

//-----------------------------------------------------------------------------
int vtkGeoMapFetcher::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // API key is required for GoogleMap and MapQuest, but not for OSM
  if (this->Provider != OpenStreetMap && this->APIKey.empty())
  {
    vtkErrorMacro("An API key must be provided.");
    return 0;
  }

  // If OpenStreetMap is selected, ensure HTTPS capability via libcurl SSL support.
  // Warn once and abort early to avoid spamming per-tile messages.
  if (this->Provider == OpenStreetMap && !::HasCurlSSLSupport())
  {
    vtkWarningMacro(
      "OpenStreetMap requires HTTPS, but libcurl on this build lacks SSL/TLS support. "
      "Please rebuild curl with SSL enabled or select another provider.");
    return 0;
  }

  // get the data object
  vtkImageData* output = vtkImageData::GetData(outputVector);

  // build url
  std::stringstream url;

  int currentZoomLevel = this->ZoomLevel;
  double currentCenter[2] = { this->Center[0], this->Center[1] };
  if (this->FetchingMethod == BoundingBox)
  {
    // If we use the bounding box we'll use it to approximate the center and the needed
    // zoom level. This allows us to stay coherent with image fetched using the `Zoom and
    // center` method and when fetching images with different providers. Also behaviour
    // when using both BoundingBox and Size parameters of the MapQuest API seems to gives
    // uncoherent images.
    // With this method, we ensure that BB of the result image >= BB of the input mesh.

    currentCenter[0] = (this->MapBoundingBox[0] + this->MapBoundingBox[1]) * 0.5;
    currentCenter[1] = (this->MapBoundingBox[2] + this->MapBoundingBox[3]) * 0.5;
    double deltaLat = std::abs(2.0 * (this->MapBoundingBox[1] - currentCenter[0]));
    double deltaLng = std::abs(2.0 * (this->MapBoundingBox[3] - currentCenter[1]));

    if (deltaLat == 0.0 && deltaLng == 0.0)
    {
      vtkWarningMacro("Input bounding box is empty, aborting.");
      return 0;
    }

    if (deltaLat > deltaLng)
    {
      currentZoomLevel = static_cast<int>(std::round(std::log2(this->Dimension[0] / deltaLat)));
    }
    else
    {
      currentZoomLevel = static_cast<int>(std::round(std::log2(this->Dimension[1] / deltaLng)));
    }
  }

  bool useComputedGeoref = false;
  bool mosaicBuilt = false;
  double finalOrigin[2] = { 0.0, 0.0 };  // lon, lat
  double finalSpacing[2] = { 0.0, 0.0 }; // dLon, dLat per pixel

  if (this->Provider == OpenStreetMap && this->FetchingMethod == BoundingBox)
  {
    // OSM mosaic for BoundingBox: compute tile range and assemble via helper
    long xMin = 0, xMax = 0, yTop = 0, yBottom = 0;
    int outZoom = currentZoomLevel;
    int mosaicW = 256, mosaicH = 256;
    int dim[2] = { static_cast<int>(this->Dimension[0]), static_cast<int>(this->Dimension[1]) };
    double bbox[4] = { this->MapBoundingBox[0], this->MapBoundingBox[1], this->MapBoundingBox[2],
      this->MapBoundingBox[3] };
    ::ComputeOSMTileRangeFromBBox(
      bbox, dim, currentZoomLevel, xMin, xMax, yTop, yBottom, outZoom, mosaicW, mosaicH);

    const int comps = 3;
    output->SetDimensions(mosaicW, mosaicH, 1);
    output->AllocateScalars(VTK_UNSIGNED_CHAR, comps);
    if (output->GetPointData()->GetScalars())
    {
      output->GetPointData()->GetScalars()->SetName("PNGImage");
    }

    // Initialize the output image by setting all pixel values (all components) to zero before
    // filling tiles.
    {
      vtkIdType numTuples = static_cast<vtkIdType>(mosaicW) * static_cast<vtkIdType>(mosaicH);
      unsigned char* outPtrAll = static_cast<unsigned char*>(output->GetScalarPointer(0, 0, 0));
      std::memset(outPtrAll, 0, static_cast<size_t>(numTuples) * comps);
    }

    // Fetch each OSM tile, copy its pixels into the correct position of final mosaic image.
    for (long ty = yTop; ty <= yBottom; ++ty)
    {
      for (long tx = xMin; tx <= xMax; ++tx)
      {
        vtkNew<vtkImageData> tmp;
        std::stringstream tu;
        tu << OSMBaseURL << currentZoomLevel << "/" << tx << "/" << ty << ".png";
        if (!this->FetchVtkPNG(tu.str(), tmp))
        {
          vtkWarningMacro("Missing OSM tile at " << tu.str());
          continue;
        }

        for (int py = 0; py < 256; ++py)
        {
          // Compute the destination Y index in the output mosaic image.
          // We invert the Y direction so that northern tiles (smaller ty) appear at higher Y
          // positions in VTK, because in VTK the Y axis increases upward (bottom to top).
          int dstY = static_cast<int>((yBottom - ty) * 256 + py);

          // Get a pointer to the beginning of the destination row in the output image.
          // (tx - xMin) * 256 gives the X offset of this tile in the mosaic (in pixels),
          // dstY is the Y offset computed above.
          unsigned char* dst = static_cast<unsigned char*>(
            output->GetScalarPointer(static_cast<int>((tx - xMin) * 256), dstY, 0));

          // Get a pointer to the beginning of the source row in the current tile image.
          // py is the current row within the 256×256 tile, starting from the top of the tile.
          unsigned char* src = static_cast<unsigned char*>(tmp->GetScalarPointer(0, py, 0));
          std::memcpy(dst, src, static_cast<size_t>(256 * comps));
        }
      }
    }

    // Georeference mosaic using exact OSM tile formulas for bounds
    // Left/Right are tile columns xMin .. xMax (inclusive). Right bound is xMax+1 tile edge.
    const double lonLeft = OSMTileXToLon(xMin, outZoom);
    const double lonRight = OSMTileXToLon(xMax + 1, outZoom);
    // Top/Bottom are tile rows yTop .. yBottom (inclusive). Bottom bound is yBottom+1 tile edge.
    const double latTop = OSMTileYToLat(yTop, outZoom);
    const double latBottom = OSMTileYToLat(yBottom + 1, outZoom);

    // Compute spacing and origin at bottom-left to match VTK j-axis upward.
    finalSpacing[0] = (lonRight - lonLeft) / static_cast<double>(mosaicW); // dLon per pixel
    finalSpacing[1] = (latTop - latBottom) / static_cast<double>(mosaicH); // dLat per pixel
    finalOrigin[0] = lonLeft;                                              // lon
    finalOrigin[1] = latBottom;                                            // lat (bottom)
    useComputedGeoref = true;
    mosaicBuilt = true;
  }
  else if (this->Provider == OpenStreetMap && this->FetchingMethod == ZoomCenter)
  {
    // Compute tile x/y for OSM
    const double lat = currentCenter[0];
    const double lon = currentCenter[1];
    const long tileX = OSMLonToTileX(lon, currentZoomLevel);
    const long tileY = OSMLatToTileY(lat, currentZoomLevel);

    url << OSMBaseURL << currentZoomLevel << "/" << tileX << "/" << tileY;
    url << ".png";

    // Convert tile x,y coordinates into latitude and longitude using helpers
    const double lonLeft = OSMTileXToLon(tileX, currentZoomLevel);
    const double lonRight = OSMTileXToLon(tileX + 1, currentZoomLevel);
    const double latTop = OSMTileYToLat(tileY, currentZoomLevel);
    const double latBottom = OSMTileYToLat(tileY + 1, currentZoomLevel);

    finalSpacing[0] = (lonRight - lonLeft) / 256.0; // dLon per pixel
    finalSpacing[1] = (latTop - latBottom) / 256.0; // dLat per pixel
    finalOrigin[0] = lonLeft;                       // origin lon
    finalOrigin[1] = latBottom;                     // origin lat (bottom)
    useComputedGeoref = true;
  }
  else if (this->Provider == GoogleMap)
  {
    url << GoogleStaticBaseURL << "?format=png&key=" << this->APIKey;
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
    url << "&center=" << currentCenter[0] << "," << currentCenter[1];
    url << "&zoom=" << currentZoomLevel;
  }
  else if (this->Provider == MapQuest)
  {
    url << MapQuestStaticBaseURL << "?format=PNG&margin=0&key=" << this->APIKey;
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
    url << "&center=" << currentCenter[0] << "," << currentCenter[1];
    url << "&zoom=" << currentZoomLevel;
  }

  if (!mosaicBuilt && !this->FetchVtkPNG(url.str(), output))
  {
    vtkErrorMacro(<< "Couldn't fetch geographical image.");
    return 0;
  }

  if (!useComputedGeoref)
  {
    int* dims = output->GetDimensions();
    double corner[2];
    vtkGeoMapFetcher::LatLngToPoint(currentCenter[0], currentCenter[1], corner[0], corner[1]);

    // shift to top-left corner in mercator space
    int scale = (1 << (currentZoomLevel + static_cast<int>(this->Upscale)));
    corner[0] -= 0.5 * dims[0] / scale;
    corner[1] -= 0.5 * dims[1] / scale;

    vtkGeoMapFetcher::PointToLatLng(corner[0], corner[1], corner[0], corner[1]);

    // spacing in lat/lon degrees per pixel
    finalSpacing[1] = 2.0 * (currentCenter[0] - corner[0]) / dims[1]; // dLat
    finalSpacing[0] = 2.0 * (currentCenter[1] - corner[1]) / dims[0]; // dLon
    finalOrigin[0] = corner[1];                                       // lon at left
    finalOrigin[1] = corner[0];                                       // lat at bottom
  }

  output->SetSpacing(finalSpacing[0], finalSpacing[1], 1.0);
  output->SetOrigin(finalOrigin[0], finalOrigin[1], 0.0);
  output->GetPointData()->SetActiveScalars("PNGImage");

  // Overlay attribution only for OSM
  if (this->Provider == OpenStreetMap)
  {
    this->OverlayOSMAttribution(output);
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

  if (buffer.size() < 8 || strncmp(pngMagic, buffer.data(), 8) != 0)
  {
    vtkErrorMacro("Download failed or not a PNG. Size=" << buffer.size());
    return false;
  }

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(buffer.data(), buffer.size());
  // convert to image
  vtkNew<vtkPNGReader> pngReader;
  pngReader->SetStream(stream);
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
