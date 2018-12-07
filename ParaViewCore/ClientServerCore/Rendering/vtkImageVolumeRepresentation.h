/*=========================================================================

  Program:   ParaView
  Module:    vtkImageVolumeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageVolumeRepresentation
 * @brief   representation for showing image
 * datasets as a volume.
 *
 * vtkImageVolumeRepresentation is a representation for volume rendering
 * vtkImageData. Unlike other data-representations used by ParaView, this
 * representation does not support delivery to client (or render server) nodes.
 * In those configurations, it merely delivers a outline for the image to the
 * client and render-server and those nodes simply render the outline.
*/

#ifndef vtkImageVolumeRepresentation_h
#define vtkImageVolumeRepresentation_h

#include "vtkNew.h"                               // needed for vtkNew.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtkColorTransferFunction;
class vtkExtentTranslator;
class vtkFixedPointVolumeRayCastMapper;
class vtkImageData;
class vtkOutlineSource;
class vtkPExtentTranslator;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkPVCacheKeeper;
class vtkPVLODVolume;
class vtkSmartVolumeMapper;
class vtkVolumeProperty;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkImageVolumeRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkImageVolumeRepresentation* New();
  vtkTypeMacro(vtkImageVolumeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  /**
   * This needs to be called on all instances of vtkGeometryRepresentation when
   * the input is modified. This is essential since the geometry filter does not
   * have any real-input on the client side which messes with the Update
   * requests.
   */
  void MarkModified() override;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool val) override;

  //***************************************************************************
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty.
  void SetInterpolationType(int val);
  void SetColor(vtkColorTransferFunction* lut);
  void SetScalarOpacity(vtkPiecewiseFunction* pwf);
  void SetScalarOpacityUnitDistance(double val);
  void SetAmbient(double);
  void SetDiffuse(double);
  void SetSpecular(double);
  void SetSpecularPower(double);
  void SetShade(bool);
  void SetMapScalars(bool);
  void SetMultiComponentsMapping(bool);

  //@{
  /**
   * Methods to set isosurface values.
   */
  void SetIsosurfaceValue(int i, double value);
  void SetNumberOfIsosurfaces(int number);
  //@}

  //***************************************************************************
  // Forwarded to vtkSmartVolumeMapper.
  void SetRequestedRenderMode(int);
  void SetShowIsosurfaces(int);

  /**
   * Provides access to the actor used by this representation.
   */
  vtkPVLODVolume* GetActor() { return this->Actor; }

protected:
  vtkImageVolumeRepresentation();
  ~vtkImageVolumeRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  /**
   * Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
   */
  bool IsCached(double cache_key) override;

  /**
   * Passes on parameters to the active volume mapper
   */
  virtual void UpdateMapperParameters();

  /**
   * Used in ConvertSelection to locate the rendered prop.
   */
  virtual vtkPVLODVolume* GetRenderedProp() { return this->Actor; };

  vtkImageData* Cache;
  vtkPVCacheKeeper* CacheKeeper;
  vtkSmartVolumeMapper* VolumeMapper;
  vtkVolumeProperty* Property;
  vtkPVLODVolume* Actor;

  vtkOutlineSource* OutlineSource;
  vtkPolyDataMapper* OutlineMapper;
  ;

  unsigned long DataSize;
  double DataBounds[6];

  // meta-data about the input image to pass on to render view for hints
  // when redistributing data.
  vtkNew<vtkPExtentTranslator> PExtentTranslator;
  double Origin[3];
  double Spacing[3];
  int WholeExtent[6];

  bool MapScalars;
  bool MultiComponentsMapping;

private:
  vtkImageVolumeRepresentation(const vtkImageVolumeRepresentation&) = delete;
  void operator=(const vtkImageVolumeRepresentation&) = delete;
};

#endif
