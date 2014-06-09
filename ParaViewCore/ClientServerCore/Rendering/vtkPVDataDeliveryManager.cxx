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

    vtkPriorityQueueItem() :
      RepresentationId(0), BlockId(0),
      Level(0), Index(0), Priority(0)
    {
    }

    bool operator < (const vtkPriorityQueueItem& other) const
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

    unsigned long TimeStamp;
    unsigned long ActualMemorySize;
  public:
    vtkOrderedCompositingInfo OrderedCompositingInfo;

    vtkWeakPointer<vtkPVDataRepresentation> Representation;
    bool CloneDataToAllNodes;
    bool DeliverToClientAndRenderingProcesses;
    bool GatherBeforeDeliveringToClient;
    bool Redistributable;
    bool Streamable;

    vtkItem() :
      Producer(vtkSmartPointer<vtkPVTrivialProducer>::New()),
      TimeStamp(0),
      ActualMemorySize(0),
      CloneDataToAllNodes(false),
      DeliverToClientAndRenderingProcesses(false),
      GatherBeforeDeliveringToClient(false),
      Redistributable(false),
      Streamable(false)
    { }

    void SetDataObject(vtkDataObject* data)
      {
      this->DataObject = data;
      this->ActualMemorySize = data? data->GetActualMemorySize() : 0;

      vtkTimeStamp ts; ts.Modified();
      this->TimeStamp = ts;
      }

    void SetActualMemorySize(unsigned long size)
      {
      this->ActualMemorySize = size;
      }

    void SetDeliveredDataObject(vtkDataObject* data)
      {
      this->DeliveredDataObject = data;
      }

    void SetRedistributedDataObject(vtkDataObject* data)
      {
      this->RedistributedDataObject = data;
      }

    vtkDataObject* GetDeliveredDataObject()
      {
      return this->DeliveredDataObject.GetPointer();
      }

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

    vtkDataObject* GetDataObject() const
      { return this->DataObject.GetPointer(); }
    unsigned long GetTimeStamp() const
      { return this->TimeStamp; }

    unsigned long GetVisibleDataSize()
      {
      if (this->Representation && this->Representation->GetVisibility())
        {
        return this->ActualMemorySize;
        }
      return 0;
      }

    void SetNextStreamedPiece(vtkDataObject* data)
      {
      this->StreamedPiece = data;
      }
    vtkDataObject* GetStreamedPiece()
      {
      return this->StreamedPiece;
      }
    };

  typedef std::map<unsigned int, std::pair<vtkItem, vtkItem> > ItemsMapType;

  vtkItem* GetItem(unsigned int index, bool use_second)
    {
    if (this->ItemsMap.find(index) != this->ItemsMap.end())
      {
      return use_second? &(this->ItemsMap[index].second) :
        &(this->ItemsMap[index].first);
      }
    return NULL;
    }

  vtkItem* GetItem(vtkPVDataRepresentation* repr, bool use_second)
    {
    return this->GetItem(repr->GetUniqueIdentifier(), use_second);
    }

  unsigned long GetVisibleDataSize(bool use_second_if_available)
    {
    unsigned long size = 0;
    ItemsMapType::iterator iter;
    for (iter = this->ItemsMap.begin(); iter != this->ItemsMap.end(); ++iter)
      {
      if (use_second_if_available && iter->second.second.GetDataObject())
        {
        size += iter->second.second.GetVisibleDataSize();
        }
      else
        {
        size += iter->second.first.GetVisibleDataSize();
        }
      }
    return size;
    }

  ItemsMapType ItemsMap;
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
  assert( "A representation must have a valid UniqueIdentifier"
          && repr->GetUniqueIdentifier());

  vtkInternals::vtkItem item;
  item.Representation = repr;
  this->Internals->ItemsMap[repr->GetUniqueIdentifier()].first = item;

  vtkInternals::vtkItem item2;
  item2.Representation = repr;
  this->Internals->ItemsMap[repr->GetUniqueIdentifier()].second= item2;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::UnRegisterRepresentation(
  vtkPVDataRepresentation* repr)
{
  this->Internals->ItemsMap.erase(repr->GetUniqueIdentifier());
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation* vtkPVDataDeliveryManager::GetRepresentation(
  unsigned int index)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(index, false);
  return item? item->Representation : NULL;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetDeliverToAllProcesses(
  vtkPVDataRepresentation* repr, bool mode, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
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
  vtkPVDataRepresentation* repr, bool deliver_to_client,
  bool gather_before_delivery, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
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
  vtkPVDataRepresentation* repr, bool value/*=true*/)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false);
  vtkInternals::vtkItem* low_item = this->Internals->GetItem(repr, true);
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
void vtkPVDataDeliveryManager::SetStreamable(
  vtkPVDataRepresentation* repr, bool val)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false);
  vtkInternals::vtkItem* low_item = this->Internals->GetItem(repr, true);
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
void vtkPVDataDeliveryManager::SetPiece(
  vtkPVDataRepresentation* repr, vtkDataObject* data, bool low_res,
  unsigned long trueSize)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (item)
    {
    unsigned long data_time = 0;
    if (data && (data->GetMTime() > data_time))
      {
      data_time = data->GetMTime();
      }
    if (data_time > item->GetTimeStamp() ||
      item->GetDataObject() != data)
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
  vtkPVDataRepresentation* repr, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->GetProducer(
    this->RenderView->GetUseOrderedCompositing())->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetPiece(unsigned int id, vtkDataObject* data, bool
  low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res);
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
void vtkPVDataDeliveryManager::SetOrderedCompositingInformation(
  vtkPVDataRepresentation* repr, vtkExtentTranslator* translator,
  const int whole_extents[6], const double origin[3], const double spacing[3])
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false);
  if (item)
    {
    vtkInternals::vtkOrderedCompositingInfo info;
    info.Translator = translator;
    memcpy(info.WholeExtent, whole_extents, sizeof(int)*6);
    memcpy(info.Origin, origin, sizeof(double)*3);
    memcpy(info.Spacing, spacing, sizeof(double)*3);

    item->OrderedCompositingInfo = info;
    }
  else
    {
    vtkErrorMacro("Invalid argument.");
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVDataDeliveryManager::GetProducer(
  unsigned int id, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->GetProducer(
    this->RenderView->GetUseOrderedCompositing())->GetOutputPort(0);
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::NeedsDelivery(
  unsigned long timestamp,
  std::vector<unsigned int> &keys_to_deliver, bool use_low)
{
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin();
    iter != this->Internals->ItemsMap.end(); ++iter)
    {
    vtkInternals::vtkItem& item = use_low? iter->second.second : iter->second.first;
    if (item.Representation &&
      item.Representation->GetVisibility() &&
      item.GetTimeStamp() > timestamp)
      {
      keys_to_deliver.push_back(iter->first);
      }
    }
  return keys_to_deliver.size() > 0;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::Deliver(int use_lod, unsigned int size, unsigned int *values)

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

  vtkTimerLog::MarkStartEvent(use_lod?
    "LowRes Data Migration" : "FullRes Data Migration");

  bool using_remote_rendering =
    use_lod? this->RenderView->GetUseDistributedRenderingForInteractiveRender() :
    this->RenderView->GetUseDistributedRenderingForStillRender();
  int mode = this->RenderView->GetDataDistributionMode(using_remote_rendering);

  for (unsigned int cc=0; cc < size; cc++)
    {
    vtkInternals::vtkItem* item = this->Internals->GetItem(values[cc], use_lod !=0);

    vtkDataObject* data = item->GetDataObject();

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
      dataMover->SetSkipDataServerGatherToZero(
        item->GatherBeforeDeliveringToClient == false);
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

  vtkTimerLog::MarkEndEvent(use_lod?
    "LowRes Data Migration" : "FullRes Data Migration");
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::RedistributeDataForOrderedCompositing(
  bool use_lod)
{
  if (this->RenderView->GetUpdateTimeStamp() > this->RedistributionTimeStamp)
    {
    vtkTimerLog::MarkStartEvent("Regenerate Kd-Tree");
    // need to re-generate the kd-tree.
    this->RedistributionTimeStamp.Modified();

    vtkNew<vtkKdTreeManager> cutsGenerator;
    vtkInternals::ItemsMapType::iterator iter;
    for (iter = this->Internals->ItemsMap.begin();
      iter != this->Internals->ItemsMap.end(); ++iter)
      {
      vtkInternals::vtkItem& item =  iter->second.first;
      if (item.Representation &&
        item.Representation->GetVisibility())
        {
        if (item.OrderedCompositingInfo.Translator)
          {
          // implies that the representation is providing us with means to
          // override how the ordered compositing happens.
          const vtkInternals::vtkOrderedCompositingInfo &info =
            item.OrderedCompositingInfo;
          cutsGenerator->SetStructuredDataInformation(info.Translator,
            info.WholeExtent, info.Origin, info.Spacing);
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
  for (iter = this->Internals->ItemsMap.begin();
    iter != this->Internals->ItemsMap.end(); ++iter)
    {
    vtkInternals::vtkItem& item = use_lod? iter->second.second : iter->second.first;

    if (!item.Redistributable ||
      item.Representation == NULL ||
      item.Representation->GetVisibility() == false ||

      // delivered object can be null in case we're updating lod and the
      // representation doeesn't have any LOD data.
      item.GetDeliveredDataObject() == NULL)
      {
      continue;
      }

    if (item.GetRedistributedDataObject() &&

      // input-data didn't change
      (item.GetDeliveredDataObject()->GetMTime() <
       item.GetRedistributedDataObject()->GetMTime()) &&

      // kd-tree didn't change
      (item.GetRedistributedDataObject()->GetMTime() >
       this->KdTree->GetMTime()))
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
vtkPKdTree* vtkPVDataDeliveryManager::GetKdTree()
{
  return this->KdTree;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetNextStreamedPiece(
  vtkPVDataRepresentation* repr, vtkDataObject* data)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, /*low_res=*/false);
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
  vtkPVDataRepresentation* repr)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, /*low_res=*/false);
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
  for (iter = this->Internals->ItemsMap.begin();
    iter != this->Internals->ItemsMap.end(); ++iter)
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
  for (iter = this->Internals->ItemsMap.begin();
    iter != this->Internals->ItemsMap.end(); ++iter)
    {
    vtkInternals::vtkItem& item = iter->second.first;
    if (item.Representation &&
      item.Representation->GetVisibility() &&
      item.Streamable &&
      item.GetStreamedPiece())
      {
      keys.push_back(iter->first);
      }
    }
  return (keys.size() > 0);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::DeliverStreamedPieces(
  unsigned int size, unsigned int *values)
{
  // This method gets called on all processes to deliver any streamed pieces
  // currently available. This is similar to Deliver(...) except that this deals
  // with only delivering pieces for streaming. 

  bool using_remote_rendering =
    this->RenderView->GetUseDistributedRenderingForStillRender();
  int mode = this->RenderView->GetDataDistributionMode(using_remote_rendering);

  for (unsigned int cc=0; cc < size; cc++)
    {
    vtkInternals::vtkItem* item = this->Internals->GetItem(values[cc], false);

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
  const int prime = 31;
  int result = 1;
  result = prime * result + static_cast<int>(this->Internals->ItemsMap.size());
  vtkInternals::ItemsMapType::iterator iter = this->Internals->ItemsMap.begin();
  for(;iter != this->Internals->ItemsMap.end(); iter++)
    {
    result = prime * result + static_cast<int>(iter->first);
    }

  return result;
}
