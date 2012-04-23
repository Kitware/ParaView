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
#include "vtkBSPCutsGenerator.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPKdTree.h"
#include "vtkPVRenderView.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

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
unsigned long vtkRepresentedDataStorage::GetVisibleDataSize(bool low_res)
{
  return this->Internals->GetVisibleDataSize(low_res);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::RegisterRepresentation(
  int id, vtkPVDataRepresentation* repr)
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
void vtkRepresentedDataStorage::SetPiece(int id, vtkDataObject* data, bool
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
  int id, bool low_res)
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
  std::vector<int> &keys_to_deliver, bool use_low)
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
void vtkRepresentedDataStorage::Deliver(int use_lod, unsigned int size, int *values)

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
    use_lod? this->View->GetUseDistributedRenderingForInteractiveRender() :
    this->View->GetUseDistributedRenderingForStillRender();
  int mode = this->View->GetDataDistributionMode(using_remote_rendering);


  for (unsigned int cc=0; cc < size; cc++)
    {
    vtkInternals::vtkItem* item = this->Internals->GetItem(values[cc], use_lod !=0);

    vtkDataObject* data = item->GetDataObject();

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
void vtkRepresentedDataStorage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
