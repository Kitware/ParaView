/*=========================================================================

  Program:   ParaView
  Module:    vtkGeoMapFetcher.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGeoMapFetcher
 *
 * Download a map using a provider (GoogleMap or MapQuest) and position it
 * using X=lon and Y=lat (in degrees)
 *
 */

#ifndef vtkGeoMapFetcher_h
#define vtkGeoMapFetcher_h

#include "vtkGeographicalMapModule.h"
#include "vtkImageAlgorithm.h"

#include <string>
#include <vector>

class VTKGEOGRAPHICALMAP_EXPORT vtkGeoMapFetcher : public vtkImageAlgorithm
{
public:
  static vtkGeoMapFetcher* New();
  vtkTypeMacro(vtkGeoMapFetcher, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum MapType
  {
    Road = 0,
    Hybrid = 1,
    Satellite = 2,
    Alternative = 3
  };

  enum APIProvider
  {
    GoogleMap = 0,
    MapQuest = 1
  };

  //@{
  /**
   * Get/Set the center point in latitude/longitude coordinates (in degrees).
   * Default is 0,0.
   */
  vtkSetVector2Macro(Center, double);
  vtkGetVector2Macro(Center, double);
  //@}

  //@{
  /**
   * Get/Set the dimension of the downloaded map.
   * Default is 500x500.
   */
  vtkSetVector2Macro(Dimension, unsigned short);
  vtkGetVector2Macro(Dimension, unsigned short);
  //@}

  //@{
  /**
   * Get/Set the zoom level of the downloaded map.
   * Default is 4.
   */
  vtkSetClampMacro(ZoomLevel, unsigned short, 0, 18);
  vtkGetMacro(ZoomLevel, unsigned short);
  //@}

  //@{
  /**
   * Select the provider service used.
   * Default is MapQuest.
   */
  vtkSetClampMacro(Provider, int, GoogleMap, MapQuest);
  vtkGetMacro(Provider, int);
  //@}

  //@{
  /**
   * Get/Set the type of the map.
   * Default is Satellite.
   */
  vtkSetClampMacro(Type, int, Road, Alternative);
  vtkGetMacro(Type, int);
  //@}

  //@{
  /**
   * Get/Set the upscale flag. If enabled, the image will have twice the requested dimension.
   * Default is true.
   */
  vtkSetMacro(Upscale, bool);
  vtkGetMacro(Upscale, bool);
  //@}

  //@{
  /**
   * Get/Set the API key used for the selected provider.
   */
  vtkSetMacro(APIKey, std::string);
  vtkGetMacro(APIKey, std::string);
  //@}

protected:
  vtkGeoMapFetcher();
  ~vtkGeoMapFetcher() override = default;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Convert Lat/Lon to pixel coordinates.
   * see https://stackoverflow.com/a/12511820/2609654
   */
  static void LatLngToPoint(double lat, double lng, double& x, double& y);

  /**
   * Convert pixel coordinates to Lat/Lon.
   * see https://stackoverflow.com/a/12511820/2609654
   */
  static void PointToLatLng(double x, double y, double& lat, double& lng);

  /**
   * Callback function used by CURL.
   */
  static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

  /**
   * Download data using CURL.
   */
  bool DownloadData(const std::string& url, std::vector<char>& buffer);

  double Center[2] = { 0.0, 0.0 };
  unsigned short Dimension[2] = { 500, 500 };
  unsigned short ZoomLevel = 4;
  std::string APIKey = "";
  int Provider = MapQuest;
  int Type = Satellite;
  bool Upscale = true;

private:
  vtkGeoMapFetcher(const vtkGeoMapFetcher&) = delete;
  void operator=(const vtkGeoMapFetcher&) = delete;
};

#endif
