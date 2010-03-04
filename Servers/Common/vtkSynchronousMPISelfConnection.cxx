/*=========================================================================

  Program:   ParaView
  Module:    vtkSynchronousMPISelfConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSynchronousMPISelfConnection.h"

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkPVInformation.h"
#include "vtkPVProgressHandler.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkSynchronousMPISelfConnection);
vtkCxxRevisionMacro(vtkSynchronousMPISelfConnection, "1.4");
//----------------------------------------------------------------------------
vtkSynchronousMPISelfConnection::vtkSynchronousMPISelfConnection()
{
}

//----------------------------------------------------------------------------
vtkSynchronousMPISelfConnection::~vtkSynchronousMPISelfConnection()
{
}

//----------------------------------------------------------------------------
void vtkSynchronousMPISelfConnection::Finalize()
{
  this->vtkSelfConnection::Finalize();
}

//-----------------------------------------------------------------------------
int vtkSynchronousMPISelfConnection::Initialize(
  int argc, char** argv, int *partitionId)
{
  int retVal = this->Superclass::Initialize(argc, argv, partitionId);
  this->ProgressHandler->SetConnection(0);
  return retVal;
}

//----------------------------------------------------------------------------
int vtkSynchronousMPISelfConnection::InitializeSatellite(
  int vtkNotUsed(argc), char** vtkNotUsed(argv))
{
  this->RegisterSatelliteRMIs();

  // Skip calling ProcessRMIs().
  return 0;
}

//----------------------------------------------------------------------------
void vtkSynchronousMPISelfConnection::SendStreamToServerNodeInternal(
  int vtkNotUsed(remoteId), vtkClientServerStream& stream)
{
  // Every processes all streams locally.
  // Logic being as follows:
  // * Everyone processes RootOnly streams since all processes act as Root as
  //   far as streams/proxies go.
  // * Everyone processes AllNodes streams since the same stream in constructed
  //   on all nodes hence no need to broadcast or anything.
  this->ProcessStreamLocally(stream);
}

//----------------------------------------------------------------------------
void vtkSynchronousMPISelfConnection::GatherInformation(vtkTypeUInt32 serverFlags, 
  vtkPVInformation* info, vtkClientServerID id)
{
  if (info->GetRootOnly() || this->GetNumberOfPartitions() == 1)
    {
    this->vtkSelfConnection::GatherInformation(serverFlags, info, id);
    return;
    }

  if (this->GetPartitionId() > 0)
    {
    this->Controller->ProcessRMIs();

    // * Receive info from root. 
    int length;
    this->Controller->Broadcast(&length, 1, 0);
    unsigned char* data = new unsigned char[length];
    this->Controller->Broadcast(const_cast<unsigned char*>(data), length, 0);
    vtkClientServerStream stream;
    stream.SetData(data, length);
    info->CopyFromStream(&stream);
    delete [] data; 
    }
  else
    {
    this->Superclass::GatherInformation(serverFlags, info, id);

    this->Controller->TriggerRMIOnAllChildren(
      vtkMultiProcessController::BREAK_RMI_TAG);

    // * Send info to everyone.
    if (info)
      {
      vtkClientServerStream css;
      info->CopyToStream(&css);
      size_t length;
      const unsigned char* data;
      css.GetData(&data, &length);
      int len = static_cast<int>(length);
      this->Controller->Broadcast(&len, 1, 0);
      this->Controller->Broadcast(const_cast<unsigned char*>(data),
        length, 0);
      }
    else
      {
      int len = 0; 
      this->Controller->Broadcast(&len, 1, 0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSynchronousMPISelfConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


