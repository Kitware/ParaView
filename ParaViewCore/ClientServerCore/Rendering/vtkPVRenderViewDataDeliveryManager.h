/*=========================================================================

  Program: ParaView
  Module:  vtkPVRenderViewDataDeliveryManager

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVRenderViewDataDeliveryManager
 * @brief vtkPVRenderView specific subclass of vtkPVDataDeliveryManager.
 *
 * This class adds vtkPVRenderView specific data movement logic to
 * vtkPVDataDeliveryManager.
 */

#ifndef vtkPVRenderViewDataDeliveryManager_h
#define vtkPVRenderViewDataDeliveryManager_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataDeliveryManager.h"
#include "vtkSmartPointer.h" // needed for iVar.
#include "vtkTuple.h"        // needed for vtkTuple.
#include "vtkWeakPointer.h"  // needed for iVar.

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtentTranslator;
class vtkInformation;
class vtkPKdTree;
class vtkPVDataRepresentation;
class vtkPVView;

#include <vector>

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVRenderViewDataDeliveryManager
  : public vtkPVDataDeliveryManager
{
public:
  static vtkPVRenderViewDataDeliveryManager* New();
  vtkTypeMacro(vtkPVRenderViewDataDeliveryManager, vtkPVDataDeliveryManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * By default, this class only delivers geometries to nodes that are doing the
   * rendering at a given stage. However, certain representations, such as
   * data-label representation, or cube-axes representation, need to the
   * geometry to be delivered to all nodes always. That can be done by using
   * this method (via vtkPVRenderView::SetDeliverToAllProcesses()).
   */
  void SetDeliverToAllProcesses(vtkPVDataRepresentation*, bool flag, bool low_res, int port = 0);

  /**
   * By default, this class only delivers geometries to nodes that are doing the
   * rendering at a given stage. However, certain representations, such as
   * text-source representation, need to the geometry to be delivered to  the
   * client as well.  That can be done by using this method (via
   * vtkPVRenderView::SetDeliverToAllProcesses()). The different between
   * SetDeliverToAllProcesses() and this is that the former gather-and-scatters
   * the data on the server nodes, while the latter will optionally gather the data to
   * deliver to the client and never scatter.
   */
  void SetDeliverToClientAndRenderingProcesses(vtkPVDataRepresentation*, bool deliver_to_client,
    bool gather_before_delivery, bool low_res, int port = 0);

  /**
   * Under certain cases, e.g. when remote rendering in parallel with
   * translucent geometry, the geometry may need to be redistributed to ensure
   * ordered compositing can be employed correctly. Marking geometry provided by
   * a representation as redistributable makes it possible for this class to
   * redistribute the geometry as needed. Only vtkPolyData, vtkUnstructuredGrid
   * or a multi-block comprising of vtkPolyData is currently supported.
   */
  void MarkAsRedistributable(vtkPVDataRepresentation*, bool value = true, int port = 0);

  //@{
  /**
   * For representations that have indicated that the data is redistributable
   * (using MarkAsRedistributable), this control the mode for redistribution.
   * Specifically, it indicates how to handle cells that are on the boundary of
   * the redistribution KdTree. Default is to split the cells, one can change it
   * to duplicate cells instead by using mode as
   * `vtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS`.
   */
  void SetRedistributionMode(vtkPVDataRepresentation*, int mode, int port = 0);
  void SetRedistributionModeToSplitBoundaryCells(vtkPVDataRepresentation* repr, int port = 0);
  void SetRedistributionModeToDuplicateBoundaryCells(vtkPVDataRepresentation* repr, int port = 0);
  //@}

  /**
   * Provides access to the partitioning kd-tree that was generated using the
   * data provided by the representations. The view uses this kd-tree to decide
   * on the compositing order when ordered compositing is being used.
   */
  vtkPKdTree* GetKdTree();

  /**
   * Called by the view on every render when ordered compositing is to be used to
   * ensure that the geometries are redistributed, as needed.
   */
  void RedistributeDataForOrderedCompositing(bool use_lod);

  /**
   * Removes all redistributed data that may have been redistributed for ordered compositing
   * earlier when using KdTree based redistribution.
   */
  void ClearRedistributedData(bool use_load);

  /**
   * Pass the structured-meta-data for determining rendering order for ordered
   * compositing.
   */
  void SetOrderedCompositingInformation(vtkPVDataRepresentation* repr,
    vtkExtentTranslator* translator, const int whole_extents[6], const double origin[3],
    const double spacing[3], int port = 0);

  // *******************************************************************
  // UNDER CONSTRUCTION STREAMING API
  // *******************************************************************

  /**
   * Mark a representation as streamable. Any representation can indicate that
   * it is streamable i.e. the view can call streaming passses on it and it will
   * deliver data incrementally.
   */
  void SetStreamable(vtkPVDataRepresentation*, bool, int port = 0);

  //@{
  /**
   * Passes the current streamed piece. This is the piece that will be delivered
   * to the rendering node.
   */
  void SetNextStreamedPiece(vtkPVDataRepresentation* repr, vtkDataObject* piece, int port = 0);
  vtkDataObject* GetCurrentStreamedPiece(vtkPVDataRepresentation* repr, int port = 0);
  void ClearStreamedPieces();
  //@}

  /**
   * Deliver streamed pieces. Unlike regular data, streamed pieces are delivered
   * and released. Representations are expected to manage the pieces once they
   * are delivered to them.
   */
  void DeliverStreamedPieces(unsigned int size, unsigned int* keys);

  /**
   * Fills up the vector with the keys for representations that have non-null
   * streaming pieces.
   */
  bool GetRepresentationsReadyToStreamPieces(std::vector<unsigned int>& keys);

  //@{
  /**
   * When set to true GetDeliveredDataKey returns `REDISTRIBUTED_DATA_KEY` else
   * returns the key returned by GetViewDataDistributionMode
   */
  vtkGetMacro(UseRedistributedDataAsDeliveredData, bool);
  vtkSetMacro(UseRedistributedDataAsDeliveredData, bool);
  //@}

  int GetDeliveredDataKey(bool low_res) const override;

protected:
  vtkPVRenderViewDataDeliveryManager();
  ~vtkPVRenderViewDataDeliveryManager() override;

  void MoveData(vtkPVDataRepresentation* repr, bool low_res, int port) override;

  int GetViewDataDistributionMode(bool low_res) const;
  int GetMoveMode(vtkInformation* info, int viewMode) const;

  vtkSmartPointer<vtkPKdTree> KdTree;
  vtkTimeStamp RedistributionTimeStamp;
  std::string LastCutsGeneratorToken;
  bool UseRedistributedDataAsDeliveredData = false;

private:
  vtkPVRenderViewDataDeliveryManager(const vtkPVRenderViewDataDeliveryManager&) = delete;
  void operator=(const vtkPVRenderViewDataDeliveryManager&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkPVRenderViewDataDeliveryManager.h
