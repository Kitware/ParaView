// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVMultiClientsInformation.h"

#include "vtkClientServerStream.h"
#include "vtkCompositeMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"

#include <algorithm>

vtkStandardNewMacro(vtkPVMultiClientsInformation);

//----------------------------------------------------------------------------
vtkPVMultiClientsInformation::vtkPVMultiClientsInformation()
{
  this->MultiClientEnable = 0;
  this->ClientIds = nullptr;
  this->ClientId = 0;
  this->MasterId = 0;
  this->NumberOfClients = 1;
}

//----------------------------------------------------------------------------
vtkPVMultiClientsInformation::~vtkPVMultiClientsInformation()
{
  if (this->ClientIds)
  {
    delete[] this->ClientIds;
    this->ClientIds = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPVMultiClientsInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Multi-client: " << this->MultiClientEnable << endl;
  os << indent << "ClientId: " << this->ClientId << endl;
  os << indent << "MasterId: " << this->MasterId << endl;
  os << indent << "NumberOfClients: " << this->NumberOfClients << endl;
  os << indent << "Client list: ";
  for (int i = 0; i < this->NumberOfClients; i++)
  {
    os << this->GetClientId(i) << " ";
  }
  os << endl;
}

//----------------------------------------------------------------------------
void vtkPVMultiClientsInformation::DeepCopy(vtkPVMultiClientsInformation* info)
{
  this->MultiClientEnable = info->MultiClientEnable;
  this->ClientId = info->GetClientId();
  this->MasterId = info->GetMasterId();
  this->NumberOfClients = info->GetNumberOfClients();
  if (this->ClientIds)
  {
    delete[] this->ClientIds;
    this->ClientIds = nullptr;
  }
  if (info->ClientIds)
  {
    this->ClientIds = new int[this->NumberOfClients];
    for (int cc = 0; cc < this->NumberOfClients; cc++)
    {
      this->ClientIds[cc] = info->GetClientId(cc);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVMultiClientsInformation::CopyFromObject(vtkObject* vtkNotUsed(obj))
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkWarningMacro("ProcessModule is not available.");
    return;
  }

  auto config = vtkRemotingCoreConfiguration::GetInstance();
  this->MultiClientEnable = config->GetMultiClientMode() ? 1 : 0;

  // Retrieve the current client connection ID
  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetSession());
  vtkCompositeMultiProcessController* ctrl;
  if (this->ClientIds)
  {
    delete[] this->ClientIds;
    this->ClientIds = nullptr;
  }
  if (session &&
    (ctrl = vtkCompositeMultiProcessController::SafeDownCast(
       session->GetController(vtkPVSession::CLIENT))))
  {
    this->ClientId = ctrl->GetActiveControllerID();
    this->MasterId = ctrl->GetMasterController();
    this->NumberOfClients = ctrl->GetNumberOfControllers();
    this->ClientIds = new int[this->NumberOfClients];
    for (int cc = 0; cc < this->NumberOfClients; cc++)
    {
      this->ClientIds[cc] = ctrl->GetControllerId(cc);
    }
  }
  else
  {
    this->ClientId = 0;
    this->MasterId = 0;
    this->NumberOfClients = 1;
    this->MultiClientEnable = false;
  }
}

//----------------------------------------------------------------------------
// Consider an option added if it is a non-default option that the user
// has probably selected.
void vtkPVMultiClientsInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVMultiClientsInformation* serverInfo;
  serverInfo = vtkPVMultiClientsInformation::SafeDownCast(info);
  if (serverInfo)
  {
    this->NumberOfClients = std::max(this->NumberOfClients, serverInfo->NumberOfClients);
    this->ClientId = std::max(this->ClientId, serverInfo->ClientId);
    this->MasterId = std::max(this->MasterId, serverInfo->MasterId);
    if (this->ClientIds == nullptr && serverInfo->ClientIds)
    {
      this->ClientIds = new int[serverInfo->NumberOfClients];
      for (int cc = 0; cc < serverInfo->NumberOfClients; cc++)
      {
        this->ClientIds[cc] = serverInfo->ClientIds[cc];
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVMultiClientsInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->MultiClientEnable;
  *css << this->ClientId;
  *css << this->MasterId;
  *css << this->NumberOfClients;
  for (int cc = 0; cc < this->NumberOfClients; cc++)
  {
    *css << this->GetClientId(cc);
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVMultiClientsInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if (!css->GetArgument(0, 0, &this->MultiClientEnable))
  {
    vtkErrorMacro("Error parsing MultiClientEnable from message.");
    return;
  }
  if (!css->GetArgument(0, 1, &this->ClientId))
  {
    vtkErrorMacro("Error parsing ClientId from message.");
    return;
  }
  if (!css->GetArgument(0, 2, &this->MasterId))
  {
    vtkErrorMacro("Error parsing MasterId from message.");
    return;
  }
  if (!css->GetArgument(0, 3, &this->NumberOfClients))
  {
    vtkErrorMacro("Error parsing NumberOfClients from message.");
    return;
  }
  if (this->ClientId != 0)
  {
    if (this->ClientIds)
    {
      delete[] this->ClientIds;
      this->ClientIds = nullptr;
    }
    this->ClientIds = new int[this->NumberOfClients];
    for (int cc = 0; cc < this->NumberOfClients; cc++)
    {
      if (!css->GetArgument(0, 4 + cc, &this->ClientIds[cc]))
      {
        vtkErrorMacro("Error parsing ClientIds from message.");
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVMultiClientsInformation::GetClientId(int idx)
{
  // Invalid internal data
  if (this->ClientIds == nullptr || !(idx < this->NumberOfClients && idx >= 0))
  {
    return this->ClientId;
  }

  return this->ClientIds[idx];
}
