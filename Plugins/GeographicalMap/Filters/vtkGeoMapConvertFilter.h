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

class VTKGEOGRAPHICALMAP_EXPORT vtkGeoMapConvertFilter : public vtkStructuredGridAlgorithm
{
public:
  static vtkGeoMapConvertFilter* New();
  vtkTypeMacro(vtkGeoMapConvertFilter, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ProjectionType
  {
    Custom = 0,
    Orthographic = 1,
    Lambert93 = 2
  };

  //@{
  /**
   * Set/Get the destination projection.
   */
  vtkSetClampMacro(Projection, int, Custom, Lambert93);
  vtkGetMacro(Projection, int);
  //@}

  /**
   * Set the custom PROJ.4 string.
   */
  void SetPROJ4String(const char* projString);

protected:
  vtkGeoMapConvertFilter() = default;
  ~vtkGeoMapConvertFilter() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  std::string PROJ4String;
  int Projection = Lambert93;

private:
  vtkGeoMapConvertFilter(const vtkGeoMapConvertFilter&) = delete;
  void operator=(const vtkGeoMapConvertFilter&) = delete;
};

#endif
