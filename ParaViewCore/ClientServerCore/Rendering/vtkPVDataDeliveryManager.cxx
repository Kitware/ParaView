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
#include "vtkPVDataDeliveryManager.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkExtentTranslator.h"
#include "vtkKdTreeManager.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPKdTree.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPVTrivialProducer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <map>
#include <queue>
#include <sstream>
#include <utility>

//*****************************************************************************
class vtkPVDataDeliveryManager::vtkInternals
{
  friend class vtkItem;
  std::map<int, vtkSmartPointer<vtkDataObject> > EmptyDataObjectTypes;

  // This helps us avoid creating new instances of various data object types to use as
  // empty datasets. Instead, we build a map and keep reusing objects.
  vtkDataObject* GetEmptyDataObject(vtkDataObject* ref)
  {
    if (ref)
    {
      auto iter = this->EmptyDataObjectTypes.find(ref->GetDataObjectType());
      if (iter != this->EmptyDataObjectTypes.end())
      {
        return iter->second;
      }
      else
      {
        vtkSmartPointer<vtkDataObject> clone;
        clone.TakeReference(ref->NewInstance());
        this->EmptyDataObjectTypes[ref->GetDataObjectType()] = clone;
        return clone;
      }
    }
    return nullptr;
  }

public:
  class vtkPriorityQueueItem
  {
  public:
    unsigned int RepresentationId;
    unsigned int BlockId;
    unsigned int Level;
    unsigned int Index;
    double Priority;

    vtkPriorityQueueItem()
      : RepresentationId(0)
      , BlockId(0)
      , Level(0)
      , Index(0)
      , Priority(0)
    {
    }

    bool operator<(const vtkPriorityQueueItem& other) const
    {
      return this->Priority < other.Priority;
    }
  };

  typedef std::priority_queue<vtkPriorityQueueItem> PriorityQueueType;
  PriorityQueueType PriorityQueue;

  class vtkOrderedCompositingInfo
  {
  public:
    vtkSmartPointer<vtkExtentTranslator> Translator;
    double Origin[3];
    double Spacing[3];
    int WholeExtent[6];
  };

  class vtkItem
  {
    vtkSmartPointer<vtkPVTrivialProducer> Producer;

    // Data object produced by the representation.
    vtkWeakPointer<vtkDataObject> DataObject;

    // Data object available after delivery to the "rendering" node.
    std::map<int, vtkSmartPointer<vtkDataObject> > DeliveredDataObjects;

    // Data object after re-distributing when using ordered compositing, for
    // example.
    vtkSmartPointer<vtkDataObject> RedistributedDataObject;

    // Data object for a streamed piece.
    vtkSmartPointer<vtkDataObject> StreamedPiece;

    vtkMTimeType TimeStamp;
    vtkMTimeType ActualMemorySize;

  public:
    vtkOrderedCompositingInfo OrderedCompositingInfo;

    bool CloneDataToAllNodes;
    bool DeliverToClientAndRenderingProcesses;
    bool GatherBeforeDeliveringToClient;
    bool Redistributable;
    bool Streamable;
    int RedistributionMode;

    vtkItem()
      : Producer(vtkSmartPointer<vtkPVTrivialProducer>::New())
      , DataObject{}
      , DeliveredDataObjects{}
      , RedistributedDataObject{}
      , StreamedPiece{}
      , TimeStamp(0)
      , ActualMemorySize(0)
      , CloneDataToAllNodes(false)
      , DeliverToClientAndRenderingProcesses(false)
      , GatherBeforeDeliveringToClient(false)
      , Redistributable(false)
      , Streamable(false)
      , RedistributionMode(vtkOrderedCompositeDistributor::SPLIT_BOUNDARY_CELLS)
    {
    }

    void SetDataObject(vtkDataObject* data, vtkInternals* helper)
    {
      this->DataObject = data;
      this->DeliveredDataObjects.clear();
      this->RedistributedDataObject = nullptr;
      this->ActualMemorySize = data ? data->GetActualMemorySize() : 0;
      // This method gets called when data is entirely changed. That means that any
      // data we may have delivered or redistributed would also be obsolete.
      // Hence we reset the `Producer` as well. This avoids #2160.

      // explanation for using a clone: typically, the Producer is connected by the
      // representation to a rendering pipeline e.g. the mapper. As that could be the
      // case, we need to ensure the producer's input is cleaned too. Setting simply nullptr
      // could confuse the mapper and hence we setup a data object of the same type as the data.
      // we could simply set the data too, but that can lead to other confusion as the mapper should
      // never directly see the representation's data.
      this->Producer->SetOutput(helper->GetEmptyDataObject(data));

      vtkTimeStamp ts;
      ts.Modified();
      this->TimeStamp = ts;
    }

    void SetActualMemorySize(unsigned long size) { this->ActualMemorySize = size; }
    unsigned long GetActualMemorySize() const { return this->ActualMemorySize; }

    /**
     * called to move data from the DataObject to DeliveredDataObject via a
     * vtkMPIMoveData instance, if needed based on the requested
     * data_distribution_mode passed in. The item may override the
     * data_distribution_mode based on its attributes.
     */
    void Deliver(int data_distribution_mode)
    {
      auto dataObj = this->GetDataObject();
      assert(dataObj != nullptr);

      const int real_mode = this->GetItemDataDistributionMode(data_distribution_mode);
      vtkNew<vtkMPIMoveData> dataMover;
      dataMover->InitializeForCommunicationForParaView();
      dataMover->SetOutputDataType(dataObj->GetDataObjectType());
      dataMover->SetMoveMode(real_mode);
      if (this->DeliverToClientAndRenderingProcesses)
      {
        dataMover->SetSkipDataServerGatherToZero(this->GatherBeforeDeliveringToClient == false);
      }
      dataMover->SetInputData(dataObj);
      dataMover->Update();

      // Save the delivered data object. We store it in a map where key is the
      // delivery mode. This is essential to avoid clobbering data when in
      // collaboration mode and different clients have different delivery modes.
      this->DeliveredDataObjects[real_mode] = dataMover->GetOutputDataObject(0);
    }

    /**
     * called to redistribute, typically for cases where ordered compositing is
     * needed. Currently, we only support redistribution when data_distribution_mode is
     * PASS_THROUGH or COLLECT_AND_PASS_THROUGH i.e. remote rendering is being employed.
     *
     * @returns false if redistribution was skipped (or not needed) and true if
     *          data was redistributed.
     */
    bool Redistribute(int data_distribution_mode, vtkPKdTree* tree, const std::string debugName)
    {
      assert(tree != nullptr);

      const int real_mode = this->GetItemDataDistributionMode(data_distribution_mode);
      if ((real_mode != vtkMPIMoveData::PASS_THROUGH &&
            real_mode != vtkMPIMoveData::COLLECT_AND_PASS_THROUGH) ||
        this->Redistributable == false)
      {
        // nothing to do, item is either non-redistributable or mode is not
        // PASS_THROUGH or COLLECT_AND_PASS_THROUGH -- the only modes in which we support data
        // redistribution.
        return false;
      }

      vtkDataObject* deliveredDataObject = this->GetDeliveredDataObject(real_mode);
      if (deliveredDataObject == nullptr)
      {
        return false;
      }

      if (this->RedistributedDataObject == nullptr ||
        this->RedistributedDataObject->GetMTime() < tree->GetMTime() ||
        this->RedistributedDataObject->GetMTime() < deliveredDataObject->GetMTime())
      {
        // release old memory (not necessarily, but no harm).
        this->RedistributedDataObject = nullptr;

        vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "redistribute: %s", debugName.c_str());

        vtkNew<vtkOrderedCompositeDistributor> redistributor;
        redistributor->SetController(vtkMultiProcessController::GetGlobalController());
        redistributor->SetInputData(deliveredDataObject);
        redistributor->SetPKdTree(tree);
        redistributor->SetPassThrough(0);
        redistributor->SetBoundaryMode(this->RedistributionMode);
        redistributor->Update();
        this->RedistributedDataObject = redistributor->GetOutputDataObject(0);
        return true;
      }

      return false;
    }

    /**
     * cleanup the redistributed data object, on demand.
     */
    void ClearRedistributedData() { this->RedistributedDataObject = nullptr; }

    vtkDataObject* GetDeliveredDataObject(int data_distribution_mode) const
    {
      try
      {
        const int real_mode = this->GetItemDataDistributionMode(data_distribution_mode);
        return this->DeliveredDataObjects.at(real_mode);
      }
      catch (std::out_of_range&)
      {
        return nullptr;
      }
    }

    vtkDataObject* GetRedistributedDataObject()
    {
      return this->RedistributedDataObject.GetPointer();
    }

    vtkPVTrivialProducer* GetProducer(bool use_redistributed_data, int data_distribution_mode)
    {
      vtkDataObject* prev = this->Producer->GetOutputDataObject(0);
      vtkDataObject* cur = prev;
      if (use_redistributed_data && this->Redistributable &&
        this->RedistributedDataObject != nullptr)
      {
        cur = this->RedistributedDataObject;
      }
      else
      {
        cur = this->GetDeliveredDataObject(data_distribution_mode);
      }
      this->Producer->SetOutput(cur);
      if (cur != prev && cur != nullptr)
      {
        // this is only needed to overcome a bug in the mapper where they are
        // using input's mtime incorrectly.
        cur->Modified();
      }
      return this->Producer.GetPointer();
    }

    vtkDataObject* GetDataObject() const { return this->DataObject.GetPointer(); }
    vtkMTimeType GetTimeStamp() const { return this->TimeStamp; }
    vtkMTimeType GetDeliveryTimeStamp(int distribution_mode) const
    {
      if (auto dobj = this->GetDeliveredDataObject(distribution_mode))
      {
        return dobj->GetMTime();
      }
      return vtkMTimeType{ 0 };
    }
    void SetNextStreamedPiece(vtkDataObject* data) { this->StreamedPiece = data; }
    vtkDataObject* GetStreamedPiece() { return this->StreamedPiece; }

  private:
    /**
     * Each item may have overrides to the data delivery mode. This method can
     * be called to update the requested_mode based on the representation
     * specific overrides specified.
     */
    int GetItemDataDistributionMode(int requested_mode) const
    {
      if (this->CloneDataToAllNodes)
      {
        return vtkMPIMoveData::CLONE;
      }
      else if (this->DeliverToClientAndRenderingProcesses)
      {
        if (requested_mode == vtkMPIMoveData::PASS_THROUGH)
        {
          return vtkMPIMoveData::COLLECT_AND_PASS_THROUGH;
        }
        else
        {
          // nothing to do, since the data is going to be delivered to the client
          // anyways.
        }
      }
      return requested_mode;
    }
  };

  // First is repr unique id, second is the input port.
  typedef std::pair<unsigned int, int> ReprPortType;
  typedef std::map<ReprPortType, std::pair<vtkItem, vtkItem> > ItemsMapType;

  // Keep track of representation and its uid.
  typedef std::map<unsigned int, vtkWeakPointer<vtkPVDataRepresentation> > RepresentationsMapType;

  vtkItem* GetItem(unsigned int index, bool use_second, int port, bool create_if_needed = false)
  {
    ReprPortType key(index, port);
    ItemsMapType::iterator items = this->ItemsMap.find(key);
    if (items != this->ItemsMap.end())
    {
      return use_second ? &(items->second.second) : &(items->second.first);
    }
    else if (create_if_needed)
    {
      std::pair<vtkItem, vtkItem>& itemsPair = this->ItemsMap[key];
      return use_second ? &(itemsPair.second) : &(itemsPair.first);
    }
    return NULL;
  }

  vtkItem* GetItem(
    vtkPVDataRepresentation* repr, bool use_second, int port, bool create_if_needed = false)
  {
    return this->GetItem(repr->GetUniqueIdentifier(), use_second, port, create_if_needed);
  }

  unsigned long GetVisibleDataSize(bool use_second_if_available)
  {
    unsigned long size = 0;
    ItemsMapType::iterator iter;
    for (iter = this->ItemsMap.begin(); iter != this->ItemsMap.end(); ++iter)
    {
      const ReprPortType& key = iter->first;
      if (!this->IsRepresentationVisible(key.first))
      {
        // skip hidden representations.
        continue;
      }

      if (use_second_if_available && iter->second.second.GetDataObject())
      {
        size += iter->second.second.GetActualMemorySize();
      }
      else
      {
        size += iter->second.first.GetActualMemorySize();
      }
    }
    return size;
  }

  bool IsRepresentationVisible(unsigned int id) const
  {
    RepresentationsMapType::const_iterator riter = this->RepresentationsMap.find(id);
    return (riter != this->RepresentationsMap.end() && riter->second.GetPointer() != NULL &&
      riter->second->GetVisibility());
  }

  ItemsMapType ItemsMap;
  RepresentationsMapType RepresentationsMap;
};

//*****************************************************************************

