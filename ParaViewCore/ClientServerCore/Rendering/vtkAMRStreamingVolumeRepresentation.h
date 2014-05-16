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
#include "vtkBoundingBox.h" // needed for vtkBoundingBox.

class vtkAMRStreamingPriorityQueue;
class vtkColorTransferFunction;
class vtkImageData;
class vtkOverlappingAMR;
class vtkPiecewiseFunction;
class vtkPVLODVolume;
class vtkPVRenderView;
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
  
  enum ResamplingModes
    {
    RESAMPLE_OVER_DATA_BOUNDS=0,
    RESAMPLE_USING_VIEW_FRUSTUM=1
    };

  // Description:
  // This control the logic used to determine how to place the resampling grid
  // within the AMR bounds.
  // \li RESAMPLE_OVER_DATA_BOUNDS implies that the amr volume is
  // set to the data bounds and is not updated as the user interacts.
  // \li RESAMPLE_USING_VIEW_FRUSTUM indicates that the uniform grid must be
  // repositioned when the camera changes using the current view frustum.
  void SetResamplingMode(int val);
  vtkGetMacro(ResamplingMode, int);

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

  // Description:
  // Set the number of blocks to request at a given time on a single process
  // when streaming.
  vtkSetClampMacro(StreamingRequestSize, int, 1, 10000);
  vtkGetMacro(StreamingRequestSize, int);


  // Description:
  // Set the input data arrays that this algorithm will process.
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, const char *name);
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, int fieldAttributeType)
    {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, fieldAttributeType);
    }
  virtual void SetInputArrayToProcess(int idx, vtkInformation *info)
    {
    this->Superclass::SetInputArrayToProcess(idx, info);
    }
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              const char* fieldAssociation,
                              const char* attributeTypeorName)
    {
    this->Superclass::SetInputArrayToProcess(idx, port, connection,
      fieldAssociation, attributeTypeorName);
    }

  //***************************************************************************
  // Scalar coloring API (forwarded for vtkSmartVolumeMapper.

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

  // Description:
  // Overridden to check if the input pipeline is streaming capable. This method
  // should check if streaming is enabled i.e. vtkPVView::GetEnableStreaming()
  // and the input pipeline provides necessary AMR meta-data.
  virtual int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Setup the block request. During StreamingUpdate, this will request the
  // blocks based on priorities determined by the vtkAMRStreamingPriorityQueue,
  // otherwise it doesn't make any specific request. AMR sources can treat the
  // absence of specific block request to mean various things. It's expected
  // that read only the root block (or a few more) in that case.
  virtual int RequestUpdateExtent(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Process the current input for volume rendering (if anything).
  // When not in StreamingUpdate, this also initializes the priority queue since
  // the input AMR may have totally changed, including its structure.
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  // Description:
  // Returns true when the input pipeline supports streaming. It is set in
  // RequestInformation().
  vtkGetMacro(StreamingCapablePipeline, bool);

  // Description:
  // Returns true when StreamingUpdate() is being processed.
  vtkGetMacro(InStreamingUpdate, bool);

  // Description:
  // Returns true if this representation has a "next piece" that it streamed.
  // This method will update the PriorityQueue using the view planes specified
  // and then call Update() on the representation, making it reexecute and
  // regenerate the outline for the next "piece" of data.
  bool StreamingUpdate(vtkPVRenderView* view, const double view_planes[24]);

  // Description:
  // This is the data object generated processed by the most recent call to
  // RequestData() while not streaming. 
  // This is non-empty only on the data-server nodes.
  vtkSmartPointer<vtkDataObject> ProcessedData;

  // Description:
  // This is the data object generated processed by the most recent call to
  // RequestData() while streaming. 
  // This is non-empty only on the data-server nodes.
  vtkSmartPointer<vtkDataObject> ProcessedPiece;

  // Description:
  // vtkAMRStreamingPriorityQueue is a helper class we used to compute the order
  // in which to request blocks from the input pipeline. Implementations can
  // come up with their own rules to decide the request order based on
  // application and data type.
  vtkSmartPointer<vtkAMRStreamingPriorityQueue> PriorityQueue;

  // Description:
  // vtkImageData source used to resample an AMR dataset into a uniform grid
  // suitable for volume rendering.
  vtkSmartPointer<vtkResampledAMRImageSource> Resampler;

  // Description:
  // Rendering components.
  vtkSmartPointer<vtkSmartVolumeMapper> VolumeMapper;
  vtkSmartPointer<vtkVolumeProperty> Property;
  vtkSmartPointer<vtkPVLODVolume> Actor;

  // Description:
  // Used to keep track of data bounds.
  vtkBoundingBox DataBounds;

  int ResamplingMode;
  int StreamingRequestSize;

private:
  vtkAMRStreamingVolumeRepresentation(const vtkAMRStreamingVolumeRepresentation&); // Not implemented
  void operator=(const vtkAMRStreamingVolumeRepresentation&); // Not implemented

  // Description:
  // This flag is set to true if the input pipeline is streaming capable in
  // RequestInformation(). Note that in client-server mode, this is valid only
  // on the data-server nodes since all other nodes don't have input pipelines
  // connected, they cannot indicate if the pipeline supports streaming.
  bool StreamingCapablePipeline;

  // Description:
  // This flag is used to indicate that the representation is being updated
  // during the streaming pass. RequestData() can use this flag to reset
  // internal datastructures when the input changes for non-streaming reasons
  // and we need to clear our streaming buffers since the streamed data is no
  // longer valid.
  bool InStreamingUpdate;

//ETX
};

#endif
