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
#include "vtkObjectFactory.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkServerSocket.h"
#include "vtkSocketCommunicator.h"


#include <vtkstd/string>
#include <vtkstd/vector>


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
  vtkstd::vector<vtkstd::string> MachineNames;
};


vtkMPIMToNSocketConnection::vtkMPIMToNSocketConnection()
{
  this->MachinesFileName = 0;
  this->Socket = 0;
  this->HostName = 0;
  this->PortNumber = 0;
  this->Internals = new vtkMPIMToNSocketConnectionInternals;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
  this->SocketCommunicator = 0;
  this->NumberOfConnections = -1;
  this->ServerSocket = 0;
}

vtkMPIMToNSocketConnection::~vtkMPIMToNSocketConnection()
{
  if (this->ServerSocket)
    {
    this->ServerSocket->Delete();
    this->ServerSocket = 0;
    }
  if(this->SocketCommunicator)
    {
    this->SocketCommunicator->CloseConnection();
    this->SocketCommunicator->Delete();
    } 
  this->SetController(0);
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
  os << indent << "Socket: (" << this->Socket << ")\n";
  os << indent << "SocketCommunicator: (" << this->SocketCommunicator << ")\n";
  vtkIndent i2 = indent.GetNextIndent();
  for(unsigned int i = 0; i < this->Internals->ServerInformation.size(); ++i)
    {
    os << i2 << "Server Information for Process: " << i << ":\n";
    vtkIndent i3 = i2.GetNextIndent();
    os << i3 << "PortNumber: " << this->Internals->ServerInformation[i].PortNumber << "\n";
    os << i3 << "HostName: " << this->Internals->ServerInformation[i].HostName.c_str() << "\n";
    }
  os << indent << "MachinesFileName: " << (this->MachinesFileName?this->MachinesFileName:"(none)") << endl;
  os << indent << "PortNumber: " << this->PortNumber << endl;
}

void vtkMPIMToNSocketConnection::LoadMachinesFile()
{
  if(!this->MachinesFileName)
    {
    return;
    }

  vtkWarningMacro("The names of the machines making up this server should "
                  "be specified in the XML configuration file. The --machines "
                  "(and -m) command-line arguments have been deprecated and "
                  "will be removed in the next ParaView release.");

  FILE* file = fopen(this->MachinesFileName, "r");
  char machinename[1024];
  if(!file)
    {
    vtkErrorMacro("Could not open file : " << this->MachinesFileName);
    return;
    }
  while(!feof(file))
    {
    if(fgets(machinename, 1024, file) != 0)
      {
      int pos = static_cast<int>(strlen(machinename))-1;
      if(machinename[pos] == '\n')
        {
        machinename[pos] = 0;
        }
      if(strlen(machinename) > 0)
        {
        this->Internals->MachineNames.push_back(machinename);
        }
      }
    }
  fclose(file);
}

void vtkMPIMToNSocketConnection::SetMachineName(unsigned int idx,
                                                const char* name)
{
  if (name && strlen(name) > 0)
    {
    if (idx >= this->Internals->MachineNames.size())
      {
      this->Internals->MachineNames.push_back(name);
      }
    else
      {
      this->Internals->MachineNames[idx] = name;
      }
    }
}

void  vtkMPIMToNSocketConnection::SetupWaitForConnection()
{
  if(this->SocketCommunicator)
    {
    vtkErrorMacro("SetupWaitForConnection called more than once");
    return;
    }
  unsigned int myId = this->Controller->GetLocalProcessId();
  if(myId >= (unsigned int)this->NumberOfConnections)
    {
    return;
    }
  this->SocketCommunicator = vtkSocketCommunicator::New();
  // open a socket on a random port
  vtkDebugMacro( << "open with port " << this->PortNumber );
  this->ServerSocket = vtkServerSocket::New();
  this->ServerSocket->CreateServer(this->PortNumber);
  
  // find out the random port picked
  int port = this->ServerSocket->GetServerPort();
  if(this->Internals->MachineNames.size())
    {
    if( myId < this->Internals->MachineNames.size())
      {
      this->SetHostName(this->Internals->MachineNames[myId].c_str());
      }
    else
      {
      vtkErrorMacro("Bad configuration file more processes than machines listed."
                    << " Configfile= " << this->MachinesFileName << "\n"
                    << " process id = " << myId << "\n"
                    << " number of machines in file: " <<  
                    this->Internals->MachineNames.size() << "\n");
      this->SetHostName("localhost");
      }
    }
  else
    {
    this->SetHostName("localhost");
    }
  this->PortNumber = port;
  if(this->NumberOfConnections == -1)
    {
      this->NumberOfConnections = this->Controller->GetNumberOfProcesses();
    }
  cout.flush();
}

void vtkMPIMToNSocketConnection::WaitForConnection()
{ 
  unsigned int myId = this->Controller->GetLocalProcessId();
  if(myId >= static_cast<unsigned int>(this->NumberOfConnections))
    {
    return;
    }
  if(!this->SocketCommunicator || !this->ServerSocket)
    {
    vtkErrorMacro("SetupWaitForConnection must be called before WaitForConnection");
    return;
    }
  cout << "WaitForConnection: id :" 
       << myId << "  Port:" << this->PortNumber << "\n";
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
  cout.flush();
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
  if(host)
    {
      this->Internals->ServerInformation[processNumber].HostName = host;
    }
}



void vtkMPIMToNSocketConnection::GetPortInformation(
  vtkMPIMToNSocketConnectionPortInformation* info)
{
  // if the number of connections are not set then
  // use the number of processes for this group
  // if not, then use the set number of connections
  // this is for support of connections both ways
  if(this->NumberOfConnections == -1)
    {
      info->SetNumberOfConnections(this->Controller->GetNumberOfProcesses()); 
    }
  else
    {
      info->SetNumberOfConnections(this->NumberOfConnections); 
    }
  int myId = this->Controller->GetLocalProcessId();
  // for id = 0 set the port information for process 0 in
  // in the information object, this is because the gather does
  // not call AddInformation for process 0
  if(myId == 0)
    {
    this->LoadMachinesFile();
    info->SetPortNumber(0, this->PortNumber);
    if(this->Internals->MachineNames.size() &&
       (this->Internals->MachineNames.size() 
        < static_cast<unsigned int>(info->GetNumberOfConnections())))
      {
      vtkErrorMacro("Bad Configuration file, expected " << info->GetNumberOfConnections()
                    << " machines and found " << this->Internals->MachineNames.size());
      }
    for(unsigned int i = 0; i < this->Internals->MachineNames.size(); ++i)
      {
      info->SetHostName(i, this->Internals->MachineNames[i].c_str());
      }
    }
  info->SetHostName(this->HostName);
  info->SetProcessNumber(myId);
  info->SetPortNumber(this->PortNumber);
}

  
