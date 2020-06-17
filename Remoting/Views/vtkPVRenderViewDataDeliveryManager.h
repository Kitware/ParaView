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

#include "vtkBoundingBox.h" // needed for iVar.
#include "vtkPVDataDeliveryManager.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // needed for iVar.
#include "vtkTuple.h"               // needed for vtkTuple.
#include "vtkWeakPointer.h"         // needed for iVar.

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtentTranslator;
class vtkInformation;
class vtkMatrix4x4;
class vtkPVDataRepresentation;
class vtkPVView;

#include <vector>

class VTKREMOTINGVIEWS_EXPORT vtkPVRenderViewDataDeliveryManager : public vtkPVDataDeliveryManager
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

  //@{
  /**
   * For representations that have indicated that the data is redistributable
   * (using SetOrderedCompositingConfiguration), this control the mode for redistribution.
   * Specifically, it indicates how to handle cells that are on the boundary of
   * the redistribution KdTree. Default is to split the cells, one can change it
   * to duplicate cells instead by using mode as
   * `vtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS`.
   */
  void SetRedistributionMode(vtkPVDataRepresentation*, int mode, int port = 0);
  void SetRedistributionModeToSplitBoundaryCells(vtkPVDataRepresentation* repr, int port = 0);
  void SetRedistributionModeToDuplicateBoundaryCells(vtkPVDataRepresentation* repr, int port = 0);
  void SetRedistributionModeToUniquelyAssignBoundaryCells(
    vtkPVDataRepresentation* repr, int port = 0);
  //@}

  /**
   * Called by the view on every render when ordered compositing is to be used to
   * ensure that the geometries are redistributed, as needed.
   */
  void RedistributeDataForOrderedCompositing(bool use_lod);

  /**
   * Removes all redistributed data that may have been redistributed for ordered compositing
   * earlier when using KdTree based redistribution.
   *
   * TODO: check is this is still needed.
   */
  void ClearRedistributedData(bool use_load);

  /**
   * Pass information how to handle data redistribution when using ordered
   * compositing.
   */
  void SetOrderedCompositingConfiguration(
    vtkPVDataRepresentation* repr, int config, const double* bds, int port = 0);

  //@{
  /**
   * Pass data bounds information.
   */
  void SetGeometryBounds(vtkPVDataRepresentation* repr, const double bds[6],
    vtkMatrix4x4* matrix = nullptr, int port = 0);
  vtkBoundingBox GetGeometryBounds(vtkPVDataRepresentation* repr, int port = 0);
  vtkBoundingBox GetTransformedGeometryBounds(vtkPVDataRepresentation* repr, int port = 0);
  //@}

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

  //@{
  /**
   * Provides access to the "cuts" built by this class when doing ordered
   * compositing. Note, this may not be up-to-date unless ordered compositing is
   * being used and are only available on the rendering-ranks i.e. pvserver or
   * pvrenderserver ranks.
   */
  const std::vector<vtkBoundingBox>& GetCuts() const { return this->Cuts; }
  vtkTimeStamp GetCutsMTime() const { return this->CutsMTime; }
  //@}

  //@{
  /**
   * When using an internally generated kd-tree for ordered compositing, this
   * method provides access to the complete kd-tree and the rendering rank
   * assignments for each node in the kd-tree.
   *
   * These are empty when the explicit data bounds are being used to determine
   * sorting order i.e. when vtkPVRenderViewDataDeliveryManager does not
   * generate its own kd tree internally instead relies on the representation
   * provided partitioning.
   */
  const std::vector<vtkBoundingBox>& GetRawCuts() const { return this->RawCuts; }
  const std::vector<int>& GetRawCutsRankAssignments() const { return this->RawCutsRankAssignments; }
  //@}

protected:
  vtkPVRenderViewDataDeliveryManager();
  ~vtkPVRenderViewDataDeliveryManager() override;

  void MoveData(vtkPVDataRepresentation* repr, bool low_res, int port) override;

  int GetViewDataDistributionMode(bool low_res) const;
  int GetMoveMode(vtkInformation* info, int viewMode) const;

  std::vector<vtkBoundingBox> Cuts;
  std::vector<vtkBoundingBox> RawCuts;
  std::vector<int> RawCutsRankAssignments;
  vtkTimeStamp CutsMTime;

  vtkTimeStamp RedistributionTimeStamp;
  std::string LastCutsGeneratorToken;
  bool UseRedistributedDataAsDeliveredData = false;

private:
  vtkPVRenderViewDataDeliveryManager(const vtkPVRenderViewDataDeliveryManager&) = delete;
  void operator=(const vtkPVRenderViewDataDeliveryManager&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkPVRenderViewDataDeliveryManager.h
