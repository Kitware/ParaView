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

#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingPiecesInformation.h"
#include "vtkRenderer.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"
#include "vtkTimerLog.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMDataDeliveryManager);
//----------------------------------------------------------------------------
vtkSMDataDeliveryManager::vtkSMDataDeliveryManager()
{
}

//----------------------------------------------------------------------------
vtkSMDataDeliveryManager::~vtkSMDataDeliveryManager()
{
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::SetViewProxy(vtkSMViewProxy* viewproxy)
{
  if (viewproxy != this->ViewProxy)
  {
    this->ViewProxy = viewproxy;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::Deliver(bool interactive)
{
  assert(this->ViewProxy != NULL);

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->ViewProxy->GetClientSideObject());
  bool use_lod = interactive && view->GetUseLODForInteractiveRender();
  bool use_distributed_rendering = interactive
    ? view->GetUseDistributedRenderingForInteractiveRender()
    : view->GetUseDistributedRenderingForStillRender();

  vtkMTimeType update_ts = view->GetUpdateTimeStamp();
  int delivery_type = LOCAL_RENDERING_AND_FULL_RES;
  if (!use_lod && use_distributed_rendering)
  {
    delivery_type = REMOTE_RENDERING_AND_FULL_RES;
  }
  else if (use_lod && !use_distributed_rendering)
  {
    delivery_type = LOCAL_RENDERING_AND_LOW_RES;
  }
  else if (use_lod && use_distributed_rendering)
  {
    delivery_type = REMOTE_RENDERING_AND_LOW_RES;
  }

  vtkTimeStamp& timeStamp = this->DeliveryTimestamps[delivery_type];
  if (timeStamp > update_ts)
  {
    // we delivered the data since the update. Nothing delivery to be done.
    return;
  }

  std::vector<unsigned int> keys_to_deliver;
  if (!view->GetDeliveryManager()->NeedsDelivery(timeStamp, keys_to_deliver, use_lod))
  {
    timeStamp.Modified();
    return;
  }

  // cout << "Request Delivery: " <<  keys_to_deliver.size() << endl;
  vtkTimerLog::MarkStartEvent("vtkSMDataDeliveryManager: Deliver Geometry");
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this->ViewProxy) << "Deliver"
         << static_cast<int>(use_lod) << static_cast<unsigned int>(keys_to_deliver.size())
         << vtkClientServerStream::InsertArray(
              &keys_to_deliver[0], static_cast<int>(keys_to_deliver.size()))
         << vtkClientServerStream::End;
  this->ViewProxy->GetSession()->ExecuteStream(this->ViewProxy->GetLocation(), stream, false);
  timeStamp.Modified();
  vtkTimerLog::MarkEndEvent("vtkSMDataDeliveryManager: Deliver Geometry");
}

//----------------------------------------------------------------------------
bool vtkSMDataDeliveryManager::DeliverStreamedPieces()
{
  // Deliver() relies on the client telling the server about the representation
  // that need data from the server-side. Deliver() can reliably determine that
  // since representation objects on client side are indeed updated correctly
  // when data changes (that's to the update-suppressing behaviour of
  // vtkPVDataRepresentation subclasses).

  // However for streaming, only the server-side representations update, hence
  // client has no information about representations that have pieces to stream.
  // Hence we do the following:
  // 1. Ask data-server information about what representations have some pieces
  //    reader.
  // 2. Request streamed pieces for those representations.

  vtkTimerLog::MarkStartEvent("vtkSMDataDeliveryManager: Deliver Geometry (streaming)");

  vtkNew<vtkPVStreamingPiecesInformation> info;
  this->ViewProxy->GatherInformation(info.GetPointer(), vtkPVSession::DATA_SERVER);

  std::vector<unsigned int> keys_to_deliver;
  info->GetKeys(keys_to_deliver);
  if (keys_to_deliver.size() != 0)
  {
    vtkSMSession* session = this->ViewProxy->GetSession();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this->ViewProxy) << "DeliverStreamedPieces"
           << static_cast<unsigned int>(keys_to_deliver.size())
           << vtkClientServerStream::InsertArray(
                &keys_to_deliver[0], static_cast<int>(keys_to_deliver.size()))
           << vtkClientServerStream::End;
    session->ExecuteStream(this->ViewProxy->GetLocation(), stream, false);
  }

  vtkTimerLog::MarkEndEvent("vtkSMDataDeliveryManager: Deliver Geometry (streaming)");
  return keys_to_deliver.size() > 0;
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
