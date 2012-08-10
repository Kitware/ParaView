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
#include "vtkObjectFactory.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVRenderView.h"
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

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(
    this->ViewProxy->GetClientSideObject());
  bool use_lod = interactive && view->GetUseLODForInteractiveRender();
  bool use_distributed_rendering =
    interactive? view->GetUseDistributedRenderingForInteractiveRender():
    view->GetUseDistributedRenderingForStillRender();

  unsigned long update_ts = view->GetUpdateTimeStamp();
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

  //cout << "Request Delivery: " <<  keys_to_deliver.size() << endl;
  vtkTimerLog::MarkStartEvent("vtkSMDataDeliveryManager: Deliver Geometry");
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this->ViewProxy)
         << "Deliver"
         << static_cast<int>(use_lod)
         << static_cast<unsigned int>(keys_to_deliver.size())
         << vtkClientServerStream::InsertArray(
           &keys_to_deliver[0], keys_to_deliver.size())
         << vtkClientServerStream::End;
  this->ViewProxy->GetSession()->ExecuteStream(
    this->ViewProxy->GetLocation(), stream, false);
  timeStamp.Modified();
  vtkTimerLog::MarkEndEvent("vtkSMDataDeliveryManager: Deliver Geometry");
}

//----------------------------------------------------------------------------
bool vtkSMDataDeliveryManager::DeliverNextPiece()
{
  // This method gets called to deliver next piece in queue during streaming.
  // vtkSMDataDeliveryManager::Deliver() relies on the "client" to decide what
  // representations are dirty and requests geometries for those. This makes it
  // possible to work well in multi-clients mode without putting any additional
  // burden on the server side.
  //
  // For streaming, however, we require that multi-clients mode is disabled and
  // the server decides what pieces need to be delivered since the server has
  // more information about the data to decide how it should be streamed.

  vtkTimerLog::MarkStartEvent("DeliverNextPiece");
  vtkSMSession* session = this->ViewProxy->GetSession();

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(
    this->ViewProxy->GetClientSideObject());

  double planes[24];
  vtkRenderer* ren = view->GetRenderer();
  ren->GetActiveCamera()->GetFrustumPlanes(
    ren->GetTiledAspectRatio(), planes);

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this->ViewProxy)
         << "GetNextPieceToDeliver"
         << vtkClientServerStream::InsertArray(planes, 24)
         << vtkClientServerStream::End;
  session->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream, false);

  const vtkClientServerStream& result = session->GetLastResult(
    vtkPVSession::DATA_SERVER_ROOT);
  //result.Print(cout);

  // extract the "piece-key" from the result and then request it.
  unsigned int representation_id = 0;
  result.GetArgument(0, 0, &representation_id);
  if (representation_id != 0)
    {
    // we have something to deliver.
    vtkClientServerStream stream2;
    stream2 << vtkClientServerStream::Invoke
           << VTKOBJECT(this->ViewProxy)
           << "GetDeliveryManager"
           << vtkClientServerStream::End;
    stream2 << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult
           << "StreamingDeliver"
           << representation_id
           << vtkClientServerStream::End;
    this->ViewProxy->GetSession()->ExecuteStream(
      this->ViewProxy->GetLocation(), stream2, false);
    }
  vtkTimerLog::MarkEndEvent("DeliverNextPiece");
  return representation_id != 0;
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
