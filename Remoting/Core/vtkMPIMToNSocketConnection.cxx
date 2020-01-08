/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIMToNSocketConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIMToNSocketConnection.h"

#include "vtkClientSocket.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVServerOptions.h"
#include "vtkProcessModule.h"
#include "vtkServerSocket.h"
#include "vtkSocketCommunicator.h"

#include <assert.h>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkMPIMToNSocketConnection);

vtkCxxSetObjectMacro(vtkMPIMToNSocketConnection, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIMToNSocketConnection, SocketCommunicator, vtkSocketCommunicator);
class vtkMPIMToNSocketConnectionInternals
{
public:
  struct NodeInformation
  {
    int PortNumber;
    std::string HostName;
  };
  std::vector<NodeInformation> ServerInformation;

  std::string SelfHostName;
};

vtkMPIMToNSocketConnection::vtkMPIMToNSocketConnection()
{
  this->Socket = 0;
  this->PortNumber = 0;
  this->Internals = new vtkMPIMToNSocketConnectionInternals;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SocketCommunicator = 0;
  this->NumberOfConnections = -1;
  this->ServerSocket = 0;
  this->IsWaiting = false;
}

vtkMPIMToNSocketConnection::~vtkMPIMToNSocketConnection()
{
  if (this->ServerSocket)
  {
    this->ServerSocket->Delete();
    this->ServerSocket = 0;
  }
  if (this->SocketCommunicator)
  {
    this->SocketCommunicator->CloseConnection();
    this->SocketCommunicator->Delete();
  }
  this->SetController(0);
  delete this->Internals;
  this->Internals = 0;
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SelfHostName: " << this->Internals->SelfHostName.c_str() << endl;

  os << indent << "NumberOfConnections: (" << this->NumberOfConnections << ")\n";
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "Socket: (" << this->Socket << ")\n";
  os << indent << "SocketCommunicator: (" << this->SocketCommunicator << ")\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (unsigned int i = 0; i < this->Internals->ServerInformation.size(); ++i)
  {
    os << i2 << "Server Information for Process: " << i << ":\n";
    vtkIndent i3 = i2.GetNextIndent();
    os << i3 << "PortNumber: " << this->Internals->ServerInformation[i].PortNumber << "\n";
    os << i3 << "HostName: " << this->Internals->ServerInformation[i].HostName.c_str() << "\n";
  }
  os << indent << "PortNumber: " << this->PortNumber << endl;
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::Initialize(int waiting_process_type)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();
  assert(options != NULL);

  this->SetController(pm->GetGlobalController());
  this->Internals->SelfHostName = options->GetHostName();

  this->IsWaiting = (waiting_process_type == pm->GetProcessType());
  if (this->IsWaiting)
  {
    this->SetupWaitForConnection();
  }
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::ConnectMtoN()
{
  cout << "ConnectMtoN" << endl;
  if (this->IsWaiting)
  {
    this->WaitForConnection();
  }
  else
  {
    this->Connect();
  }
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::SetupWaitForConnection()
{
  if (this->SocketCommunicator)
  {
    vtkErrorMacro("SetupWaitForConnection called more than once");
    return;
  }
  unsigned int myId = this->Controller->GetLocalProcessId();
  if (myId >= (unsigned int)this->NumberOfConnections)
  {
    return;
  }
  this->SocketCommunicator = vtkSocketCommunicator::New();
  // open a socket on a random port
  vtkDebugMacro(<< "open with port " << this->PortNumber);
  this->ServerSocket = vtkServerSocket::New();
  this->ServerSocket->CreateServer(this->PortNumber);

  // find out the random port picked
  this->PortNumber = this->ServerSocket->GetServerPort();
  if (this->NumberOfConnections == -1)
  {
    this->NumberOfConnections = this->Controller->GetNumberOfProcesses();
  }
  cout.flush();
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::WaitForConnection()
{
  unsigned int myId = this->Controller->GetLocalProcessId();
  if (myId >= static_cast<unsigned int>(this->NumberOfConnections))
  {
    return;
  }
  if (!this->SocketCommunicator || !this->ServerSocket)
  {
    vtkErrorMacro("SetupWaitForConnection must be called before WaitForConnection");
    return;
  }
  cout << "Waiting for connection:"
       << " rank :" << myId << " host :" << this->Internals->SelfHostName.c_str()
       << " port :" << this->PortNumber << "\n";

  vtkClientSocket* socket = this->ServerSocket->WaitForConnection();
  this->ServerSocket->Delete();
  this->ServerSocket = 0;
  if (!socket)
  {
    vtkErrorMacro("Failed to get connection!");
    return;
  }
  this->SocketCommunicator->SetSocket(socket);
  this->SocketCommunicator->ServerSideHandshake();
  socket->Delete();

  int data;
  this->SocketCommunicator->Receive(&data, 1, 1, 1238);
  cout << "Received Hello from process " << data << "\n";
  cout.flush();
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::Connect()
{
  if (this->SocketCommunicator)
  {
    vtkErrorMacro("Connect called more than once");
    return;
  }
  unsigned int myId = this->Controller->GetLocalProcessId();
  if (myId >= this->Internals->ServerInformation.size())
  {
    return;
  }

  this->SocketCommunicator = vtkSocketCommunicator::New();

  const vtkMPIMToNSocketConnectionInternals::NodeInformation& targetNode =
    this->Internals->ServerInformation[myId];

  cout << "Connecting :"
       << " rank :" << myId << " dest-host :" << targetNode.HostName.c_str()
       << " dest-port :" << targetNode.PortNumber << endl;

  this->SocketCommunicator->ConnectTo(
    const_cast<char*>(targetNode.HostName.c_str()), targetNode.PortNumber);

  int id = static_cast<int>(myId);
  this->SocketCommunicator->Send(&id, 1, 1, 1238);
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::SetNumberOfConnections(int c)
{
  this->NumberOfConnections = c;
  this->Internals->ServerInformation.resize(this->NumberOfConnections);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::SetPortInformation(
  unsigned int processNumber, int port, const char* host)
{
  if (processNumber >= this->Internals->ServerInformation.size())
  {
    vtkErrorMacro("Attempt to set port information for process larger than number of processes.\n"
      << "Max process id " << this->Internals->ServerInformation.size() << " attempted "
      << processNumber << "\n");
    return;
  }
  this->Internals->ServerInformation[processNumber].PortNumber = port;
  if (host)
  {
    this->Internals->ServerInformation[processNumber].HostName = host;
  }
}

//------------------------------------------------------------------------------
void vtkMPIMToNSocketConnection::GetPortInformation(vtkMPIMToNSocketConnectionPortInformation* info)
{
  // This method must be called only on the "Waiting" process.
  if (this->NumberOfConnections == -1)
  {
    return;
  }

  // Pass the current processes information.
  info->SetConnectionInformation(
    this->Controller->GetLocalProcessId(), this->PortNumber, this->Internals->SelfHostName.c_str());
}
