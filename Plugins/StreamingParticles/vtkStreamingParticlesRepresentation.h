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
// .NAME vtkStreamingParticlesRepresentation - representation for rendering
// particles with streaming.
// .SECTION Description
// vtkStreamingParticlesRepresentation is a simple representation for multiblock
// datasets that with streaming capabilities.

#ifndef __vtkStreamingParticlesRepresentation_h
#define __vtkStreamingParticlesRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkSmartPointer.h" // for smart pointer.
#include "vtkWeakPointer.h" // for weak pointer.
#include "vtkBoundingBox.h" // needed for vtkBoundingBox.
#include <vector> // needed for std::vector

class vtkCompositePolyDataMapper2;
class vtkMultiBlockDataSet;
class vtkPVLODActor;
class vtkScalarsToColors;
class vtkStreamingParticlesPriorityQueue;

class vtkStreamingParticlesRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkStreamingParticlesRepresentation* New();
  vtkTypeMacro(vtkStreamingParticlesRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input data arrays that this algorithm will process. Overridden to
  // pass the array selection to the mapper.
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

  // Description:
  // Overridden to handle various view passes.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool val);

  // Description:
  // Set the number of blocks to request at a given time on a single process
  // when streaming.
  vtkSetClampMacro(StreamingRequestSize, int, 1, 10000);
  vtkGetMacro(StreamingRequestSize, int);

  // Description:
  // Helps with debugging.
  vtkSetMacro(UseOutline, bool);

  // Description:
  // Set the opacity.
  void SetOpacity(double val);

  //---------------------------------------------------------------------------
  // The following API is to simply provide the functionality similar to
  // vtkGeometryRepresentation.
  //---------------------------------------------------------------------------
  void SetLookupTable(vtkScalarsToColors*);
  void SetPointSize(double val);
    
//BTX
protected:
  vtkStreamingParticlesRepresentation();
  ~vtkStreamingParticlesRepresentation();


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
  int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Overridden to check if the input pipeline is streaming capable. This method
  // should check if streaming is enabled i.e. vtkPVView::GetEnableStreaming()
  // and the input pipeline provides necessary AMR meta-data.
  virtual int RequestInformation(vtkInformation *rqst,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);

  // Description:
  // Setup the block request. During StreamingUpdate, this will request the
  // blocks based on priorities determined by the vtkStreamingParticlesPriorityQueue,
  // otherwise it doesn't make any specific request. AMR sources can treat the
  // absence of specific block request to mean various things. It's expected
  // that read only the root block (or a few more) in that case.
  virtual int RequestUpdateExtent(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Generate the outline for the current input.
  // When not in StreamingUpdate, this also initializes the priority queue since
  // the input AMR may have totally changed, including its structure.
  virtual int RequestData(vtkInformation *rqst,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);

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
  bool StreamingUpdate(const double view_planes[24]);
  
  // Description:
  // Called in StreamingUpdate() to determine the blocks to stream in the
  // current pass. Returns false if no blocks need to be streaming currently.
  bool DetermineBlocksToStream();

  // Description:
  // This is the data object generated processed by the most recent call to
  // RequestData() while not streaming. 
  // This is non-empty only on the data-server nodes.
  vtkSmartPointer<vtkMultiBlockDataSet> ProcessedData;

  // Description:
  // This is the data object generated processed by the most recent call to
  // RequestData() while streaming. 
  // This is non-empty only on the data-server nodes.
  vtkSmartPointer<vtkDataObject> ProcessedPiece;

  // Description:
  // Helps us keep track of the data being rendered.
  vtkWeakPointer<vtkDataObject> RenderedData;

  // Description:
  // vtkStreamingParticlesPriorityQueue is a helper class we used to compute the order
  // in which to request blocks from the input pipeline. Implementations can
  // come up with their own rules to decide the request order based on
  // application and data type.
  vtkSmartPointer<vtkStreamingParticlesPriorityQueue> PriorityQueue;

  // Description:
  // Actor used to render the outlines in the view.
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkPVLODActor> Actor;

  // Description:
  // Used to keep track of data bounds.
  vtkBoundingBox DataBounds;

  std::vector<int> StreamingRequest;
  int StreamingRequestSize;
  bool UseOutline;

private:
  vtkStreamingParticlesRepresentation(const vtkStreamingParticlesRepresentation&); // Not implemented
  void operator=(const vtkStreamingParticlesRepresentation&); // Not implemented

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
