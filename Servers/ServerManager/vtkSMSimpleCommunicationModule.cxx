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

#include "vtkProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSimpleCommunicationModule);
vtkCxxRevisionMacro(vtkSMSimpleCommunicationModule, "1.5");

//----------------------------------------------------------------------------
vtkSMSimpleCommunicationModule::vtkSMSimpleCommunicationModule()
{
}

//----------------------------------------------------------------------------
vtkSMSimpleCommunicationModule::~vtkSMSimpleCommunicationModule()
{
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

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (!pm->GetStreamPointer())
    {
    return;
    }

  if (serverid == 0)
    {
    pm->SendStreamToServerRootTemp(stream);
    }
  else
    {
    pm->SendStreamToServerTemp(stream);
    }

  // TODO Replace this with a generic controller where each node is the
  // root node of one server. The server manager is simple another 
  // server. The controller should take care of sending/receiving etc.

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

//---------------------------------------------------------------------------
void vtkSMSimpleCommunicationModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
