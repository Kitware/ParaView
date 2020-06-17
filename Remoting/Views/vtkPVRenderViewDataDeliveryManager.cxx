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
#include "vtkPVRenderViewDataDeliveryManager.h"
#include "vtkPVDataDeliveryManagerInternals.h"

#include "vtkDIYKdTreeUtilities.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMPIMoveData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingMacros.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <map>
#include <numeric>
#include <queue>
#include <sstream>
#include <utility>

namespace
{

static const int STREAMING_DATA_KEY = 1024;
static const int REDISTRIBUTED_DATA_KEY = 1025;

class vtkPVRVDMKeys : public vtkObject
{
public:
  vtkTypeMacro(vtkPVRVDMKeys, vtkObject);

  static vtkInformationIntegerKey* CLONE_TO_ALL_PROCESSES();
  static vtkInformationIntegerKey* DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES();
  static vtkInformationIntegerKey* GATHER_BEFORE_DELIVERING_TO_CLIENT();
  static vtkInformationIntegerKey* REDISTRIBUTION_MODE();
  static vtkInformationIntegerKey* IS_STREAMABLE();
  static vtkInformationDoubleVectorKey* GEOMETRY_BOUNDS();
  static vtkInformationDoubleVectorKey* TRANSFORMED_GEOMETRY_BOUNDS();
  static vtkInformationIntegerKey* ORDERED_COMPOSITING_CONFIGURATION();
  static vtkInformationDoubleVectorKey* ORDERED_COMPOSITING_BOUNDS();

  static int GetOrderedCompositingConfiguration(vtkInformation* info)
  {
    return info->Has(vtkPVRVDMKeys::ORDERED_COMPOSITING_CONFIGURATION())
      ? info->Get(vtkPVRVDMKeys::ORDERED_COMPOSITING_CONFIGURATION())
      : 0;
  }

protected:
  vtkPVRVDMKeys() = default;
  ~vtkPVRVDMKeys() override = default;

private:
  vtkPVRVDMKeys(const vtkPVRVDMKeys&) = delete;
  void operator=(const vtkPVRVDMKeys&) = delete;
};
vtkInformationKeyMacro(vtkPVRVDMKeys, CLONE_TO_ALL_PROCESSES, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, GATHER_BEFORE_DELIVERING_TO_CLIENT, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, REDISTRIBUTION_MODE, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, IS_STREAMABLE, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, ORDERED_COMPOSITING_CONFIGURATION, Integer);
vtkInformationKeyRestrictedMacro(vtkPVRVDMKeys, ORDERED_COMPOSITING_BOUNDS, DoubleVector, 6);
vtkInformationKeyRestrictedMacro(vtkPVRVDMKeys, GEOMETRY_BOUNDS, DoubleVector, 6);
vtkInformationKeyRestrictedMacro(vtkPVRVDMKeys, TRANSFORMED_GEOMETRY_BOUNDS, DoubleVector, 6);
} // end of namespace

//*****************************************************************************
vtkStandardNewMacro(vtkPVRenderViewDataDeliveryManager);
//----------------------------------------------------------------------------
vtkPVRenderViewDataDeliveryManager::vtkPVRenderViewDataDeliveryManager()
{
}

