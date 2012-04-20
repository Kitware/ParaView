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
#include "vtkSMDataDeliveryManager.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkRepresentedDataStorage.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMDataDeliveryManager);
//----------------------------------------------------------------------------
vtkSMDataDeliveryManager::vtkSMDataDeliveryManager():
  UpdateObserverTag(0)
{
}

//----------------------------------------------------------------------------
vtkSMDataDeliveryManager::~vtkSMDataDeliveryManager()
{
  if (this->ViewProxy && this->UpdateObserverTag)
    {
    this->ViewProxy->RemoveObserver(this->UpdateObserverTag);
    this->UpdateObserverTag = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::SetViewProxy(vtkSMViewProxy* viewproxy)
{
  if (viewproxy == this->ViewProxy)
    {
    return;
    }

  if (this->ViewProxy && this->UpdateObserverTag)
    {
    this->ViewProxy->RemoveObserver(this->UpdateObserverTag);
    this->UpdateObserverTag = 0;
    }
  this->ViewProxy = viewproxy;
  if (this->ViewProxy)
    {
    this->UpdateObserverTag = this->ViewProxy->AddObserver(
      vtkCommand::UpdateDataEvent,
      this, &vtkSMDataDeliveryManager::OnViewUpdated);
    }

  this->Modified();
  this->ViewUpdateStamp.Modified();
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::OnViewUpdated()
{
  // Update the timestamp so we realize that view was updated. Hence, we can
  // expect new geometry needs to be delivered. Otherwise that code is
  // short-circuited for better rendering performance.
  this->ViewUpdateStamp.Modified();
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::Deliver(bool interactive)
{
  assert(this->ViewProxy != NULL);

  // FIXME:STREAMING this needs to be a little bit cleaner.
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(
    this->ViewProxy->GetClientSideObject());
  bool use_lod = interactive && view->GetUseLODForInteractiveRender();

  if ( (!use_lod && this->ViewUpdateStamp < this->GeometryDeliveryStamp) ||
    (use_lod && this->ViewUpdateStamp < this->LODGeometryDeliveryStamp) )
    {
    return;
    }

  vtkTimeStamp& timeStamp = use_lod?
    this->LODGeometryDeliveryStamp : this->GeometryDeliveryStamp;

  std::vector<int> keys_to_deliver;
  if (!view->GetGeometryStore()->NeedsDelivery(timeStamp, keys_to_deliver, use_lod))
    {
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this->ViewProxy)
         << "GetGeometryStore"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "Deliver"
         << static_cast<int>(use_lod)
         << static_cast<unsigned int>(keys_to_deliver.size())
         << vtkClientServerStream::InsertArray(
           &keys_to_deliver[0], keys_to_deliver.size())
         << vtkClientServerStream::End;
  this->ViewProxy->GetSession()->ExecuteStream(
    this->ViewProxy->GetLocation(), stream, false);
  timeStamp.Modified();
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
