/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRStreamingVolumeRepresentation - representation used for volume
// rendering AMR datasets with ability to stream blocks.
// .SECTION Description
// vtkAMRStreamingVolumeRepresentation  is a representation used for volume
// rendering AMR datasets with ability to stream blocks from the input pipeline.

#ifndef __vtkAMRStreamingVolumeRepresentation_h
#define __vtkAMRStreamingVolumeRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include "vtkWeakPointer.h" // needed for vtkSmartPointer.

class vtkAMRStreamingPriorityQueue;
class vtkColorTransferFunction;
class vtkImageData;
class vtkOverlappingAMR;
class vtkPiecewiseFunction;
class vtkPVLODVolume;
class vtkResampledAMRImageSource;
class vtkSmartVolumeMapper;
class vtkVolumeProperty;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkAMRStreamingVolumeRepresentation :
  public vtkPVDataRepresentation
{
public:
  static vtkAMRStreamingVolumeRepresentation* New();
  vtkTypeMacro(vtkAMRStreamingVolumeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  // Overridden to skip processing when visibility if off.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool val);

  // Description:
  // Get/Set the resample buffer size. This controls the resolution at which the
  // data is resampled.
  void SetNumberOfSamples(int x, int y, int z);

  //***************************************************************************
  // Scalar coloring API (forwarded for vtkSmartVolumeMapper.

  // This is same a vtkDataObject::FieldAssociation types so you can use those
  // as well.
  enum AttributeTypes
    {
    POINT_DATA=0,
    CELL_DATA=1
    };

  // Description:
  // Methods to control scalar coloring. ColorAttributeType defines the
  // attribute type.
  void SetColorAttributeType(int val);

  // Description:
  // Pick the array to color with.
  void SetColorArrayName(const char*);

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
  void SetIndependantComponents(bool);
  
  //***************************************************************************
  // Forwarded to vtkSmartVolumeMapper.
  void SetRequestedRenderMode(int);

//BTX
protected:
  vtkAMRStreamingVolumeRepresentation();
  ~vtkAMRStreamingVolumeRepresentation();

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  vtkWeakPointer<vtkOverlappingAMR> InputData;
  vtkResampledAMRImageSource* Resampler;
  vtkSmartVolumeMapper* VolumeMapper;
  vtkVolumeProperty* Property;
  vtkPVLODVolume* Actor;
  double DataBounds[6];


  //***************************************************************************
  // For streaming support.
  virtual int RequestUpdateExtent(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Returns true if this representation has a "next piece" that it streamed. 
  bool StreamingUpdate(const double view_planes[24]);

  // Description:
  // Set to true if the input pipeline is streaming capable. Note, in
  // client-server mode, this is valid only on the data-server nodes i.e. the
  // nodes that have input pipelines connected to begin with.
  bool StreamingCapablePipeline;

  // Description:
  // Flag used to determine if RequestData() was called during streaming.
  bool InStreamingUpdate;

  vtkSmartPointer<vtkAMRStreamingPriorityQueue> PriorityQueue;

private:
  vtkAMRStreamingVolumeRepresentation(const vtkAMRStreamingVolumeRepresentation&); // Not implemented
  void operator=(const vtkAMRStreamingVolumeRepresentation&); // Not implemented

//ETX
};

#endif
