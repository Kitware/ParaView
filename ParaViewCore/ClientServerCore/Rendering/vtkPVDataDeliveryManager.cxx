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
#include "vtkPVRenderView.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPVTrivialProducer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <map>
#include <queue>
#include <utility>

//*****************************************************************************
class vtkPVDataDeliveryManager::vtkInternals
{
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
    vtkSmartPointer<vtkDataObject> DeliveredDataObject;

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

    vtkItem()
      : Producer(vtkSmartPointer<vtkPVTrivialProducer>::New())
      , TimeStamp(0)
      , ActualMemorySize(0)
      , CloneDataToAllNodes(false)
      , DeliverToClientAndRenderingProcesses(false)
      , GatherBeforeDeliveringToClient(false)
      , Redistributable(false)
      , Streamable(false)
    {
    }

    void SetDataObject(vtkDataObject* data)
    {
      this->DataObject = data;
      this->ActualMemorySize = data ? data->GetActualMemorySize() : 0;

      vtkTimeStamp ts;
      ts.Modified();
      this->TimeStamp = ts;
    }

    void SetActualMemorySize(unsigned long size) { this->ActualMemorySize = size; }
    unsigned long GetActualMemorySize() const { return this->ActualMemorySize; }

    void SetDeliveredDataObject(vtkDataObject* data) { this->DeliveredDataObject = data; }

    void SetRedistributedDataObject(vtkDataObject* data) { this->RedistributedDataObject = data; }

    vtkDataObject* GetDeliveredDataObject() { return this->DeliveredDataObject.GetPointer(); }

    vtkDataObject* GetRedistributedDataObject()
    {
      return this->RedistributedDataObject.GetPointer();
    }

    vtkPVTrivialProducer* GetProducer(bool use_redistributed_data)
    {
      if (use_redistributed_data && this->Redistributable)
      {
        this->Producer->SetOutput(this->RedistributedDataObject);
      }
      else
      {
        this->Producer->SetOutput(this->DeliveredDataObject);
      }
      return this->Producer.GetPointer();
    }

