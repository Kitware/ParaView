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

#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkKdTreeManager.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPKdTree.h"
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
  static vtkInformationIntegerKey* IS_REDISTRIBUTABLE();
  static vtkInformationIntegerKey* REDISTRIBUTION_MODE();
  static vtkInformationIntegerKey* IS_STREAMABLE();

  //@{
  // Ordered compositing meta-data.
  static vtkInformationIntegerVectorKey* WHOLE_EXTENT();
  static vtkInformationDoubleVectorKey* ORIGIN();
  static vtkInformationDoubleVectorKey* SPACING();
  static vtkInformationObjectBaseKey* EXTENT_TRANSLATOR();
  //@}

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
vtkInformationKeyMacro(vtkPVRVDMKeys, IS_REDISTRIBUTABLE, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, REDISTRIBUTION_MODE, Integer);
vtkInformationKeyMacro(vtkPVRVDMKeys, IS_STREAMABLE, Integer);
vtkInformationKeyRestrictedMacro(vtkPVRVDMKeys, WHOLE_EXTENT, IntegerVector, 6);
vtkInformationKeyRestrictedMacro(vtkPVRVDMKeys, ORIGIN, DoubleVector, 3);
vtkInformationKeyRestrictedMacro(vtkPVRVDMKeys, SPACING, DoubleVector, 3);
vtkInformationKeyMacro(vtkPVRVDMKeys, EXTENT_TRANSLATOR, ObjectBase);

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
void vtkPVRenderViewDataDeliveryManager::MarkAsRedistributable(
  vtkPVDataRepresentation* repr, bool value /*=true*/, int port)
{
  if (auto info = this->GetPieceInformation(repr, false, port))
  {
    info->Set(vtkPVRVDMKeys::IS_REDISTRIBUTABLE(), value ? 1 : 0);
  }
  if (auto info = this->GetPieceInformation(repr, true, port))
  {
    info->Set(vtkPVRVDMKeys::IS_REDISTRIBUTABLE(), value ? 1 : 0);
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
void vtkPVRenderViewDataDeliveryManager::SetOrderedCompositingInformation(
  vtkPVDataRepresentation* repr, vtkExtentTranslator* translator, const int whole_extents[6],
  const double origin[3], const double spacing[3], int port)
{
  if (auto info = this->GetPieceInformation(repr, false, port))
  {
    info->Set(vtkPVRVDMKeys::EXTENT_TRANSLATOR(), translator);
    info->Set(vtkPVRVDMKeys::WHOLE_EXTENT(), whole_extents, 6);
    info->Set(vtkPVRVDMKeys::ORIGIN(), origin, 3);
    info->Set(vtkPVRVDMKeys::SPACING(), spacing, 3);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::RedistributeDataForOrderedCompositing(bool low_res)
{
  if (this->GetView()->GetUpdateTimeStamp() > this->RedistributionTimeStamp)
  {
    this->RedistributionTimeStamp.Modified();

    // looks like the view has been `update`d since we last came here. However,
    // that still doesn't imply that the geometry changed enough to require us
    // to re-generate kd-tree. So we build a token that helps us determine if
    // something significant changed.
    std::ostringstream token_stream;
    vtkNew<vtkKdTreeManager> cutsGenerator;
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
        if (info->Has(vtkPVRVDMKeys::EXTENT_TRANSLATOR()) &&
          info->Get(vtkPVRVDMKeys::EXTENT_TRANSLATOR()) != nullptr)
        {
          token_stream << ";a" << iter->first.first << "=" << item.GetTimeStamp(cacheKey);
          // cout << "use structured info: ";
          // cout << this->GetRepresentation(iter->first.first)->GetLogName() << "("
          // <<iter->first.second<<")" << endl;
          // implies that the representation is providing us with means to
          // override how the ordered compositing happens.
          double origin[3];
          double spacing[3];
          int whole_extents[6];
          info->Get(vtkPVRVDMKeys::WHOLE_EXTENT(), whole_extents);
          info->Get(vtkPVRVDMKeys::ORIGIN(), origin);
          info->Get(vtkPVRVDMKeys::SPACING(), spacing);
          cutsGenerator->SetStructuredDataInformation(
            vtkExtentTranslator::SafeDownCast(info->Get(vtkPVRVDMKeys::EXTENT_TRANSLATOR())),
            whole_extents, origin, spacing);
        }
        else if (info->Has(vtkPVRVDMKeys::IS_REDISTRIBUTABLE()) &&
          info->Get(vtkPVRVDMKeys::IS_REDISTRIBUTABLE()) == 1)
        {
          token_stream << "b" << iter->first.first << "=" << item.GetTimeStamp(cacheKey) << ","
                       << item.GetDeliveryTimeStamp(mode, cacheKey);
          // cout << "redistribute: ";
          // cout << this->GetRepresentation(iter->first.first)->GetLogName() << "("
          // <<iter->first.second<<") = "
          //   << item.GetDeliveredDataObject()
          //   << endl;
          cutsGenerator->AddDataObject(item.GetDeliveredDataObject(mode, cacheKey));
        }
      }
    }

    if (this->LastCutsGeneratorToken != token_stream.str())
    {
      vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "regenerate kd-tree");
      cutsGenerator->GenerateKdTree();
      this->KdTree = cutsGenerator->GetKdTree();
      this->LastCutsGeneratorToken = token_stream.str();
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(),
        "skipping kd-tree regeneration (nothing relevant changed).");
    }
  }

  if (this->KdTree == nullptr)
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
    if (mode == vtkMPIMoveData::CLONE || (!info->Has(vtkPVRVDMKeys::IS_REDISTRIBUTABLE()) ||
                                           info->Get(vtkPVRVDMKeys::IS_REDISTRIBUTABLE()) == 0))
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
      if (redistributedObject == nullptr ||
        redistributedObject->GetMTime() < this->KdTree->GetMTime() ||
        redistributedObject->GetMTime() < deliveredDataObject->GetMTime())
      {
        item.SetDeliveredDataObject(REDISTRIBUTED_DATA_KEY, cacheKey, nullptr);
        vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "redistribute: %s", debugName.c_str());
        vtkNew<vtkOrderedCompositeDistributor> redistributor;
        redistributor->SetController(vtkMultiProcessController::GetGlobalController());
        redistributor->SetInputData(deliveredDataObject);
        redistributor->SetPKdTree(this->KdTree);
        redistributor->SetPassThrough(0);
        redistributor->SetBoundaryMode(info->Has(vtkPVRVDMKeys::REDISTRIBUTION_MODE())
            ? info->Get(vtkPVRVDMKeys::REDISTRIBUTION_MODE())
            : vtkOrderedCompositeDistributor::SPLIT_BOUNDARY_CELLS);
        redistributor->Update();
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
vtkPKdTree* vtkPVRenderViewDataDeliveryManager::GetKdTree()
{
  return this->KdTree;
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
