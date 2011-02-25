/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPICommunicator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMPICommunicator.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVSession.h"

#include "vtkMPI.h"

vtkStandardNewMacro(vtkPVMPICommunicator);
//----------------------------------------------------------------------------
vtkPVMPICommunicator::vtkPVMPICommunicator()
{
}

//----------------------------------------------------------------------------
vtkPVMPICommunicator::~vtkPVMPICommunicator()
{
}

//----------------------------------------------------------------------------
int vtkPVMPICommunicator::ReceiveDataInternal(
  char* data, int length, int sizeoftype,
  int remoteProcessId, int tag,
  vtkMPICommunicatorReceiveDataInfo* info,
  int useCopy, int& senderId)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetActiveSession());
  vtkPVProgressHandler* progressHandler =
    session? session->GetProgressHandler() : NULL;

  if (!progressHandler || this->GetLocalProcessId() != 0 ||
    this->GetNumberOfProcesses() <= 1)
    {
    return this->Superclass::ReceiveDataInternal(
      data, length, sizeoftype,
      remoteProcessId, tag, info, useCopy, senderId);
    }
  
  // Only on root node when satellites exist.

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  
  int retVal;

  Request receiveReq;
  if (!CheckForMPIError(MPI_Irecv(data, length, info->DataType, remoteProcessId, tag, 
    *(info->Handle), &receiveReq.Req->Handle)))
    {
    return 0;
    }

  progressHandler->RefreshProgress();
  int index = -1;
  do
    {
    MPI_Request requests[2];
    requests[0] = receiveReq.Req->Handle;
    int num_requests = 1;
    vtkMPICommunicatorOpaqueRequest* asyncReq =
      progressHandler->GetAsyncRequest();
    if (asyncReq)
      {
      requests[1] = asyncReq->Handle;
      num_requests = 2;
      }
    retVal = MPI_Waitany(num_requests, requests, &index, &(info->Status));
    if (!CheckForMPIError(retVal))
      {
      receiveReq.Cancel();
      return 0;
      }
    if (index == 1)
      {
      // MPI_Waitany destroys the successful request object. Now the
      // progressHandler cannot re-test the request. Hence we force it to
      // pretend that the request has indeed been received without actually
      // testing it.
      // Received the progress request, handle it.
      // This will also set up a new request for the progress.
      progressHandler->MarkAsyncRequestReceived();
      progressHandler->RefreshProgress();
      }
    } while (index != 0);

  if (retVal == MPI_SUCCESS)
    {
    senderId = info->Status.MPI_SOURCE;
    }
  return retVal;
}


//----------------------------------------------------------------------------
void vtkPVMPICommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