    vtkDataObject* GetDataObject() const { return this->DataObject.GetPointer(); }
    vtkMTimeType GetTimeStamp() const { return this->TimeStamp; }
    void SetNextStreamedPiece(vtkDataObject* data) { this->StreamedPiece = data; }
    vtkDataObject* GetStreamedPiece() { return this->StreamedPiece; }
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
    if (data_time > item->GetTimeStamp() || item->GetDataObject() != data)
    {
      item->SetDataObject(data);
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

  return item->GetProducer(this->RenderView->GetUseOrderedCompositing())->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetPiece(
  unsigned int id, vtkDataObject* data, bool low_res, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res, port, true);
  if (item)
  {
    item->SetDataObject(data);
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

  return item->GetProducer(this->RenderView->GetUseOrderedCompositing())->GetOutputPort(0);
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::NeedsDelivery(
  vtkMTimeType timestamp, std::vector<unsigned int>& keys_to_deliver, bool use_low)
{
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    if (this->Internals->IsRepresentationVisible(iter->first.first))
    {
      vtkInternals::vtkItem& item = use_low ? iter->second.second : iter->second.first;
      if (item.GetTimeStamp() > timestamp)
      {
        // FIXME: convert keys_to_deliver to a vector of pairs.
        keys_to_deliver.push_back(iter->first.first);
        keys_to_deliver.push_back(static_cast<unsigned int>(iter->first.second));
      }
    }
  }
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

  vtkTimerLog::MarkStartEvent(use_lod ? "LowRes Data Migration" : "FullRes Data Migration");

  bool using_remote_rendering = use_lod
    ? this->RenderView->GetUseDistributedRenderingForInteractiveRender()
    : this->RenderView->GetUseDistributedRenderingForStillRender();
  int mode = this->RenderView->GetDataDistributionMode(using_remote_rendering);

  for (unsigned int cc = 0; cc < size; cc += 2)
  {
    int port = static_cast<int>(values[cc + 1]);

    vtkInternals::vtkItem* item = this->Internals->GetItem(values[cc], use_lod != 0, port);
    vtkDataObject* data = item ? item->GetDataObject() : NULL;
    if (!data)
    {
      // ideally, we want to sync this info between all ranks some other rank
      // doesn't deadlock (esp. in collaboration mode).
      continue;
    }

    //    if (data != NULL && data->IsA("vtkUniformGridAMR"))
    //      {
    //      // we are dealing with AMR datasets.
    //      // We assume for now we're not running in render-server mode. We can
    //      // ensure that at some point in future.
    //      // So we are either in pass-through or collect mode.

    //      // FIXME: check that the mode flags are "suitable" for AMR.
    //      }

    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(data ? data->GetDataObjectType() : VTK_POLY_DATA);
    dataMover->SetMoveMode(mode);
    if (item->CloneDataToAllNodes)
    {
      dataMover->SetMoveModeToClone();
    }
    else if (item->DeliverToClientAndRenderingProcesses)
    {
      if (mode == vtkMPIMoveData::PASS_THROUGH)
      {
        dataMover->SetMoveMode(vtkMPIMoveData::COLLECT_AND_PASS_THROUGH);
      }
      else
      {
        // nothing to do, since the data is going to be delivered to the client
        // anyways.
      }
      dataMover->SetSkipDataServerGatherToZero(item->GatherBeforeDeliveringToClient == false);
    }
    dataMover->SetInputData(data);

    if (dataMover->GetOutputGeneratedOnProcess())
    {
      // release old memory (not necessarily, but try).
      item->SetDeliveredDataObject(NULL);
    }
    dataMover->Update();
    if (item->GetDeliveredDataObject() == NULL)
    {
      item->SetDeliveredDataObject(dataMover->GetOutputDataObject(0));
    }
  }

  vtkTimerLog::MarkEndEvent(use_lod ? "LowRes Data Migration" : "FullRes Data Migration");
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::RedistributeDataForOrderedCompositing(bool use_lod)
{
  if (this->RenderView->GetUpdateTimeStamp() > this->RedistributionTimeStamp)
  {
    vtkTimerLog::MarkStartEvent("Regenerate Kd-Tree");
    // need to re-generate the kd-tree.
    this->RedistributionTimeStamp.Modified();

    vtkNew<vtkKdTreeManager> cutsGenerator;
    vtkInternals::ItemsMapType::iterator iter;
    for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
    {
      vtkInternals::vtkItem& item = iter->second.first;
      if (this->Internals->IsRepresentationVisible(iter->first.first))
      {
        if (item.OrderedCompositingInfo.Translator)
        {
          // implies that the representation is providing us with means to
          // override how the ordered compositing happens.
          const vtkInternals::vtkOrderedCompositingInfo& info = item.OrderedCompositingInfo;
          cutsGenerator->SetStructuredDataInformation(
            info.Translator, info.WholeExtent, info.Origin, info.Spacing);
        }
        else if (item.Redistributable)
        {
          cutsGenerator->AddDataObject(item.GetDeliveredDataObject());
        }
      }
    }
    cutsGenerator->GenerateKdTree();
    this->KdTree = cutsGenerator->GetKdTree();

    vtkTimerLog::MarkEndEvent("Regenerate Kd-Tree");
  }

  if (this->KdTree == NULL)
  {
    return;
  }

  vtkTimerLog::MarkStartEvent("Redistributing Data for Ordered Compositing");
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    if (!this->Internals->IsRepresentationVisible(iter->first.first))
    {
      // skip hidden representations;
      continue;
    }

    vtkInternals::vtkItem& item = use_lod ? iter->second.second : iter->second.first;
    if (!item.Redistributable ||
      // delivered object can be null in case we're updating lod and the
      // representation doeesn't have any LOD data.
      item.GetDeliveredDataObject() == NULL)
    {
      continue;
    }

    if (item.GetRedistributedDataObject() &&

      // input-data didn't change
      (item.GetDeliveredDataObject()->GetMTime() < item.GetRedistributedDataObject()->GetMTime()) &&

      // kd-tree didn't change
      (item.GetRedistributedDataObject()->GetMTime() > this->KdTree->GetMTime()))
    {
      // skip redistribution.
      continue;
    }

    // release old memory (not necessarily, but try).
    item.SetRedistributedDataObject(NULL);

    vtkNew<vtkOrderedCompositeDistributor> redistributor;
    redistributor->SetController(vtkMultiProcessController::GetGlobalController());
    redistributor->SetInputData(item.GetDeliveredDataObject());
    redistributor->SetPKdTree(this->KdTree);
    redistributor->SetPassThrough(0);
    redistributor->Update();
    item.SetRedistributedDataObject(redistributor->GetOutputDataObject(0));
  }
  vtkTimerLog::MarkEndEvent("Redistributing Data for Ordered Compositing");
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
    if (!item.Redistributable ||
      // delivered object can be null in case we're updating lod and the
      // representation doeesn't have any LOD data.
      item.GetDeliveredDataObject() == NULL)
    {
      continue;
    }
    item.SetRedistributedDataObject(item.GetDeliveredDataObject());
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

  bool using_remote_rendering = this->RenderView->GetUseDistributedRenderingForStillRender();
  int mode = this->RenderView->GetDataDistributionMode(using_remote_rendering);

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
