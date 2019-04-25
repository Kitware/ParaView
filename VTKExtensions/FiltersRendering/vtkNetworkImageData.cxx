/*=========================================================================

  Program:   ParaView
  Module:    vtkNetworkImageData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNetworkImageData.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkNetworkImageData);

//----------------------------------------------------------------------------
vtkNetworkImageData::~vtkNetworkImageData()
{
  this->Buffer = nullptr;
}

//----------------------------------------------------------------------------
void vtkNetworkImageData::UpdateBuffer()
{
  if (this->GetMTime() < this->UpdateImageTime)
  {
    return;
  }

  this->Buffer = vtkImageData::SafeDownCast(this->GetInput());
  if (this->Buffer == nullptr)
  {
    return;
  }

  vtkPVSession* session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());
  if (!session)
  {
    vtkErrorMacro("Active session must be a vtkPVSession.");
    return;
  }

  vtkPVSession::ServerFlags roles = session->GetProcessRoles();
  if ((roles & vtkPVSession::CLIENT) != 0)
  {
    vtkMultiProcessController* rs_controller = session->GetController(vtkPVSession::RENDER_SERVER);
    if (rs_controller)
    {
      rs_controller->Send(this->Buffer, 1, 0x287823);
    }
  }
  else if ((roles & vtkPVSession::RENDER_SERVER) != 0 ||
    (roles & vtkPVSession::RENDER_SERVER_ROOT) != 0)
  {
    // receive the image from the client.
    vtkMultiProcessController* client_controller = session->GetController(vtkPVSession::CLIENT);
    if (client_controller)
    {
      client_controller->Receive(this->Buffer, 1, 0x287823);
    }
  }

  vtkMultiProcessController* globalController = vtkMultiProcessController::GetGlobalController();
  if (globalController->GetNumberOfProcesses() > 1)
  {
    globalController->Broadcast(this->Buffer, 0);
  }

  this->UpdateImageTime.Modified();
}

//----------------------------------------------------------------------------
int vtkNetworkImageData::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->Buffer->GetExtent(), 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetworkImageData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // shallow copy internal buffer to output
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->ShallowCopy(this->Buffer);
  return 1;
}
