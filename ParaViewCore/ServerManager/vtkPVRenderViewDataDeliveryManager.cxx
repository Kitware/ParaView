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

#include "vtkAlgorithmOutput.h"
#include "vtkClientServerStream.h"
#include "vtkMPIMoveData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkRepresentedDataStorage.h"
#include "vtkRepresentedDataStorageInternals.h"
#include "vtkSMDataDeliveryManagerProxy.h"
#include "vtkSMSession.h"

#include <vector>

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
void vtkPVRenderViewDataDeliveryManager::SetView(vtkPVView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (!rview && view)
    {
    return;
    }

  this->Superclass::SetView(view);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::OnViewUpdated()
{
  this->ViewUpdateStamp.Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::Deliver(
  vtkSMDataDeliveryManagerProxy* proxy, bool interactive)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetView());
  bool use_lod = interactive && view->GetUseLODForInteractiveRender();

  if ( (!use_lod && this->ViewUpdateStamp < this->GeometryDeliveryStamp) ||
    (use_lod && this->ViewUpdateStamp < this->LODGeometryDeliveryStamp) )
    {
    return;
    }

  vtkTimeStamp& timeStamp = use_lod?
    this->LODGeometryDeliveryStamp : this->GeometryDeliveryStamp;

  // based on the "rendering mode", we decide where the data is being delivered.
  vtkRepresentedDataStorage::vtkInternals* map = use_lod?
    view->GetLODGeometryStore()->GetInternals() :
    view->GetGeometryStore()->GetInternals();

  std::vector<int> keys_to_deliver;

  for (vtkRepresentedDataStorage::vtkInternals::ItemsMapType::iterator iter=
    map->ItemsMap.begin(); iter != map->ItemsMap.end(); ++iter)
    {
    if (iter->second.Representation &&
      iter->second.Representation->GetVisibility() &&
      iter->second.GetDataObject() &&
      iter->second.GetTimeStamp() > timeStamp)
      {
      keys_to_deliver.push_back(iter->first);
      }
    }

  if (keys_to_deliver.size() > 0)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(proxy)
           << "Deliver"
           << static_cast<int>(use_lod)
           << static_cast<unsigned int>(keys_to_deliver.size())
           << vtkClientServerStream::InsertArray(
             &keys_to_deliver[0], keys_to_deliver.size())
           << vtkClientServerStream::End;
    proxy->GetSession()->ExecuteStream(
      proxy->GetLocation(), stream, false);
    }
  timeStamp.Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::Deliver(
  int use_lod, unsigned int size, int *values)
{
  cout << "Delivering : " << size << endl;
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetView());
  vtkRepresentedDataStorage* storage =
    use_lod? view->GetLODGeometryStore() : view->GetGeometryStore();

  for (unsigned int cc=0; cc < size; cc++)
    {
    int id = values[cc];
    vtkAlgorithmOutput* output = storage->GetProducer(id);
    vtkDataObject* data = output->GetProducer()->GetOutputDataObject(
      output->GetIndex());

    vtkNew<vtkMPIMoveData> dataMover;
    dataMover->InitializeForCommunicationForParaView();
    dataMover->SetOutputDataType(data->GetDataObjectType());
    dataMover->SetMoveModeToCollect();
    dataMover->SetInputConnection(output);
    dataMover->Update();

    if (dataMover->GetServer() == 0)
      {
      storage->SetPiece(id, dataMover->GetOutputDataObject(0));
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