//----------------------------------------------------------------------------
vtkPVRenderViewDataDeliveryManager::~vtkPVRenderViewDataDeliveryManager()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetDeliverToAllProcesses(
  vtkPVDataRepresentation* repr, bool mode, bool low_res, int port)
{
  if (auto info = this->GetPieceInformation(repr, low_res, port))
  {
    info->Set(vtkPVRVDMKeys::CLONE_TO_ALL_PROCESSES(), mode ? 1 : 0);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetDeliverToClientAndRenderingProcesses(
  vtkPVDataRepresentation* repr, bool deliver_to_client, bool gather_before_delivery, bool low_res,
  int port)
{
  if (auto info = this->GetPieceInformation(repr, low_res, port))
  {
    info->Set(
      vtkPVRVDMKeys::DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES(), deliver_to_client ? 1 : 0);
    info->Set(vtkPVRVDMKeys::GATHER_BEFORE_DELIVERING_TO_CLIENT(), gather_before_delivery ? 1 : 0);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetRedistributionMode(
  vtkPVDataRepresentation* repr, int mode, int port)
{
  if (auto info = this->GetPieceInformation(repr, false, port))
  {
    info->Set(vtkPVRVDMKeys::REDISTRIBUTION_MODE(), mode);
  }
  if (auto info = this->GetPieceInformation(repr, true, port))
  {
    info->Set(vtkPVRVDMKeys::REDISTRIBUTION_MODE(), mode);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetRedistributionModeToSplitBoundaryCells(
  vtkPVDataRepresentation* repr, int port)
{
  this->SetRedistributionMode(repr, vtkOrderedCompositeDistributor::SPLIT_BOUNDARY_CELLS, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetRedistributionModeToDuplicateBoundaryCells(
  vtkPVDataRepresentation* repr, int port)
{
  this->SetRedistributionMode(
    repr, vtkOrderedCompositeDistributor::ASSIGN_TO_ALL_INTERSECTING_REGIONS, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetRedistributionModeToUniquelyAssignBoundaryCells(
  vtkPVDataRepresentation* repr, int port)
{
  this->SetRedistributionMode(repr, vtkOrderedCompositeDistributor::ASSIGN_TO_ONE_REGION, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetStreamable(
  vtkPVDataRepresentation* repr, bool val, int port)
{
  if (auto info = this->GetPieceInformation(repr, false, port))
  {
    info->Set(vtkPVRVDMKeys::IS_STREAMABLE(), val);
  }
  if (auto info = this->GetPieceInformation(repr, true, port))
  {
    info->Set(vtkPVRVDMKeys::IS_STREAMABLE(), val);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetOrderedCompositingConfiguration(
  vtkPVDataRepresentation* repr, int config, const double* bds, int port)
{
  if (auto info = this->GetPieceInformation(repr, /*low_res=*/false, port))
  {
    info->Set(vtkPVRVDMKeys::ORDERED_COMPOSITING_CONFIGURATION(), config);
    if (bds && vtkMath::AreBoundsInitialized(bds))
    {
      info->Set(vtkPVRVDMKeys::ORDERED_COMPOSITING_BOUNDS(), bds, 6);
    }
    else
    {
      info->Remove(vtkPVRVDMKeys::ORDERED_COMPOSITING_BOUNDS());
    }
  }
  if (auto info = this->GetPieceInformation(repr, /*low_res=*/true, port))
  {
    info->Set(vtkPVRVDMKeys::ORDERED_COMPOSITING_CONFIGURATION(), config);
    if (bds && vtkMath::AreBoundsInitialized(bds))
    {
      info->Set(vtkPVRVDMKeys::ORDERED_COMPOSITING_BOUNDS(), bds, 6);
    }
    else
    {
      info->Remove(vtkPVRVDMKeys::ORDERED_COMPOSITING_BOUNDS());
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetGeometryBounds(
  vtkPVDataRepresentation* repr, const double bounds[6], vtkMatrix4x4* matrix, int port)
{
  double transformed_bounds[6];
  std::copy(bounds, bounds + 6, transformed_bounds);
  const vtkBoundingBox bbox(bounds);
  if (matrix && bbox.IsValid())
  {
    double min_point[4] = { bounds[0], bounds[2], bounds[4], 1 };
    double max_point[4] = { bounds[1], bounds[3], bounds[5], 1 };
    matrix->MultiplyPoint(min_point, min_point);
    matrix->MultiplyPoint(max_point, max_point);
    transformed_bounds[0] = min_point[0] / min_point[3];
    transformed_bounds[2] = min_point[1] / min_point[3];
    transformed_bounds[4] = min_point[2] / min_point[3];
    transformed_bounds[1] = max_point[0] / max_point[3];
    transformed_bounds[3] = max_point[1] / max_point[3];
    transformed_bounds[5] = max_point[2] / max_point[3];
  }

  // Eventually, we may want to cache the matrix too so we can use it when
  // building the kd-tree, for example (see paraview/paraview#19691)
  if (auto info = this->GetPieceInformation(repr, /*low_res=*/false, port))
  {
    info->Set(vtkPVRVDMKeys::GEOMETRY_BOUNDS(), bounds, 6);
    info->Set(vtkPVRVDMKeys::TRANSFORMED_GEOMETRY_BOUNDS(), transformed_bounds, 6);
  }
  if (auto info = this->GetPieceInformation(repr, /*low_res=*/true, port))
  {
    info->Set(vtkPVRVDMKeys::GEOMETRY_BOUNDS(), bounds, 6);
    info->Set(vtkPVRVDMKeys::TRANSFORMED_GEOMETRY_BOUNDS(), transformed_bounds, 6);
  }
}

//----------------------------------------------------------------------------
vtkBoundingBox vtkPVRenderViewDataDeliveryManager::GetGeometryBounds(
  vtkPVDataRepresentation* repr, int port)
{
  vtkBoundingBox bbox;
  if (auto info = this->GetPieceInformation(repr, /*low-res*/ false, port))
  {
    if (info->Has(vtkPVRVDMKeys::GEOMETRY_BOUNDS()))
    {
      double gbds[6];
      info->Get(vtkPVRVDMKeys::GEOMETRY_BOUNDS(), gbds);
      bbox.AddBounds(gbds);
    }
  }
  return bbox;
}

//----------------------------------------------------------------------------
vtkBoundingBox vtkPVRenderViewDataDeliveryManager::GetTransformedGeometryBounds(
  vtkPVDataRepresentation* repr, int port)
{
  vtkBoundingBox bbox;
  if (auto info = this->GetPieceInformation(repr, /*low-res*/ false, port))
  {
    if (info->Has(vtkPVRVDMKeys::TRANSFORMED_GEOMETRY_BOUNDS()))
    {
      double gbds[6];
      info->Get(vtkPVRVDMKeys::TRANSFORMED_GEOMETRY_BOUNDS(), gbds);
      bbox.AddBounds(gbds);
    }
  }
  return bbox;
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::RedistributeDataForOrderedCompositing(bool low_res)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  const int num_ranks = controller ? controller->GetNumberOfProcesses() : 1;
  if (this->GetView()->GetUpdateTimeStamp() > this->RedistributionTimeStamp)
  {
    this->RedistributionTimeStamp.Modified();

    // looks like the view has been `update`d since we last came here. However,
    // that still doesn't imply that the geometry changed enough to require us
    // to re-generate kd-tree. So we build a token that helps us determine if
    // something significant changed.
    std::ostringstream token_stream;
    std::vector<vtkDataObject*> data_for_loadbalacing;
    bool use_explicit_bounds = false;
    vtkBoundingBox local_bounds;
    for (auto iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end();
         ++iter)
    {
      auto repr = this->GetRepresentation(iter->first.first);
      const auto cacheKey = this->GetCacheKey(repr);
      vtkInternals::vtkItem& item = iter->second.first;
      auto info = item.GetPieceInformation(cacheKey);
      const int mode = this->GetViewDataDistributionMode(low_res);
      if (this->Internals->IsRepresentationVisible(iter->first.first))
      {
        const int config = vtkPVRVDMKeys::GetOrderedCompositingConfiguration(info);
        if ((config & vtkPVRenderView::USE_DATA_FOR_LOAD_BALANCING) != 0)
        {
          token_stream << ";a" << iter->first.first << "=" << item.GetTimeStamp(cacheKey);
          data_for_loadbalacing.push_back(item.GetDeliveredDataObject(mode, cacheKey));
        }
        else if ((config & vtkPVRenderView::USE_BOUNDS_FOR_REDISTRIBUTION) != 0)
        {
          token_stream << ";b" << iter->first.first << "=" << item.GetTimeStamp(cacheKey);
          if (info->Has(vtkPVRVDMKeys::ORDERED_COMPOSITING_BOUNDS()))
          {
            double gbds[6];
            info->Get(vtkPVRVDMKeys::GEOMETRY_BOUNDS(), gbds);
            local_bounds.AddBounds(gbds);
          }
          else if (info->Has(vtkPVRVDMKeys::GEOMETRY_BOUNDS()))
          {
            double gbds[6];
            info->Get(vtkPVRVDMKeys::GEOMETRY_BOUNDS(), gbds);
            local_bounds.AddBounds(gbds);
          }
          use_explicit_bounds = true;
        }
      }
    }

    if (this->LastCutsGeneratorToken != token_stream.str())
    {
      if (use_explicit_bounds)
      {
        // we redistribution_bounds is non-empty, we don't build kd-tree and
        // just use the bounds given to us.
        double lbds[6];
        local_bounds.GetBounds(lbds);
        std::vector<double> all_bounds(num_ranks * 6);
        controller->AllGather(lbds, &all_bounds[0], 6);
        this->Cuts.resize(num_ranks);
        for (int cc = 0; cc < num_ranks; ++cc)
        {
          this->Cuts[cc].SetBounds(&all_bounds[6 * cc]);
        }
        this->RawCuts.clear();
        this->RawCutsRankAssignments.clear();
      }
      else
      {
        vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "regenerate kd-tree");
        this->Cuts = vtkDIYKdTreeUtilities::GenerateCuts(
          data_for_loadbalacing, num_ranks, /*use_cell_centers*/ false, controller);

        // save raw cuts and assignments.
        this->RawCuts = this->Cuts;
        this->RawCutsRankAssignments = vtkDIYKdTreeUtilities::ComputeAssignments(
          static_cast<int>(this->RawCuts.size()), controller->GetNumberOfProcesses());

        // Now, resize cuts to match the number of ranks we're rendering on.
        vtkDIYKdTreeUtilities::ResizeCuts(this->Cuts, controller->GetNumberOfProcesses());
      }
      this->LastCutsGeneratorToken = token_stream.str();
      this->CutsMTime.Modified();
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(),
        "skipping kd-tree regeneration (nothing relevant changed).");
    }
  }

  if (this->Cuts.size() == 0)
  {
    return;
  }

  bool anything_moved = false;
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    const auto id = iter->first.first;
    vtkInternals::vtkItem& item = low_res ? iter->second.second : iter->second.first;

    if (!this->Internals->IsRepresentationVisible(id))
    {
      // skip hidden representations;
      continue;
    }

    auto repr = this->GetRepresentation(id);
    const auto cacheKey = this->GetCacheKey(repr);
    const auto debugName = repr->GetLogName();
    auto info = item.GetPieceInformation(cacheKey);
    const int viewMode = this->GetViewDataDistributionMode(low_res);
    vtkDataObject* deliveredDataObject = item.GetDeliveredDataObject(viewMode, cacheKey);
    if (deliveredDataObject == nullptr)
    {
      continue;
    }

    const int mode = this->GetMoveMode(info, viewMode);
    const int config = vtkPVRVDMKeys::GetOrderedCompositingConfiguration(info);
    if (mode == vtkMPIMoveData::CLONE || (config & vtkPVRenderView::DATA_IS_REDISTRIBUTABLE) == 0)
    {
      // nothing to do, item is either non-redistributable, or
      // data is already cloned on all ranks...no redistribution is needed.
      item.SetDeliveredDataObject(REDISTRIBUTED_DATA_KEY, cacheKey, deliveredDataObject);
    }
    else if (mode != vtkMPIMoveData::PASS_THROUGH &&
      mode != vtkMPIMoveData::COLLECT_AND_PASS_THROUGH)
    {
      // nothing to do, mode is not
      // PASS_THROUGH or COLLECT_AND_PASS_THROUGH -- the only modes in which we support data
      // redistribution.
    }
    else
    {
      auto redistributedObject = item.GetDeliveredDataObject(REDISTRIBUTED_DATA_KEY, cacheKey);
      if (redistributedObject == nullptr || redistributedObject->GetMTime() < this->CutsMTime ||
        redistributedObject->GetMTime() < deliveredDataObject->GetMTime())
      {
        item.SetDeliveredDataObject(REDISTRIBUTED_DATA_KEY, cacheKey, nullptr);
        vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "redistribute: %s", debugName.c_str());
        vtkNew<vtkOrderedCompositeDistributor> redistributor;
        redistributor->SetController(vtkMultiProcessController::GetGlobalController());
        redistributor->SetInputData(deliveredDataObject);
        redistributor->SetCuts(this->Cuts);
        redistributor->SetBoundaryMode(info->Has(vtkPVRVDMKeys::REDISTRIBUTION_MODE())
            ? info->Get(vtkPVRVDMKeys::REDISTRIBUTION_MODE())
            : vtkOrderedCompositeDistributor::SPLIT_BOUNDARY_CELLS);
        redistributor->Update();
        // TODO: give representation a change to "cleanup" redistributed data
        item.SetDeliveredDataObject(
          REDISTRIBUTED_DATA_KEY, cacheKey, redistributor->GetOutputDataObject(0));
        anything_moved = true;
      }
    }
  }

  if (!anything_moved)
  {
    vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "no redistribution was done.");
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::ClearRedistributedData(bool low_res)
{
  // It seems like we should be able to set each item's RedistributedDataObject
  // to NULL in this loop but that doesn't work. For now we're leaving this as
  // is to make sure we don't break functionality but this should be revisited
  // later.
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    if (!this->Internals->IsRepresentationVisible(iter->first.first))
    {
      continue;
    }
    auto repr = this->GetRepresentation(iter->first.first);
    const auto cacheKey = this->GetCacheKey(repr);
    vtkInternals::vtkItem& item = low_res ? iter->second.second : iter->second.first;
    item.SetDeliveredDataObject(REDISTRIBUTED_DATA_KEY, cacheKey, nullptr);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::SetNextStreamedPiece(
  vtkPVDataRepresentation* repr, vtkDataObject* data, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr,
    /*low_res=*/false, port, true);
  if (item == NULL)
  {
    vtkErrorMacro("Invalid argument.");
    return;
  }

  // For now, I am going to keep things simple. Piece is delivered to the
  // representation separately. That's it.
  const auto cacheKey = this->GetCacheKey(repr);
  item->SetDeliveredDataObject(STREAMING_DATA_KEY, cacheKey, data);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVRenderViewDataDeliveryManager::GetCurrentStreamedPiece(
  vtkPVDataRepresentation* repr, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr,
    /*low_res=*/false, port);
  if (item == NULL)
  {
    vtkErrorMacro("Invalid argument.");
    return NULL;
  }
  const auto cacheKey = this->GetCacheKey(repr);
  return item->GetDeliveredDataObject(STREAMING_DATA_KEY, cacheKey);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::ClearStreamedPieces()
{
  // I am not too sure if I want to do this. Right now I am thinking once a
  // piece is delivered, the delivery manager should no longer bother about it.
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    vtkInternals::vtkItem& item = iter->second.first;
    auto repr = this->GetRepresentation(iter->first.first);
    const auto cacheKey = this->GetCacheKey(repr);
    item.SetDeliveredDataObject(STREAMING_DATA_KEY, cacheKey, nullptr);
  }
}

//----------------------------------------------------------------------------
bool vtkPVRenderViewDataDeliveryManager::GetRepresentationsReadyToStreamPieces(
  std::vector<unsigned int>& keys)
{
  // I am not too sure if I want to do this. Right now I am thinking once a
  // piece is delivered, the delivery manager should no longer bother about it.
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    if (this->Internals->IsRepresentationVisible(iter->first.first))
    {
      vtkInternals::vtkItem& item = iter->second.first;
      auto repr = this->GetRepresentation(iter->first.first);
      const auto cacheKey = this->GetCacheKey(repr);
      auto info = item.GetPieceInformation(cacheKey);
      if (info->Has(vtkPVRVDMKeys::IS_STREAMABLE()) &&
        info->Get(vtkPVRVDMKeys::IS_STREAMABLE()) == 1 &&
        item.GetDeliveredDataObject(STREAMING_DATA_KEY, cacheKey) != nullptr)
      {
        // FIXME: make keys a vector<pair<unsigned int, int> >.
        keys.push_back(iter->first.first);
        keys.push_back(static_cast<unsigned int>(iter->first.second));
      }
    }
  }
  return (keys.size() > 0);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::DeliverStreamedPieces(
  unsigned int size, unsigned int* values)
{
  // This method gets called on all processes to deliver any streamed pieces
  // currently available. This is similar to Deliver(...) except that this deals
  // with only delivering pieces for streaming.
  assert(size % 2 == 0);

  for (unsigned int cc = 0; cc < size; cc += 2)
  {
    unsigned int rid = values[cc];
    int port = static_cast<int>(values[cc + 1]);
    vtkInternals::vtkItem* item = this->Internals->GetItem(rid, false, port);
    auto repr = this->GetRepresentation(rid);
    const auto cacheKey = this->GetCacheKey(repr);

    // FIXME: we need information about the datatype on all processes. For now
    // we assume that the data type is same as the full-data (which is not
    // really necessary). We can API to allow representations to be able to
    // specify the data type.
    vtkDataObject* data = item->GetDataObject(cacheKey);
    vtkDataObject* piece = item->GetDeliveredDataObject(STREAMING_DATA_KEY, cacheKey);

    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(data->GetDataObjectType());
    dataMover->SetMoveMode(this->GetViewDataDistributionMode(/*low_res=*/false));
    dataMover->SetInputData(piece);
    dataMover->Update();
    if (dataMover->GetOutputGeneratedOnProcess())
    {
      item->SetDeliveredDataObject(STREAMING_DATA_KEY, cacheKey, dataMover->GetOutputDataObject(0));
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVRenderViewDataDeliveryManager::GetViewDataDistributionMode(bool low_res) const
{
  auto renderView = vtkPVRenderView::SafeDownCast(this->GetView());
  assert(renderView != nullptr);
  return renderView->GetDataDistributionMode(low_res);
}

//----------------------------------------------------------------------------
int vtkPVRenderViewDataDeliveryManager::GetDeliveredDataKey(bool low_res) const
{
  // for key, we simply use the mode the view is currently in. This helps keep
  // data for each mode separate.
  return this->UseRedistributedDataAsDeliveredData ? REDISTRIBUTED_DATA_KEY
                                                   : this->GetViewDataDistributionMode(low_res);
}

//----------------------------------------------------------------------------
int vtkPVRenderViewDataDeliveryManager::GetMoveMode(vtkInformation* info, int viewMode) const
{
  if (info->Has(vtkPVRVDMKeys::CLONE_TO_ALL_PROCESSES()) &&
    info->Get(vtkPVRVDMKeys::CLONE_TO_ALL_PROCESSES()) == 1)
  {
    return vtkMPIMoveData::CLONE;
  }

  if (info->Has(vtkPVRVDMKeys::DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES()) &&
    info->Get(vtkPVRVDMKeys::DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES()) == 1)
  {
    if (viewMode == vtkMPIMoveData::PASS_THROUGH)
    {
      return vtkMPIMoveData::COLLECT_AND_PASS_THROUGH;
    }
    else
    {
      // nothing to do, since the data is going to be delivered to the client
      // anyways.
    }
  }
  return viewMode;
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::MoveData(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  const auto cacheKey = this->GetCacheKey(repr);
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res, port);

  auto info = item->GetPieceInformation(cacheKey);
  const int viewMode = this->GetViewDataDistributionMode(low_res);
  const int moveMode = this->GetMoveMode(info, viewMode);

  auto dataObj = item->GetDataObject(cacheKey);
  assert(dataObj != nullptr);

  vtkNew<vtkMPIMoveData> dataMover;
  dataMover->InitializeForCommunicationForParaView();
  dataMover->SetOutputDataType(dataObj->GetDataObjectType());
  dataMover->SetMoveMode(moveMode);
  if (info->Has(vtkPVRVDMKeys::DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES()) &&
    info->Get(vtkPVRVDMKeys::DELIVER_TO_CLIENT_AND_RENDERING_PROCESSES()) == 1)
  {
    dataMover->SetSkipDataServerGatherToZero(
      info->Get(vtkPVRVDMKeys::GATHER_BEFORE_DELIVERING_TO_CLIENT()) == 0);
  }
  dataMover->SetInputData(dataObj);
  dataMover->Update();
  item->SetDeliveredDataObject(viewMode, cacheKey, dataMover->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
