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
#include "vtkPVLogger.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingPiecesInformation.h"
#include "vtkRenderer.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"

#include <cassert>

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
  const bool use_lod = interactive && view->GetUseLODForInteractiveRender();
  const bool use_distributed_rendering = use_lod ? view->GetUseDistributedRenderingForLODRender()
                                                 : view->GetUseDistributedRenderingForRender();

  const int data_distribution_mode = view->GetDataDistributionMode(use_distributed_rendering);
  vtkMTimeType update_ts = view->GetUpdateTimeStamp();

  // note: this will create new vtkTimeStamp, if needed.
  vtkTimeStamp& timeStamp = use_lod ? this->DeliveryTimestampsLOD[data_distribution_mode]
                                    : this->DeliveryTimestamps[data_distribution_mode];

  if (timeStamp > update_ts)
  {
    // we have delivered the data since the last update on this view for the
    // chosen data delivery mode. No delivery needs to be done at this time.
    return;
  }

  // Get a list of representations for which we need to delivery data.
  std::vector<unsigned int> keys_to_deliver;
  if (!view->GetDeliveryManager()->NeedsDelivery(timeStamp, keys_to_deliver, interactive))
  {
    timeStamp.Modified();
    return;
  }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this->ViewProxy) << "Deliver"
         << static_cast<int>(use_lod) << static_cast<unsigned int>(keys_to_deliver.size())
         << vtkClientServerStream::InsertArray(
              &keys_to_deliver[0], static_cast<int>(keys_to_deliver.size()))
         << vtkClientServerStream::End;
  this->ViewProxy->GetSession()->ExecuteStream(this->ViewProxy->GetLocation(), stream, false);
  timeStamp.Modified();
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

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "deliver geometry (for streaming)");

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

  return keys_to_deliver.size() > 0;
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
