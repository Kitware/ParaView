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
#include "vtkRepresentedDataStorage.h"
#include "vtkRepresentedDataStorageInternals.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAMRVolumeRepresentation.h"
#include "vtkBSPCutsGenerator.h"
#include "vtkCamera.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkOverlappingAMR.h"
#include "vtkPKdTree.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSession.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedIntArray.h"
#include "vtkAMRBox.h"
#include "vtkMath.h"

#include <assert.h>

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

vtkStandardNewMacro(vtkRepresentedDataStorage);
//----------------------------------------------------------------------------
vtkRepresentedDataStorage::vtkRepresentedDataStorage()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkRepresentedDataStorage::~vtkRepresentedDataStorage()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::SetView(vtkPVRenderView* view)
{
  this->View = view;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkRepresentedDataStorage::GetView()
{
  return this->View;
}

//----------------------------------------------------------------------------
unsigned long vtkRepresentedDataStorage::GetVisibleDataSize(bool low_res)
{
  return this->Internals->GetVisibleDataSize(low_res);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::RegisterRepresentation(
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
void vtkRepresentedDataStorage::UnRegisterRepresentation(
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
void vtkRepresentedDataStorage::SetDeliverToAllProcesses(
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
void vtkRepresentedDataStorage::MarkAsRedistributable(
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
void vtkRepresentedDataStorage::SetStreamable(
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
void vtkRepresentedDataStorage::SetPiece(
  vtkPVDataRepresentation* repr, vtkDataObject* data, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
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
vtkAlgorithmOutput* vtkRepresentedDataStorage::GetProducer(
  vtkPVDataRepresentation* repr, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->GetProducer()->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::SetPiece(unsigned int id, vtkDataObject* data, bool
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
vtkAlgorithmOutput* vtkRepresentedDataStorage::GetProducer(
  unsigned int id, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->GetProducer()->GetOutputPort(0);
}

//----------------------------------------------------------------------------
bool vtkRepresentedDataStorage::NeedsDelivery(
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
void vtkRepresentedDataStorage::Deliver(int use_lod, unsigned int size, unsigned int *values)

{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSession* activeSession = vtkPVSession::SafeDownCast(pm->GetActiveSession());
  if (activeSession && activeSession->IsMultiClients())
    {
    if (!this->View->SynchronizeForCollaboration())
      {
      return;
      }
    }

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
    use_lod? this->View->GetUseDistributedRenderingForInteractiveRender() :
    this->View->GetUseDistributedRenderingForStillRender();
  int mode = this->View->GetDataDistributionMode(using_remote_rendering);


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
    dataMover->SetInputConnection(item->GetProducer()->GetOutputPort());
    dataMover->Update();
    item->SetDataObject(dataMover->GetOutputDataObject(0));
    }

  // There's a possibility that we'd need to do ordered compositing.
  // Ask the view if we need to redistribute the data for ordered compositing.
  bool use_ordered_compositing = using_remote_rendering &&
    this->View->GetUseOrderedCompositing();

  if (use_ordered_compositing && !use_lod)
    {
    vtkNew<vtkBSPCutsGenerator> cutsGenerator;
    vtkInternals::ItemsMapType::iterator iter;
    for (iter = this->Internals->ItemsMap.begin();
      iter != this->Internals->ItemsMap.end(); ++iter)
      {
      vtkInternals::vtkItem& item =  iter->second.first;
      if (item.Representation &&
        item.Representation->GetVisibility() &&
        item.Redistributable)
        {
        cutsGenerator->AddInputData(item.GetDataObject());
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
    }
  else if (!use_lod)
    {
    this->KdTree = NULL;
    }
  // FIXME:STREAMING
  // 1. Fix code to avoid recomputing of KdTree unless really necessary.
  // 2. If KdTree is recomputed and is indeed different, then we need to
  //    redistribute all the visible "redistributable" datasets, not just the
  //    ones being requested.

  if (this->KdTree)
    {
    vtkTimerLog::MarkStartEvent("Redistributing Data for Ordered Compositing");
    for (unsigned int cc=0; cc < size; cc++)
      {
      vtkInternals::vtkItem* item = this->Internals->GetItem(values[cc], use_lod !=0);
      if (!item->Redistributable)
        {
        continue;
        }

      vtkDataObject* data = item->GetDataObject();

      vtkNew<vtkOrderedCompositeDistributor> redistributor;
      redistributor->SetController(vtkMultiProcessController::GetGlobalController());
      redistributor->SetInputData(data);
      redistributor->SetPKdTree(this->KdTree);
      redistributor->SetPassThrough(0);
      redistributor->Update();
      item->SetDataObject(redistributor->GetOutputDataObject(0));
      }
    vtkTimerLog::MarkEndEvent("Redistributing Data for Ordered Compositing");
    }


  vtkTimerLog::MarkEndEvent(use_lod?
    "LowRes Data Migration" : "FullRes Data Migration");
}

//----------------------------------------------------------------------------
vtkPKdTree* vtkRepresentedDataStorage::GetKdTree()
{
  return this->KdTree;
}

//----------------------------------------------------------------------------
bool vtkRepresentedDataStorage::BuildPriorityQueue(double planes[24])
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
  //vtkRenderer* ren = this->View->GetRenderer();
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
void vtkRepresentedDataStorage::StreamingDeliver(unsigned int key)
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
    this->View->StreamingUpdate();
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
unsigned int vtkRepresentedDataStorage::GetRepresentationIdFromQueue()
{
  if (!this->Internals->PriorityQueue.empty())
    {
    return this->Internals->PriorityQueue.top().RepresentationId;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
