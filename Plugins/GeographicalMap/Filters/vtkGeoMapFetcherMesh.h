/*=========================================================================

  Program:   ParaView
  Module:    vtkGeoMapFetcherMesh.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGeoMapFetcherMesh
 *
 * Download a map using a provider (GoogleMap or MapQuest) and position it
 * using X=lon and Y=lat (in degrees).
 * Instead of vtkGeMapFetcher that relies on properties to get the position
 * of the requested data, vtkGeoMapFetcherMesh use the input geometry to set
 * the bounding box and the center.
 */

#ifndef vtkGeoMapFetcherMesh_h
#define vtkGeoMapFetcherMesh_h

#include "vtkStructuredGridAlgorithm.h"

#include "vtkGeoMapConvertFilter.h"
#include "vtkGeoMapFetcher.h"

class VTKGEOGRAPHICALMAP_EXPORT vtkGeoMapFetcherMesh : public vtkStructuredGridAlgorithm
{
public:
  static vtkGeoMapFetcherMesh* New();
  vtkTypeMacro(vtkGeoMapFetcherMesh, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the mesh projection.
   */
  vtkSetClampMacro(MeshProjection, int, 0, vtkGeoMapConvertFilter::Custom);
  vtkGetMacro(MeshProjection, int);
  //@}

  //@{
  /**
   * Get/Set the custom PROJ.4 mesh projection string.
   */
  vtkGetMacro(CustomMeshProjection, std::string);
  vtkSetMacro(CustomMeshProjection, std::string);
  //@}

  //@{
  /**
   * Forward setter to the internal vtkGeoMapFetcher object.
   */
  inline void SetFetchingMethod(int _arg)
  {
    this->Fetcher->SetFetchingMethod(_arg);
    this->Modified();
  }

  inline void SetZoomLevel(unsigned short _arg)
  {
    this->Fetcher->SetZoomLevel(_arg);
    this->Modified();
  }

  inline void SetDimension(unsigned short _arg1, unsigned short _arg2)
  {
    this->Fetcher->SetDimension(_arg1, _arg2);
    this->Modified();
  }
  inline void SetDimension(unsigned short _arg[2])
  {
    this->Fetcher->SetDimension(_arg);
    this->Modified();
  }

  inline void SetProvider(int _arg)
  {
    this->Fetcher->SetProvider(_arg);
    this->Modified();
  }

  inline void SetType(int _arg)
  {
    this->Fetcher->SetType(_arg);
    this->Modified();
  }

  inline void SetUpscale(bool _arg)
  {
    this->Fetcher->SetUpscale(_arg);
    this->Modified();
  }

  inline void SetAPIKey(const std::string& _arg)
  {
    this->Fetcher->SetAPIKey(_arg);
    this->Modified();
  }
  //@}

protected:
  vtkGeoMapFetcherMesh() = default;
  ~vtkGeoMapFetcherMesh() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkNew<vtkGeoMapFetcher> Fetcher;

private:
  // Projection parameters
  int MeshProjection = vtkGeoMapConvertFilter::LatLong;
  std::string CustomMeshProjection = "";

  vtkGeoMapFetcherMesh(const vtkGeoMapFetcherMesh&) = delete;
  void operator=(const vtkGeoMapFetcherMesh&) = delete;
};

#endif