vtkStandardNewMacro(vtkPVDataDeliveryManager);
//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::vtkPVDataDeliveryManager()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::~vtkPVDataDeliveryManager()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetRenderView(vtkPVRenderView* view)
{
  this->RenderView = view;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVDataDeliveryManager::GetRenderView()
{
  return this->RenderView;
}

//----------------------------------------------------------------------------
unsigned long vtkPVDataDeliveryManager::GetVisibleDataSize(bool low_res)
{
  return this->Internals->GetVisibleDataSize(low_res);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::RegisterRepresentation(vtkPVDataRepresentation* repr)
{
  assert("A representation must have a valid UniqueIdentifier" && repr->GetUniqueIdentifier());
  this->Internals->RepresentationsMap[repr->GetUniqueIdentifier()] = repr;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::UnRegisterRepresentation(vtkPVDataRepresentation* repr)
{
  unsigned int rid = repr->GetUniqueIdentifier();
  this->Internals->RepresentationsMap.erase(rid);

  vtkInternals::ItemsMapType::iterator iter = this->Internals->ItemsMap.begin();
  while (iter != this->Internals->ItemsMap.end())
  {
    const vtkInternals::ReprPortType& key = iter->first;
    if (key.first == rid)
    {
      vtkInternals::ItemsMapType::iterator toerase = iter;
      ++iter;
      this->Internals->ItemsMap.erase(toerase);
    }
    else
    {
      ++iter;
    }
  }
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation* vtkPVDataDeliveryManager::GetRepresentation(unsigned int index)
{
  vtkInternals::RepresentationsMapType::const_iterator iter =
    this->Internals->RepresentationsMap.find(index);
  return iter != this->Internals->RepresentationsMap.end() ? iter->second.GetPointer() : NULL;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetDeliverToAllProcesses(
  vtkPVDataRepresentation* repr, bool mode, bool low_res, int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/true);
  if (item)
  {
    item->CloneDataToAllNodes = mode;
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetDeliverToClientAndRenderingProcesses(
  vtkPVDataRepresentation* repr, bool deliver_to_client, bool gather_before_delivery, bool low_res,
  int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/true);
  if (item)
  {
    item->DeliverToClientAndRenderingProcesses = deliver_to_client;
    item->GatherBeforeDeliveringToClient = gather_before_delivery;
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::MarkAsRedistributable(
  vtkPVDataRepresentation* repr, bool value /*=true*/, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false, port, true);
  vtkInternals::vtkItem* low_item = this->Internals->GetItem(repr, true, port, true);
  if (item)
  {
    item->Redistributable = value;
    low_item->Redistributable = value;
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetRedistributionMode(
  vtkPVDataRepresentation* repr, int mode, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false, port, true);
  vtkInternals::vtkItem* low_item = this->Internals->GetItem(repr, true, port, true);
  if (item)
  {
    item->RedistributionMode = mode;
    low_item->RedistributionMode = mode;
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetRedistributionModeToSplitBoundaryCells(
  vtkPVDataRepresentation* repr, int port)
{
  this->SetRedistributionMode(repr, vtkOrderedCompositeDistributor::SPLIT_BOUNDARY_CELLS, port);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetRedistributionModeToDuplicateBoundaryCells(
  vtkPVDataRepresentation* repr, int port)
{
  this->SetRedistributionMode(
    repr, vtkOrderedCompositeDistributor::ASSIGN_TO_ALL_INTERSECTING_REGIONS, port);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetStreamable(vtkPVDataRepresentation* repr, bool val, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false, port, true);
  vtkInternals::vtkItem* low_item = this->Internals->GetItem(repr, true, port, true);
  if (item)
  {
    item->Streamable = val;
    low_item->Streamable = val;
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetPiece(vtkPVDataRepresentation* repr, vtkDataObject* data,
  bool low_res, unsigned long trueSize, int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/true);
  if (item)
  {
    vtkMTimeType data_time = 0;
    if (data && (data->GetMTime() > data_time))
    {
      data_time = data->GetMTime();
    }
    if (repr && repr->GetPipelineDataTime() > data_time)
    {
      data_time = repr->GetPipelineDataTime();
    }
    if (data_time > item->GetTimeStamp() || item->GetDataObject() != data)
    {
      item->SetDataObject(data, this->Internals);
    }
    if (trueSize > 0)
    {
      item->SetActualMemorySize(trueSize);
    }
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVDataDeliveryManager::GetProducer(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res, port);
  if (!item)
  {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
  }

  const int mode = this->GetViewDataDistributionMode(low_res);
  return item->GetProducer(this->RenderView->GetUseOrderedCompositing(), mode)->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetPiece(
  unsigned int id, vtkDataObject* data, bool low_res, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res, port, true);
  if (item)
  {
    item->SetDataObject(data, this->Internals);
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetOrderedCompositingInformation(vtkPVDataRepresentation* repr,
  vtkExtentTranslator* translator, const int whole_extents[6], const double origin[3],
  const double spacing[3], int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false, port, true);
  if (item)
  {
    vtkInternals::vtkOrderedCompositingInfo info;
    info.Translator = translator;
    memcpy(info.WholeExtent, whole_extents, sizeof(int) * 6);
    memcpy(info.Origin, origin, sizeof(double) * 3);
    memcpy(info.Spacing, spacing, sizeof(double) * 3);

    item->OrderedCompositingInfo = info;
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVDataDeliveryManager::GetProducer(unsigned int id, bool low_res, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res, port);
  if (!item)
  {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
  }

  const int mode = this->GetViewDataDistributionMode(low_res);
  return item->GetProducer(this->RenderView->GetUseOrderedCompositing(), mode)->GetOutputPort(0);
}

//----------------------------------------------------------------------------
int vtkPVDataDeliveryManager::GetViewDataDistributionMode(bool use_lod)
{
  assert(this->RenderView);
  const bool use_distributed_rendering = use_lod
    ? this->RenderView->GetUseDistributedRenderingForLODRender()
    : this->RenderView->GetUseDistributedRenderingForRender();
  return this->RenderView->GetDataDistributionMode(use_distributed_rendering);
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::NeedsDelivery(
  vtkMTimeType timestamp, std::vector<unsigned int>& keys_to_deliver, bool interactive)
{
  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "check for delivery (interactive=%s)",
    (interactive ? "true" : "false"));
  assert(this->RenderView);
  const bool use_lod = interactive && this->RenderView->GetUseLODForInteractiveRender();
  const int data_distribution_mode = this->GetViewDataDistributionMode(use_lod);
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    if (this->Internals->IsRepresentationVisible(iter->first.first))
    {
      vtkInternals::vtkItem& item = use_lod ? iter->second.second : iter->second.first;
      if (item.GetTimeStamp() > timestamp ||
        item.GetDeliveryTimeStamp(data_distribution_mode) < item.GetTimeStamp())
      {
        vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "needs-delivery: %s",
          this->GetRepresentation(iter->first.first)->GetLogName().c_str());
        // FIXME: convert keys_to_deliver to a vector of pairs.
        keys_to_deliver.push_back(iter->first.first);
        keys_to_deliver.push_back(static_cast<unsigned int>(iter->first.second));
      }
    }
  }
  vtkVLogIfF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), keys_to_deliver.size() == 0, "none");
  return keys_to_deliver.size() > 0;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::Deliver(int use_lod, unsigned int size, unsigned int* values)

{
  // This method gets called on all processes with the list of representations
  // to "deliver". We check with the view what mode we're operating in and
  // decide where the data needs to be delivered.
  //
  // Representations can provide overrides, e.g. though the view says data is
  // merely "pass-through", some representation says we need to clone the data
  // everywhere. That makes it critical that this method is called on all
  // processes at the same time to avoid deadlocks and other complications.
  //
  // This method will be implemented in "view-specific" subclasses since how the
  // data is delivered is very view specific.

  assert(size % 2 == 0);

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "%s data migration",
    (use_lod ? "low-resolution" : "full resolution"));
  const int mode = this->GetViewDataDistributionMode(use_lod != 0);

  for (unsigned int cc = 0; cc < size; cc += 2)
  {
    const unsigned int id = values[cc];
    const int port = static_cast<int>(values[cc + 1]);

    vtkInternals::vtkItem* item = this->Internals->GetItem(id, use_lod != 0, port);
    vtkDataObject* data = item ? item->GetDataObject() : NULL;
    if (!data)
    {
      // ideally, we want to sync this info between all ranks some other rank
      // doesn't deadlock (esp. in collaboration mode).
      continue;
    }

    vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "move-data: %s",
      this->GetRepresentation(id)->GetLogName().c_str());
    item->Deliver(mode);
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::RedistributeDataForOrderedCompositing(bool use_lod)
{
  const int mode = this->GetViewDataDistributionMode(use_lod);
  if (this->RenderView->GetUpdateTimeStamp() > this->RedistributionTimeStamp)
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
      vtkInternals::vtkItem& item = iter->second.first;
      if (this->Internals->IsRepresentationVisible(iter->first.first))
      {
        if (item.OrderedCompositingInfo.Translator)
        {
          token_stream << ";a" << iter->first.first << "=" << item.GetTimeStamp();
          // cout << "use structured info: ";
          // cout << this->GetRepresentation(iter->first.first)->GetLogName() << "("
          // <<iter->first.second<<")" << endl;
          // implies that the representation is providing us with means to
          // override how the ordered compositing happens.
          const vtkInternals::vtkOrderedCompositingInfo& info = item.OrderedCompositingInfo;
          cutsGenerator->SetStructuredDataInformation(
            info.Translator, info.WholeExtent, info.Origin, info.Spacing);
        }
        else if (item.Redistributable)
        {
          token_stream << "b" << iter->first.first << "=" << item.GetTimeStamp() << ","
                       << item.GetDeliveryTimeStamp(mode);
          // cout << "redistribute: ";
          // cout << this->GetRepresentation(iter->first.first)->GetLogName() << "("
          // <<iter->first.second<<") = "
          //   << item.GetDeliveredDataObject()
          //   << endl;
          cutsGenerator->AddDataObject(item.GetDeliveredDataObject(mode));
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
    if (!this->Internals->IsRepresentationVisible(id))
    {
      // skip hidden representations;
      continue;
    }

    const auto debugName = this->GetRepresentation(id)->GetLogName();
    vtkInternals::vtkItem& item = use_lod ? iter->second.second : iter->second.first;
    anything_moved = item.Redistribute(mode, this->KdTree, debugName) || anything_moved;
  }

  if (!anything_moved)
  {
    vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "no redistribution was done.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::ClearRedistributedData(bool use_lod)
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
    vtkInternals::vtkItem& item = use_lod ? iter->second.second : iter->second.first;
    item.ClearRedistributedData();
  }
}

//----------------------------------------------------------------------------
vtkPKdTree* vtkPVDataDeliveryManager::GetKdTree()
{
  return this->KdTree;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetNextStreamedPiece(
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
  item->SetNextStreamedPiece(data);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVDataDeliveryManager::GetCurrentStreamedPiece(
  vtkPVDataRepresentation* repr, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr,
    /*low_res=*/false, port);
  if (item == NULL)
  {
    vtkErrorMacro("Invalid argument.");
    return NULL;
  }
  return item->GetStreamedPiece();
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::ClearStreamedPieces()
{
  // I am not too sure if I want to do this. Right now I am thinking once a
  // piece is delivered, the delivery manager should no longer bother about it.
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    vtkInternals::vtkItem& item = iter->second.first;
    item.SetNextStreamedPiece(NULL);
  }
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::GetRepresentationsReadyToStreamPieces(
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
      if (item.Streamable && item.GetStreamedPiece())
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
void vtkPVDataDeliveryManager::DeliverStreamedPieces(unsigned int size, unsigned int* values)
{
  // This method gets called on all processes to deliver any streamed pieces
  // currently available. This is similar to Deliver(...) except that this deals
  // with only delivering pieces for streaming.
  assert(size % 2 == 0);

  const int mode = this->GetViewDataDistributionMode(/*use_lod*/ false);
  for (unsigned int cc = 0; cc < size; cc += 2)
  {
    unsigned int rid = values[cc];
    int port = static_cast<int>(values[cc + 1]);

    vtkInternals::vtkItem* item = this->Internals->GetItem(rid, false, port);

    // FIXME: we need information about the datatype on all processes. For now
    // we assume that the data type is same as the full-data (which is not
    // really necessary). We can API to allow representations to be able to
    // specify the data type.
    vtkDataObject* data = item->GetDataObject();
    vtkDataObject* piece = item->GetStreamedPiece();

    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(data->GetDataObjectType());
    dataMover->SetMoveMode(mode);
    if (item->CloneDataToAllNodes)
    {
      dataMover->SetMoveModeToClone();
    }
    dataMover->SetInputData(piece);
    dataMover->Update();
    if (dataMover->GetOutputGeneratedOnProcess())
    {
      item->SetNextStreamedPiece(dataMover->GetOutputDataObject(0));
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVDataDeliveryManager::GetSynchronizationMagicNumber()
{
  // The synchronization magic number is used to ensure that both the server and
  // the client know of have identical representations, because if they don't,
  // there may be a mismatch between the states of the two processes and it's
  // best to skip delivery to avoid deadlocks.
  const int prime = 31;
  int result = 1;
  result = prime * result + static_cast<int>(this->Internals->RepresentationsMap.size());
  for (auto iter = this->Internals->RepresentationsMap.begin();
       iter != this->Internals->RepresentationsMap.end(); ++iter)
  {
    result = prime * result + static_cast<int>(iter->first);
  }
  return result;
}
