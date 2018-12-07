/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaLayerMapper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCinemaLayerMapper
 * @brief   Mapper for Cinema layers
 *
 * vtkCinemaLayerMapper is a mapper that can render layers from a Cinema
 * database. The mapper blends layer images into the active view. If camera
 * information about the camera used when generating the layers is provided,
 * then it can use that to place the image in the current view. The mapper
 * assumes that the model-view transforms for the current camera and the camera
 * used when generating the layers are identical (with exception of view up
 * orientation).
 */

#ifndef vtkCinemaLayerMapper_h
#define vtkCinemaLayerMapper_h

#include "vtkMapper2D.h"

#include "vtkNew.h"                  // for vtkNew
#include "vtkPVCinemaReaderModule.h" // for export macros
#include "vtkSmartPointer.h"         // for vtkSmartPointer
#include <string>                    // needed for std::string
#include <vector>                    // needed for std::vector

class vtkImageData;
class vtkMatrix4x4;
class vtkScalarsToColors;

class VTKPVCINEMAREADER_EXPORT vtkCinemaLayerMapper : public vtkMapper2D
{
public:
  static vtkCinemaLayerMapper* New();
  vtkTypeMacro(vtkCinemaLayerMapper, vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * We update the rendering objects in this method.
   */
  void RenderOpaqueGeometry(vtkViewport*, vtkActor2D*) override;

  /**
   * We do the actual rendering in this method.
   * TODO: it's unclear if we should render on overlay, for now, I'm doing that.
   */
  void RenderOverlay(vtkViewport*, vtkActor2D*) override;

  /**
   * Release graphics resources.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  //@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  vtkSetMacro(ScalarVisibility, bool);
  vtkGetMacro(ScalarVisibility, bool);
  vtkBooleanMacro(ScalarVisibility, bool);
  //@}

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors* lut);
  vtkScalarsToColors* GetLookupTable() { return this->LookupTable; }
  //@}

  /**
   * Overridden to include lookup table's mtime, if ScalarVisibility is ON.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the layers to render currently. Set to an empty vector to clear or call
   * ClearLayers().
   * All vtkImageData pointers in the layers should be non-null, and have
   * identical dimensions (origin, spacing, and extents).
   */
  void SetLayers(const std::vector<vtkSmartPointer<vtkImageData> >& layers);
  const std::vector<vtkSmartPointer<vtkImageData> >& GetLayers() const { return this->Layers; }
  void ClearLayers();
  //@}

  //@{
  /**
   * Set the projection matrix used when rendering the layer.
   * This creates a deep copy of the matrix passed.
   */
  void SetLayerProjectionMatrix(vtkMatrix4x4* projMat);
  vtkMatrix4x4* GetLayerProjectionMatrix();
  //@}

  //@{
  /**
   * Get/Set the up vector used when saving the layers.
   * This is needed to roll the rendered image to align with the
   * current view up.
   */
  vtkSetVector3Macro(LayerCameraViewUp, double);
  vtkGetVector3Macro(LayerCameraViewUp, double);
  //@}

  /**
   * When set to true, the mapper will ignore LayerProjectionMatrix and
   * LayerCameraViewUp and simply render the pixels from the layer image onto
   * the entire viewport (filling it entirely). This is useful for debugging or
   * in cases where the camera projection matrices in the Cinema database are
   * potentially incorrect. When using this mode, combining with other
   * regular 3D geometry in the same scene is not recommended and will produce
   * weird artifacts.
   */
  vtkSetMacro(RenderLayersAsImage, bool);
  vtkGetMacro(RenderLayersAsImage, bool);
  vtkBooleanMacro(RenderLayersAsImage, bool);

protected:
  vtkCinemaLayerMapper();
  ~vtkCinemaLayerMapper() override;

  bool ScalarVisibility;
  vtkScalarsToColors* LookupTable;

  /**
   * Keeps track of layers to render.
   */
  std::vector<vtkSmartPointer<vtkImageData> > Layers;
  vtkNew<vtkMatrix4x4> LayerProjectionMatrix;
  double LayerCameraViewUp[3];

  bool RenderLayersAsImage;

private:
  vtkCinemaLayerMapper(const vtkCinemaLayerMapper&) = delete;
  void operator=(const vtkCinemaLayerMapper&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  vtkTimeStamp BuildTime;
};

#endif
