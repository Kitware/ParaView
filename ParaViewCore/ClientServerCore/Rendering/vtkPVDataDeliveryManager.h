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
// .NAME vtkPVDataDeliveryManager - manager for data-delivery.
// .SECTION Description
// vtkPVDataDeliveryManager manages geometry delivering for rendering. It is
// used by vtkPVRenderView to manage the delivery of geometry to the nodes where
// rendering is happening. This class helps us consolidate all the code for
// delivering different types of geometries to all the nodes involved as well we
// a managing idiosyncrasies like requiring delivering to all nodes,
// redistributing for ordered compositing, etc.

#ifndef __vtkPVDataDeliveryManager_h
#define __vtkPVDataDeliveryManager_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // needed for iVar.
#include "vtkWeakPointer.h" // needed for iVar.

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkPVDataRepresentation;
class vtkPVRenderView;
class vtkPKdTree;

//BTX
#include <vector>
//ETX

class VTK_EXPORT vtkPVDataDeliveryManager : public vtkObject
{
public:
  static vtkPVDataDeliveryManager* New();
  vtkTypeMacro(vtkPVDataDeliveryManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // View uses these methods to register a representation with the storage. This
  // makes it possible for representations to communicate with the storage
  // directly using a self pointer, while enables views on different processes
  // to communicate information about representations using their unique ids.
  void RegisterRepresentation(unsigned int id, vtkPVDataRepresentation*);
  void UnRegisterRepresentation(vtkPVDataRepresentation*);
  unsigned int GetRepresentationId(vtkPVDataRepresentation*);
  vtkPVDataRepresentation* GetRepresentation(unsigned int);

  // Description:
  // Representations (indirectly via vtkPVRenderView::SetPiece()) call this
  // method to register the geometry type they are rendering. Every
  // representation that requires delivering of any geometry must register with
  // the vtkPVDataDeliveryManager and never manage the delivery on its own.
  void SetPiece(vtkPVDataRepresentation* repr, vtkDataObject* data, bool low_res);
  void SetPiece(unsigned int repr_id, vtkDataObject* data, bool low_res);

  // Description:
  // Provides access to the producer port for the geometry of a registered
  // representation. Representations use these methods (indirectly via
  // vtkPVRenderView::GetPieceProducer() and GetPieceProducerLOD()) to obtain
  // the geometry producer for the geometry to be rendered.
  vtkAlgorithmOutput* GetProducer(vtkPVDataRepresentation*, bool low_res);
  vtkAlgorithmOutput* GetProducer(unsigned int, bool low_res);

  // Description:
  // By default, this class only delivers geometries to nodes that are doing the
  // rendering at a given stage. However, certain representations, such as
  // data-label representation, or cube-axes representation, need to the
  // geometry to be delivered to all nodes always. That can be done by using
  // this method (via vtkPVRenderView::SetDeliverToAllProcesses()).
  void SetDeliverToAllProcesses(vtkPVDataRepresentation*, bool flag, bool low_res);

  // Description:
  // Under certain cases, e.g. when remote rendering in parallel with
  // translucent geometry, the geometry may need to be redistributed to ensure
  // ordered compositing can be employed correctly. Marking geometry provided by
  // a representation as redistributable makes it possible for this class to
  // redistribute the geometry as needed. Only vtkPolyData, vtkUnstructuredGrid
  // or a multi-block comprising of vtkPolyData is currently supported.
  void MarkAsRedistributable(vtkPVDataRepresentation*);

  // Description:
  // Returns the size for all visible geometry. If low_res is true, and low-res
  // data is not available for a particular representation, then it's high-res
  // data size will be used assuming that the representation is going to render
  // the high-res geometry for low-res rendering as well.
  unsigned long GetVisibleDataSize(bool low_res);

  // Description:
  // Provides access to the partitioning kd-tree that was generated using the
  // data provided by the representations. The view uses this kd-tree to decide
  // on the compositing order when ordered compositing is being used.
  vtkPKdTree* GetKdTree();

  // Description:
  // Get/Set the render-view. The view is not reference counted.
  void SetRenderView(vtkPVRenderView*);
  vtkPVRenderView* GetRenderView();

  // Description:
  // Called by the view on ever render when ordered compositing is to be used to
  // ensure that the geometries are redistributed, as needed.
  void RedistributeDataForOrderedCompositing(bool use_lod);

//BTX
  // Description:
  // Internal method used to determine the list of representations that need
  // their geometry delivered. This is done on the "client" side, with the
  // client decide what geometries it needs and then requests those from the
  // server-sides using Deliver().
  bool NeedsDelivery(unsigned long timestamp,
    std::vector<unsigned int>& keys_to_deliver, bool use_low_res);
//ETX

  // Description:
  // Triggers delivery for the geometries of indicated representations.
  void Deliver(int use_low_res, unsigned int size, unsigned int *keys);

  // *******************************************************************
  // UNDER CONSTRUCTION STREAMING API
  // *******************************************************************

  // Description:
  // Mark a representation as streamable. Currently only
  // vtkAMRVolumeRepresentation is supported.
  void SetStreamable(vtkPVDataRepresentation*, bool);

  // Description:
  // Based on the current camera and currently available datasets, build a
  // priority queue.
  bool BuildPriorityQueue(double planes[24]);
  unsigned int GetRepresentationIdFromQueue();
  void StreamingDeliver(unsigned int key);
  // *******************************************************************


  // *******************************************************************
  // HACK for dealing with volume rendering for image data
  void SetImageDataProducer(vtkPVDataRepresentation* repr, vtkAlgorithmOutput*);

//BTX
protected:
  vtkPVDataDeliveryManager();
  ~vtkPVDataDeliveryManager();

  vtkWeakPointer<vtkPVRenderView> RenderView;
  vtkSmartPointer<vtkPKdTree> KdTree;

  vtkTimeStamp RedistributionTimeStamp;
private:
  vtkPVDataDeliveryManager(const vtkPVDataDeliveryManager&); // Not implemented
  void operator=(const vtkPVDataDeliveryManager&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
