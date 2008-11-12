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

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkPVProgressHandler.h"
#include "vtkProcessModule.h"

#include "vtkMPI.h"

vtkStandardNewMacro(vtkPVMPICommunicator);
vtkCxxRevisionMacro(vtkPVMPICommunicator, "1.1");
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
  vtkPVProgressHandler* progressHandler =
    pm? pm->GetActiveProgressHandler() : 0;

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
    requests[1] = progressHandler->GetAsyncRequest()->Handle;
    if (!CheckForMPIError(MPI_Waitany(2, requests, &index, &(info->Status))))
      {
      receiveReq.Cancel();
      return 0;
      }
    if (index == 1)
      {
      // Received the progress request, handle it.
      // This will also set up a new request for the progress.
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


