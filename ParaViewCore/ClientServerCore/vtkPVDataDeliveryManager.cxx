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
#include "vtkAMRBox.h"
#include "vtkAMRVolumeRepresentation.h"
#include "vtkBSPCutsGenerator.h"
#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkMath.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkOverlappingAMR.h"
#include "vtkPKdTree.h"
#include "vtkPointData.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedIntArray.h"
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

    unsigned long TimeStamp;
    unsigned long ActualMemorySize;
  public:
    vtkWeakPointer<vtkAlgorithmOutput> ImageDataProducer;
      // <-- HACK for image data volume rendering.

    vtkWeakPointer<vtkPVDataRepresentation> Representation;
    bool AlwaysClone;
    bool Redistributable;
    bool Streamable;

    vtkItem() :
      Producer(vtkSmartPointer<vtkPVTrivialProducer>::New()),
      TimeStamp(0),
      ActualMemorySize(0),
      AlwaysClone(false),
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
    RepresentationToIdMapType::iterator iter =
      this->RepresentationToIdMap.find(repr);
    if (iter != this->RepresentationToIdMap.end())
      {
      unsigned int index = iter->second;
      return use_second? &(this->ItemsMap[index].second) :
        &(this->ItemsMap[index].first);
      }

    return NULL;
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

  typedef std::map<vtkPVDataRepresentation*, unsigned int>
    RepresentationToIdMapType;

  RepresentationToIdMapType RepresentationToIdMap;
  ItemsMapType ItemsMap;
};
//*****************************************************************************
namespace
{
  // this code is stolen from vtkFrustumCoverageCuller.
  double vtkComputeScreenCoverage(const double planes[24],
    const double bounds[6], double &distance)
    {
    distance = 0.0;

    // a duff dataset like a polydata with no cells will have bad bounds
    if (!vtkMath::AreBoundsInitialized(const_cast<double*>(bounds)))
      {
      return 0.0;
      }
    double screen_bounds[4];

    double center[3];
    center[0] = (bounds[0] + bounds[1]) / 2.0;
    center[1] = (bounds[2] + bounds[3]) / 2.0;
    center[2] = (bounds[4] + bounds[5]) / 2.0;
    double radius = 0.5 * sqrt(
      ( bounds[1] - bounds[0] ) * ( bounds[1] - bounds[0] ) +
      ( bounds[3] - bounds[2] ) * ( bounds[3] - bounds[2] ) +
      ( bounds[5] - bounds[4] ) * ( bounds[5] - bounds[4] ) );
    for (int i = 0; i < 6; i++ )
      {
      // Compute how far the center of the sphere is from this plane
      double d = planes[i*4 + 0] * center[0] +
        planes[i*4 + 1] * center[1] +
        planes[i*4 + 2] * center[2] +
        planes[i*4 + 3];
      // If d < -radius the prop is not within the view frustum
      if ( d < -radius )
        {
        return 0.0;
        }

      // The first four planes are the ones bounding the edges of the
      // view plane (the last two are the near and far planes) The
      // distance from the edge of the sphere to these planes is stored
      // to compute coverage.
      if ( i < 4 )
        {
        screen_bounds[i] = d - radius;
        }
      // The fifth plane is the near plane - use the distance to
      // the center (d) as the value to sort by
      if ( i == 4 )
        {
        distance = d;
        }
      }

    // If the prop wasn't culled during the plane tests...
    // Compute the width and height of this slice through the
    // view frustum that contains the center of the sphere
    double full_w = screen_bounds[0] + screen_bounds[1] + 2.0 * radius;
    double full_h = screen_bounds[2] + screen_bounds[3] + 2.0 * radius;
    // Subtract from the full width to get the width of the square
    // enclosing the circle slice from the sphere in the plane
    // through the center of the sphere. If the screen bounds for
    // the left and right planes (0,1) are greater than zero, then
    // the edge of the sphere was a positive distance away from the
    // plane, so there is a gap between the edge of the plane and
    // the edge of the box.
    double part_w = full_w;
    if ( screen_bounds[0] > 0.0 )
      {
      part_w -= screen_bounds[0];
      }
    if ( screen_bounds[1] > 0.0 )
      {
      part_w -= screen_bounds[1];
      }
    // Do the same thing for the height with the top and bottom
    // planes (2,3).
    double part_h = full_h;
    if ( screen_bounds[2] > 0.0 )
      {
      part_h -= screen_bounds[2];
      }
    if ( screen_bounds[3] > 0.0 )
      {
      part_h -= screen_bounds[3];
      }

    // Compute the fraction of coverage
    if ((full_w * full_h)!=0.0)
      {
      return (part_w * part_h) / (full_w * full_h);
      }

    return 0;
    }
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
void vtkPVDataDeliveryManager::RegisterRepresentation(
  unsigned int id, vtkPVDataRepresentation* repr)
{
  this->Internals->RepresentationToIdMap[repr] = id;

  vtkInternals::vtkItem item;
  item.Representation = repr;
  this->Internals->ItemsMap[id].first = item;

  vtkInternals::vtkItem item2;
  item2.Representation = repr;
  this->Internals->ItemsMap[id].second= item2;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::UnRegisterRepresentation(
  vtkPVDataRepresentation* repr)
{
  vtkInternals::RepresentationToIdMapType::iterator iter =
    this->Internals->RepresentationToIdMap.find(repr);
  if (iter == this->Internals->RepresentationToIdMap.end())
    {
    vtkErrorMacro("Invalid argument.");
    return;
    }
  this->Internals->ItemsMap.erase(iter->second);
  this->Internals->RepresentationToIdMap.erase(iter);
}

//----------------------------------------------------------------------------
unsigned int vtkPVDataDeliveryManager::GetRepresentationId(
  vtkPVDataRepresentation* repr)
{
  vtkInternals::RepresentationToIdMapType::iterator iter =
    this->Internals->RepresentationToIdMap.find(repr);
  if (iter == this->Internals->RepresentationToIdMap.end())
    {
    vtkErrorMacro("Invalid argument.");
    return 0;
    }
  return iter->second;
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
    item->AlwaysClone = mode;
    }
  else
    {
    vtkErrorMacro("Invalid argument.");
    }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::MarkAsRedistributable(
  vtkPVDataRepresentation* repr)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false);
  vtkInternals::vtkItem* low_item = this->Internals->GetItem(repr, true);
  if (item)
    {
    item->Redistributable = true;
    low_item->Redistributable = true;
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
  if (!repr->IsA("vtkAMRVolumeRepresentation") && val == true)
    {
    vtkWarningMacro(
      "Only vtkAMRVolumeRepresentation streaming is currently supported.");
    return;
    }

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
  vtkPVDataRepresentation* repr, vtkDataObject* data, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (item)
    {
    vtkPVDataRepresentationPipeline* executive =
      vtkPVDataRepresentationPipeline::SafeDownCast(repr->GetExecutive());

    // SetPiece() is called in every REQUEST_UPDATE() or REQUEST_UPDATE_LOD()
    // pass irrespective of whether the data has actually changed. 
    // (I think that's a mistake, but the fact that representations can be
    // updated without view makes it tricky since we cannot set the data to
    // deliver in vtkPVDataRepresentation::RequestData() easily). Hence we need
    // to ensure that the data we are getting is newer than what we have.
    unsigned long data_time = executive? executive->GetDataTime() : 0;
    if (data && (data->GetMTime() > data_time))
      {
      data_time = data->GetMTime();
      }
    if (data_time > item->GetTimeStamp())
      {
      item->SetDataObject(data);
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
void vtkPVDataDeliveryManager::SetImageDataProducer(
  vtkPVDataRepresentation* repr, vtkAlgorithmOutput *producer)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, false);
  if (item)
    {
    item->ImageDataProducer = producer;
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

    if (data->IsA("vtkUniformGridAMR"))
      {
      // we are dealing with AMR datasets.
      // We assume for now we're not running in render-server mode. We can
      // ensure that at some point in future. 
      // So we are either in pass-through or collect mode.

      // FIXME: check that the mode flags are "suitable" for AMR.
      }
 
    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(data->GetDataObjectType());
    dataMover->SetMoveMode(mode);
    if (item->AlwaysClone)
      {
      dataMover->SetMoveModeToClone();
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

    vtkNew<vtkBSPCutsGenerator> cutsGenerator;
    vtkInternals::ItemsMapType::iterator iter;
    for (iter = this->Internals->ItemsMap.begin();
      iter != this->Internals->ItemsMap.end(); ++iter)
      {
      vtkInternals::vtkItem& item =  iter->second.first;
      if (item.Representation &&
        item.Representation->GetVisibility())
        {
        if (item.Redistributable)
          {
          cutsGenerator->AddInputData(item.GetDeliveredDataObject());
          }
        else if (item.ImageDataProducer)
          {
          cutsGenerator->AddInputConnection(item.ImageDataProducer);
          }
        }
      }

    vtkMultiProcessController* controller =
      vtkMultiProcessController::GetGlobalController();
    vtkStreamingDemandDrivenPipeline *sddp = vtkStreamingDemandDrivenPipeline::
      SafeDownCast(cutsGenerator->GetExecutive());
    sddp->SetUpdateExtent
      (0,controller->GetLocalProcessId(),controller->GetNumberOfProcesses(),0);
    sddp->Update(0);

    this->KdTree = cutsGenerator->GetPKdTree();
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
bool vtkPVDataDeliveryManager::BuildPriorityQueue(double planes[24])
{
  cout << "BuildPriorityQueue" << endl;
  // just find the first visible AMR dataset and build priority queue for that
  // dataset alone. In future, we can fix it to use information provided by
  // representation indicating if streaming is possible (since not every AMR
  // source is streambale).


  this->Internals->PriorityQueue = vtkInternals::PriorityQueueType();

  vtkOverlappingAMR* oamr = NULL;

  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin();
    iter != this->Internals->ItemsMap.end(); ++iter)
    {
    vtkInternals::vtkItem& item = iter->second.first;
    if (item.Representation &&
      item.Representation->GetVisibility() &&
      item.Streamable &&
      item.GetDataObject() &&
      item.GetDataObject()->IsA("vtkOverlappingAMR"))
      {
      oamr = vtkOverlappingAMR::SafeDownCast(item.GetDataObject());
      break;
      }
    }
  if (oamr == NULL)
    {
    return true;
    }

  // note: amr block ids currently don't match up with composite dataset ids :/.
  // now build a priority queue for absent blocks using the current camera.

  //double planes[24];
  //vtkRenderer* ren = this->RenderView->GetRenderer();
  //ren->GetActiveCamera()->GetFrustumPlanes(
  //  ren->GetTiledAspectRatio(), planes);

  for (unsigned int level=0; level < oamr->GetNumberOfLevels(); level++)
    {
    for (unsigned int index=0;
      index < oamr->GetNumberOfDataSets(level); index++)
      {
      if (oamr->GetDataSet(level, index) == NULL)
        {
        vtkAMRBox amrBox;
        if (!oamr->GetMetaData(level, index, amrBox))
          {
          vtkWarningMacro("Missing AMRBox meta-data for "
            << level << ", " << index);
          continue;
          }

        vtkInternals::vtkPriorityQueueItem item;
        item.RepresentationId = iter->first;
        item.BlockId = oamr->GetCompositeIndex(level, index);
        item.Level = level;
        item.Index = index;

        double bounds[6];
        amrBox.GetBounds(bounds);
        double depth = 1.0;
        double coverage = vtkComputeScreenCoverage(planes, bounds, depth);
        //cout << level <<"," << index << "(" << item.BlockId << ")" << " = " << coverage << ", " << depth << endl;
        if (coverage == 0.0)
          {
          // skip blocks that are not covered in the view.
          continue;
          }

        item.Priority = coverage / (depth > 1? depth : 1.0);
        this->Internals->PriorityQueue.push(item);
        }
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::StreamingDeliver(unsigned int key)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(key, false);
  vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(item->GetDataObject());
  if (!oamr)
    {
    cout << "ERROR: StreamingDeliver can only deliver AMR datasets for now" << endl;
    abort();
    }

  if (this->Internals->PriorityQueue.empty())
    {
    // client side.
    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(VTK_MULTIBLOCK_DATA_SET);
    dataMover->SetMoveModeToCollect();
    dataMover->SetInputConnection(NULL);
    dataMover->Update();

    // now put the piece in right place.
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(
      dataMover->GetOutputDataObject(0));

    if (mb->GetNumberOfBlocks() == 1 && mb->GetBlock(0) != NULL)
      {
      vtkNew<vtkUniformGrid> ug;
      ug->ShallowCopy(mb->GetBlock(0));

      vtkUnsignedIntArray* array = vtkUnsignedIntArray::SafeDownCast(
        ug->GetPointData()->GetArray("AMRIndex"));
      unsigned int index, level;
      level = array->GetValue(0);
      index = array->GetValue(1);
      ug->GetPointData()->RemoveArray("AMRIndex");
      oamr->SetDataSet(level, index, ug.GetPointer());
      oamr->Modified();
      }
    else
      {
      vtkWarningMacro("Empty data (or incorrect data) received");
      }
    }
  else
    {
    vtkInternals::vtkPriorityQueueItem qitem =
      this->Internals->PriorityQueue.top();
    this->Internals->PriorityQueue.pop();
    assert(qitem.RepresentationId == key);

    vtkAMRVolumeRepresentation* repr = vtkAMRVolumeRepresentation::SafeDownCast(
      item->Representation);
    repr->SetStreamingBlockId(qitem.BlockId);
    this->RenderView->StreamingUpdate();
    repr->ResetStreamingBlockId();

    oamr = vtkOverlappingAMR::SafeDownCast(item->GetDataObject());

    vtkNew<vtkMultiBlockDataSet> mb;

    vtkUniformGrid* piece = oamr->GetDataSet(qitem.Level, qitem.Index);
    if (piece == NULL)
      {
      vtkWarningMacro("Null piece delivered!");
      }
    else
      {
      vtkNew<vtkImageData> img;
      img->ShallowCopy(piece);
      mb->SetBlock(0, img.GetPointer());

      vtkNew<vtkUnsignedIntArray> indexArray;
      indexArray->SetName("AMRIndex");
      indexArray->SetNumberOfComponents(2);
      indexArray->SetNumberOfTuples(1);
      indexArray->SetValue(0, qitem.Level);
      indexArray->SetValue(1, qitem.Index);

      // this should be added in field data, however
      // vtkStructuredPointsReader/Writer get confused when field data is
      // present. So adding this as point data for now.
      img->GetPointData()->AddArray(indexArray.GetPointer());
      }

    // the one bad thing about this is that we are sending the full amr
    // meta-data again. We can do better here and not send it again.
    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(VTK_MULTIBLOCK_DATA_SET);
    dataMover->SetMoveModeToCollect();
    dataMover->SetInputData(mb.GetPointer());
    dataMover->Update();
    }
}

//----------------------------------------------------------------------------
unsigned int vtkPVDataDeliveryManager::GetRepresentationIdFromQueue()
{
  if (!this->Internals->PriorityQueue.empty())
    {
    return this->Internals->PriorityQueue.top().RepresentationId;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
