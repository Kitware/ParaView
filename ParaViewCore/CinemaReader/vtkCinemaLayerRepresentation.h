/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaLayerRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCinemaLayerRepresentation
 * @brief Representation for vtkCinemaDatabaseReader.
 *
 * vtkCinemaLayerRepresentation is designed for vtkCinemaDatabaseReader. It can
 * process data produced by vtkCinemaDatabaseReader (which in reality is simply
 * meta-data about the CinemaDatabase database and query) and render layers
 * obtained from the database in a vtkPVRenderView.
 *
 * During each render, vtkCinemaLayerRepresentation generates appropriate query using
 * control parameters obtained from the reader and camera parameters determined
 * using the current camera. Using vtkCinemaDatabase,
 * vtkCinemaLayerRepresentation then obtains the layers (or images) from the
 * database corresponding to the query. The layers along with information about
 * the camera used when layer was generated is passed on to the
 * vtkCinemaLayerMapper which handles rendering the layers into the view.
 *
 * @warning Currently vtkCinemaLayerRepresentation is designed for builtin mode
 * alone. It will need some additional work to support remote rendering.
 */

#ifndef vtkCinemaLayerRepresentation_h
#define vtkCinemaLayerRepresentation_h

#include "vtkPVDataRepresentation.h"

#include "vtkNew.h"                  // needed for vtkNew.
#include "vtkPVCinemaReaderModule.h" // for export macros
#include "vtkSmartPointer.h"         // for export vtkSmartPointer

#include <string> // needed for std::string
#include <vector> // needed for std::vector

class vtkActor2D;
class vtkCamera;
class vtkCinemaDatabase;
class vtkCinemaLayerMapper;
class vtkPVCacheKeeper;
class vtkPVCameraCollection;
class vtkScalarsToColors;

class VTKPVCINEMAREADER_EXPORT vtkCinemaLayerRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkCinemaLayerRepresentation* New();
  vtkTypeMacro(vtkCinemaLayerRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void SetVisibility(bool) VTK_OVERRIDE;
  void MarkModified() VTK_OVERRIDE;
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

  /**
   * Forwarded to the mapper
   */
  void SetLookupTable(vtkScalarsToColors* lut);
  void SetRenderLayersAsImage(bool);

protected:
  vtkCinemaLayerRepresentation();
  ~vtkCinemaLayerRepresentation();

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  bool AddToView(vtkView* view) VTK_OVERRIDE;
  bool RemoveFromView(vtkView* view) VTK_OVERRIDE;
  bool IsCached(double cache_key) VTK_OVERRIDE;

  /**
   * Called in vtkPVView::REQUEST_RENDER() pass to update the base query with
   * color array selection, if any
   */
  void UpdateMapper();

private:
  vtkCinemaLayerRepresentation(const vtkCinemaLayerRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCinemaLayerRepresentation&) VTK_DELETE_FUNCTION;

  vtkNew<vtkCinemaDatabase> CinemaDatabase;
  vtkNew<vtkPVCacheKeeper> CacheKeeper;
  vtkNew<vtkCinemaLayerMapper> Mapper;
  vtkNew<vtkActor2D> Actor;

  vtkNew<vtkPVCameraCollection> Cameras;

  std::string CinemaDatabasePath;
  std::string PipelineObject;
  std::string BaseQueryJSON;
  std::string CinemaTimeStep;
  std::string FieldName;
  std::string DefaultFieldName;

  std::string PreviousQueryJSON;
};

#endif
