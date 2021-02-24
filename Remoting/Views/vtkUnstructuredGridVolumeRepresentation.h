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

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkVolumeRepresentation.h"

class vtkAbstractVolumeMapper;
class vtkColorTransferFunction;
class vtkDataSet;
class vtkOutlineSource;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkProjectedTetrahedraMapper;
class vtkPVGeometryFilter;
class vtkPVLODVolume;
class vtkResampleToImage;
class vtkVolumeProperty;
class vtkVolumeRepresentationPreprocessor;

class VTKREMOTINGVIEWS_EXPORT vtkUnstructuredGridVolumeRepresentation
  : public vtkVolumeRepresentation
{
public:
  static vtkUnstructuredGridVolumeRepresentation* New();
  vtkTypeMacro(vtkUnstructuredGridVolumeRepresentation, vtkVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  //***************************************************************************
  //@{
  /**
   * Forwarded to vtkVolumeRepresentationPreprocessor. Controls which blocks in
   * a composite dataset are rendered.
   */
  void SetActiveAssembly(const char*);
  void AddBlockSelector(const char*);
  void RemoveAllBlockSelectors();
  //@}

  //***************************************************************************
  // Forwarded to vtkResampleToImage
  void SetSamplingDimensions(int dims[3])
  {
    this->SetSamplingDimensions(dims[0], dims[1], dims[2]);
  }
  void SetSamplingDimensions(int xdim, int ydim, int zdim);

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
  ~vtkUnstructuredGridVolumeRepresentation() override;

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
   * Passes on parameters to the active volume mapper
   */
  virtual void UpdateMapperParameters();

  int ProcessViewRequestResampleToImage(
    vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo);
  int RequestDataResampleToImage(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  vtkNew<vtkVolumeRepresentationPreprocessor> Preprocessor;
  vtkNew<vtkProjectedTetrahedraMapper> DefaultMapper;

  vtkNew<vtkResampleToImage> ResampleToImageFilter;

  vtkNew<vtkPVGeometryFilter> LODGeometryFilter;
  vtkNew<vtkPolyDataMapper> LODMapper;

  bool UseDataPartitions = false;

private:
  vtkUnstructuredGridVolumeRepresentation(const vtkUnstructuredGridVolumeRepresentation&) = delete;
  void operator=(const vtkUnstructuredGridVolumeRepresentation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
