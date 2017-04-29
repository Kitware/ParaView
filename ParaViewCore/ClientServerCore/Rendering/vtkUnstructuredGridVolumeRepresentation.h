/*=========================================================================

  Program:   ParaView
  Module:    vtkUnstructuredGridVolumeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridVolumeRepresentation
 * @brief   representation for showing
 * vtkUnstructuredGrid datasets as volumes.
 *
 * vtkUnstructuredGridVolumeRepresentation is a representation for volume
 * rendering vtkUnstructuredGrid datasets. It simply renders a translucent
 * surface for LOD i.e. interactive rendering.
*/

#ifndef vtkUnstructuredGridVolumeRepresentation_h
#define vtkUnstructuredGridVolumeRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtkAbstractVolumeMapper;
class vtkColorTransferFunction;
class vtkOutlineSource;
class vtkPExtentTranslator;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkProjectedTetrahedraMapper;
class vtkPVCacheKeeper;
class vtkPVGeometryFilter;
class vtkPVLODVolume;
class vtkResampleToImage;
class vtkVolumeProperty;
class vtkVolumeRepresentationPreprocessor;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkUnstructuredGridVolumeRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkUnstructuredGridVolumeRepresentation* New();
  vtkTypeMacro(vtkUnstructuredGridVolumeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Register a volume mapper with the representation.
   */
  void AddVolumeMapper(const char* name, vtkAbstractVolumeMapper*);

  //@{
  /**
   * Set the active volume mapper to use.
   */
  virtual void SetActiveVolumeMapper(const char*);
  vtkAbstractVolumeMapper* GetActiveVolumeMapper();
  //@}

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

  /**
   * This needs to be called on all instances of vtkGeometryRepresentation when
   * the input is modified. This is essential since the geometry filter does not
   * have any real-input on the client side which messes with the Update
   * requests.
   */
  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   * Overridden to propagate to the active representation.
   */
  virtual void SetVisibility(bool val) VTK_OVERRIDE;

  //***************************************************************************
  // Forwarded to vtkVolumeRepresentationPreprocessor
  void SetExtractedBlockIndex(unsigned int index);

  //***************************************************************************
  // Forwarded to vtkResampleToImage
  void SetSamplingDimensions(int dims[3])
  {
    this->SetSamplingDimensions(dims[0], dims[1], dims[2]);
  }
  void SetSamplingDimensions(int xdim, int ydim, int zdim);

  //***************************************************************************
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty and vtkProperty (when applicable).
  void SetInterpolationType(int val);
  void SetColor(vtkColorTransferFunction* lut);
  void SetScalarOpacity(vtkPiecewiseFunction* pwf);
  void SetScalarOpacityUnitDistance(double val);

  /**
   * Provides access to the actor used by this representation.
   */
  vtkPVLODVolume* GetActor() { return this->Actor; }

  //@{
  /**
   * Specify whether or not to redistribute the data. The default is false
   * since that is the only way in general to guarantee correct rendering.
   * Can set to true if all rendered data sets are based on the same
   * data partitioning in order to save on the data redistribution.
   */
  virtual void SetUseDataPartitions(bool);
  vtkGetMacro(UseDataPartitions, bool);
  //@}

protected:
  vtkUnstructuredGridVolumeRepresentation();
  ~vtkUnstructuredGridVolumeRepresentation();

  /**
   * Fill input port information.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  /**
   * Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
   */
  virtual bool IsCached(double cache_key) VTK_OVERRIDE;

  /**
   * Passes on parameters to the active volume mapper
   */
  virtual void UpdateMapperParameters();

  int ProcessViewRequestResampleToImage(
    vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo);
  int RequestDataResampleToImage(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  vtkVolumeRepresentationPreprocessor* Preprocessor;
  vtkPVCacheKeeper* CacheKeeper;
  vtkProjectedTetrahedraMapper* DefaultMapper;
  vtkVolumeProperty* Property;
  vtkPVLODVolume* Actor;

  vtkResampleToImage* ResampleToImageFilter;
  unsigned long DataSize;
  vtkPExtentTranslator* PExtentTranslator;
  double Origin[3];
  double Spacing[3];
  int WholeExtent[6];
  vtkOutlineSource* OutlineSource;

  vtkPVGeometryFilter* LODGeometryFilter;
  vtkPolyDataMapper* LODMapper;
  double DataBounds[6];

  bool UseDataPartitions;

private:
  vtkUnstructuredGridVolumeRepresentation(
    const vtkUnstructuredGridVolumeRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridVolumeRepresentation&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
