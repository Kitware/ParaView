/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleCommunicationModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleCommunicationModule.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformation.h"
#include "vtkSMProcessModule.h"
#include "vtkSocketController.h"

#include "vtkProcessModule.h"

#define VTK_PV_CLIENTSERVER_RMI_TAG          938531

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSimpleCommunicationModule);
vtkCxxRevisionMacro(vtkSMSimpleCommunicationModule, "1.2");

//----------------------------------------------------------------------------
vtkSMSimpleCommunicationModule::vtkSMSimpleCommunicationModule()
{
  this->SocketController = vtkSocketController::New();
}

//----------------------------------------------------------------------------
vtkSMSimpleCommunicationModule::~vtkSMSimpleCommunicationModule()
{
  this->SocketController->Delete();
}

//----------------------------------------------------------------------------
// TODO this is only a partial implementation. Have to check for duplicated
// entries in the server list etc.
void vtkSMSimpleCommunicationModule::SendStreamToServers(
  vtkClientServerStream* stream, int numServers, const int* serverIDs)
{
  for (int i=0; i<numServers; i++)
    {
    this->SendStreamToServer(stream, serverIDs[i]);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleCommunicationModule::SendStreamToServer(
  vtkClientServerStream* stream, int serverid)
{

  // TODO Replace this with a generic controller where each node is the
  // root node of one server. The server manager is simple another 
  // server. The controller should take care of sending/receiving etc.

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (!pm->GetStreamPointer())
    {
    return;
    }
  pm->GetStream() = *stream;

  if (serverid == 0)
    {
    pm->SendStreamToServerRoot();
    }
  else
    {
    pm->SendStreamToServer();
    }

//   const unsigned char* data;
//   size_t len;
//   stream->GetData(&data, &len);
//   this->SocketController->TriggerRMI(1, (void*)(data), len,
//                                      VTK_PV_CLIENTSERVER_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkSMSimpleCommunicationModule::GatherInformation(vtkPVInformation* info,
                                                       vtkClientServerID id,
                                                       int /*serverid*/)
{
  vtkProcessModule::GetProcessModule()->GatherInformation(info, id);

  // Replace this with a generic controller where each node is the
  // root node of one server. The server manager is simple another 
  // server. The controller should take care of sending/receiving etc.

//   vtkClientServerStream stream;
//   // Gather on the server.
//   stream
//     << vtkClientServerStream::Invoke
//     << this->GetProcessModuleID()
//     << "GatherInformationInternal" << info->GetClassName() << id
//     << vtkClientServerStream::End;
//   this->SendStreamToServer(&stream, serverid);

//   // Client just receives information from the server.
//   int length;
//   this->SocketController->Receive(&length, 1, 1, 398798);
//   unsigned char* data = new unsigned char[length];
//   this->SocketController->Receive(data, length, 1, 398799);
//   stream.SetData(data, length);
//   info->CopyFromStream(&stream);
//   delete [] data;
}

//----------------------------------------------------------------------------
void vtkSMSimpleCommunicationModule::Connect()
{
  this->SocketController->Initialize();
  if (!this->SocketController->ConnectTo("localhost", 11111))
    {
    vtkErrorMacro("Could not connect");
    }
  // TODO: Temporary solution
  int numServerProcs;
  this->SocketController->Receive(&numServerProcs, 1, 1, 8843);
}

//----------------------------------------------------------------------------
void vtkSMSimpleCommunicationModule::Disconnect()
{
  this->SocketController->TriggerRMI(
    1, vtkMultiProcessController::BREAK_RMI_TAG);
  this->SocketController->CloseConnection();
}

//---------------------------------------------------------------------------
void vtkSMSimpleCommunicationModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
