/*=========================================================================

  Program:   ParaView
  Module:    vtkGeoMapConvertFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGeoMapConvertFilter
 *
 * Filter used to convert geo data from a Lat/Lon projection to any other projection
 * supported by PROJ.4
 * Two preconfigured projections are provided (Orthographic and Lambert93)
 * The input is an image and the output is a structured grid.
 *
 */

#ifndef vtkGeoMapConvertFilter_h
#define vtkGeoMapConvertFilter_h

#include "vtkGeographicalMapModule.h"
#include "vtkStructuredGridAlgorithm.h"

#include <array>
#include <string>

class VTKGEOGRAPHICALMAP_EXPORT vtkGeoMapConvertFilter : public vtkStructuredGridAlgorithm
{
public:
  static vtkGeoMapConvertFilter* New();
  vtkTypeMacro(vtkGeoMapConvertFilter, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ProjectionType
  {
    Orthographic = 0,
    Lambert93,
    LatLong,

    // For devs : let Custom be the last in the enum to keep coherency in the implementation
    Custom
  };

  //@{
  /**
   * Set/Get the destination projection.
   */
  vtkSetClampMacro(DestProjection, int, 0, Custom);
  vtkGetMacro(DestProjection, int);
  //@}

  //@{
  /**
   * Get/Set the custom PROJ.4 destination projection string.
   */
  vtkGetMacro(CustomDestProjection, std::string);
  vtkSetMacro(CustomDestProjection, std::string);
  //@}

  //@{
  /**
   * Get/Set the source projection.
   */
  vtkSetClampMacro(SourceProjection, int, 0, Custom);
  vtkGetMacro(SourceProjection, int);
  //@}

  //@{
  /**
   * Set the custom PROJ.4 source projection string.
   */
  vtkGetMacro(CustomSourceProjection, std::string);
  vtkSetMacro(CustomSourceProjection, std::string);
  //@}

protected:
  vtkGeoMapConvertFilter() = default;
  ~vtkGeoMapConvertFilter() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int DestProjection = Lambert93;
  std::string CustomDestProjection;

  int SourceProjection = LatLong;
  std::string CustomSourceProjection;

  // clang-format off
  /**
   * Array of PROJ4 projection strings.
   * See enum ProjectionType to see which projection they represent.
   */
  static constexpr std::array<const char*, static_cast<int>(Custom)> PROJ4Strings = {
    "+proj=ortho +ellps=GRS80",
    "+proj=lcc +ellps=GRS80 +lon_0=3 +lat_0=46.5 +lat_1=44 +lat_2=49 +x_0=700000 +y_0=6600000",
    ""
  };
  // clang-format on

private:
  vtkGeoMapConvertFilter(const vtkGeoMapConvertFilter&) = delete;
  void operator=(const vtkGeoMapConvertFilter&) = delete;
};

#endif
