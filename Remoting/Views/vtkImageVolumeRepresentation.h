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
 *
 * vtkImageVolumeRepresentation has a few caveats:
 *
 * 1. It only supports vtkImageData and vtkPartitionedDataSet
 *    comprising of vtkImageData. In case of latter, any partition not a
 *    vtkImageData will be silently skipped.
 *
 * 2. In distributed mode, bounds on each rank as assumed to be non-overlapping.
 */

#ifndef vtkImageVolumeRepresentation_h
#define vtkImageVolumeRepresentation_h

#include "vtkNew.h"                 // needed for vtkNew.
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer
#include "vtkVolumeRepresentation.h"

#include <string> // for ivar

class vtkColorTransferFunction;
class vtkDataSet;
class vtkExtentTranslator;
class vtkFixedPointVolumeRayCastMapper;
class vtkImageData;
class vtkImplicitFunction;
class vtkOutlineSource;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkPVLODVolume;
class vtkVolumeMapper;
class vtkVolumeProperty;

class VTKREMOTINGVIEWS_EXPORT vtkImageVolumeRepresentation : public vtkVolumeRepresentation
{
public:
  static vtkImageVolumeRepresentation* New();
  vtkTypeMacro(vtkImageVolumeRepresentation, vtkVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  //***************************************************************************
  // Forwarded to vtkVolumeProperty.
  void SetAmbient(double);
  void SetDiffuse(double);
  void SetSpecular(double);
  void SetSpecularPower(double);
  void SetShade(bool);
  void SetSliceFunction(vtkImplicitFunction* slice);

  //@{
  /**
   * Methods to set isosurface values.
   */
  void SetIsosurfaceValue(int i, double value);
  void SetNumberOfIsosurfaces(int number);
  //@}

  //***************************************************************************
  // Forwarded to vtkSmartVolumeMapper/vtkMultiBlockVolumeMapper.
  void SetRequestedRenderMode(int);
  void SetBlendMode(int);
  void SetCropping(int);

  //@{
  /**
   * Get/Set the cropping origin.
   */
  vtkSetVector3Macro(CroppingOrigin, double);
  vtkGetVector3Macro(CroppingOrigin, double);
  //@}

  //@{
  /**
   * Get/Set the cropping scale.
   */
  vtkSetVector3Macro(CroppingScale, double);
  vtkGetVector3Macro(CroppingScale, double);
  //@}

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
   * Passes on parameters to the active volume mapper
   */
  virtual void UpdateMapperParameters();

  /**
   * Used in ConvertSelection to locate the rendered prop.
   */
  virtual vtkPVLODVolume* GetRenderedProp() { return this->Actor; };

  vtkSmartPointer<vtkDataObject> Cache;
  vtkSmartPointer<vtkVolumeMapper> VolumeMapper;

  vtkNew<vtkPolyDataMapper> OutlineMapper;

  int WholeExtent[6] = { 0, -1, 0, -1, 0, -1 };
  double CroppingOrigin[3] = { 0, 0, 0 };
  double CroppingScale[3] = { 1, 1, 1 };

private:
  vtkImageVolumeRepresentation(const vtkImageVolumeRepresentation&) = delete;
  void operator=(const vtkImageVolumeRepresentation&) = delete;
};

#endif
