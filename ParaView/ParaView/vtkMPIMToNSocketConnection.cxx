/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIMToNSocketConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

*/


#include "vtkMPIMToNSocketConnection.h"
#include "vtkSocketCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include <vtkstd/string>
#include <vtkstd/vector>


vtkCxxRevisionMacro(vtkMPIMToNSocketConnection, "1.3");
vtkStandardNewMacro(vtkMPIMToNSocketConnection);

vtkCxxSetObjectMacro(vtkMPIMToNSocketConnection,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIMToNSocketConnection,SocketCommunicator, vtkSocketCommunicator);
class vtkMPIMToNSocketConnectionInternals
{
public:
  struct NodeInformation
  {
    int PortNumber;
    vtkstd::string HostName;
  };
  vtkstd::vector<NodeInformation> ServerInformation;
};


vtkMPIMToNSocketConnection::vtkMPIMToNSocketConnection()
{
  this->HostName = 0;
  this->PortNumber = -1;
  this->Internals = new vtkMPIMToNSocketConnectionInternals;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
  this->SocketCommunicator = 0;
}

vtkMPIMToNSocketConnection::~vtkMPIMToNSocketConnection()
{
  if(this->SocketCommunicator)
    {
    this->SocketCommunicator->CloseConnection();
    this->SocketCommunicator->Delete();
    }
  delete [] this->HostName;
  this->HostName = 0;
  delete this->Internals;
  this->Internals = 0;
}


void vtkMPIMToNSocketConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "NumberOfConnections: (" << this->NumberOfConnections << ")\n";
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketCommunicator: (" << this->SocketCommunicator << ")\n";
  vtkIndent i2 = indent.GetNextIndent();
  for(unsigned int i = 0; i < this->Internals->ServerInformation.size(); ++i)
    {
    os << i2 << "Server Information for Process: " << i << ":\n";
    vtkIndent i3 = i2.GetNextIndent();
    os << i3 << "PortNumber: " << this->Internals->ServerInformation[i].PortNumber << "\n";
    os << i3 << "HostName: " << this->Internals->ServerInformation[i].HostName.c_str() << "\n";
    }
}



void  vtkMPIMToNSocketConnection::SetupWaitForConnection()
{
  int myId = this->Controller->GetLocalProcessId();
  // This should determine the host, and select a random port
  // for now, just hard code localhost and port 33333 for testing
  this->SetHostName("localhost");
  this->PortNumber = 33333 + myId;
  this->NumberOfConnections = this->Controller->GetNumberOfProcesses();
}

void vtkMPIMToNSocketConnection::WaitForConnection()
{ 
  if(this->SocketCommunicator)
    {
    vtkErrorMacro("WaitForConnection called more than once");
    return;
    }
  int myId = this->Controller->GetLocalProcessId();
  if(myId >= this->NumberOfConnections)
    {
    return;
    }
  this->SocketCommunicator = vtkSocketCommunicator::New();
  cout << "WaitForConnection: id :" << myId << "  host: " << this->HostName << "  Port:" << this->PortNumber << "\n";
  this->SocketCommunicator->WaitForConnection(this->PortNumber);
  int data;
  this->SocketCommunicator->Receive(&data, 1, 1, 1238);
  cout << "Received Hello from process " << data << "\n";
} 

void vtkMPIMToNSocketConnection::Connect()
{ 
   if(this->SocketCommunicator)
    {
    vtkErrorMacro("Connect called more than once");
    return;
    }
  unsigned int myId = this->Controller->GetLocalProcessId();
  if(myId >= this->Internals->ServerInformation.size())
    {
    return;
    }
  this->SocketCommunicator = vtkSocketCommunicator::New();
  cout << "Connect: id :" << myId << "  host: " 
       << this->Internals->ServerInformation[myId].HostName.c_str() 
       << "  Port:" 
       << this->Internals->ServerInformation[myId].PortNumber 
       << "\n";
  this->SocketCommunicator->ConnectTo((char*)this->Internals->ServerInformation[myId].HostName.c_str(),
                                      this->Internals->ServerInformation[myId].PortNumber );
  int id = static_cast<int>(myId);
  this->SocketCommunicator->Send(&id, 1, 1, 1238);
}


void vtkMPIMToNSocketConnection::SetNumberOfConnections(int c)
{
  this->NumberOfConnections = c;
  this->Internals->ServerInformation.resize(this->NumberOfConnections);
  this->Modified();
}


void vtkMPIMToNSocketConnection::SetPortInformation(unsigned int processNumber,
                                                    int port, const char* host)
{
  if(processNumber >= this->Internals->ServerInformation.size())
    {
    vtkErrorMacro("Attempt to set port information for process larger than number of processes.\n"
                  << "Max process id " << this->Internals->ServerInformation.size()
                  << " attempted " << processNumber << "\n");
    return;
    }
  this->Internals->ServerInformation[processNumber].PortNumber = port;
  this->Internals->ServerInformation[processNumber].HostName = host;
}



void vtkMPIMToNSocketConnection::GetPortInformation(
  vtkMPIMToNSocketConnectionPortInformation* info)
{
  info->SetNumberOfConnections(this->Controller->GetNumberOfProcesses()); 
  int myId = this->Controller->GetLocalProcessId();
  // for id = 0 set the port information for process 0 in
  // in the information object, this is because the gather does
  // not call AddInformation for process 0
  if(myId == 0)
    {
    info->SetPortInformation(0, this->PortNumber, this->HostName);
    }
  info->SetHostName(this->HostName);
  info->SetProcessNumber(myId);
  info->SetPortNumber(this->PortNumber);
}

  
