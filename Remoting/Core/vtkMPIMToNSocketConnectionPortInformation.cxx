/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIMToNSocketConnectionPortInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIMToNSocketConnectionPortInformation.h"

#include "vtkClientServerStream.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkObjectFactory.h"
#include <string>
#include <vector>

class vtkMPIMToNSocketConnectionPortInformationInternals
{
public:
  struct NodeInformation
  {
    int PortNumber;
    std::string HostName;
    NodeInformation() { this->PortNumber = -1; }
  };
  std::vector<NodeInformation> Connections;
};

vtkStandardNewMacro(vtkMPIMToNSocketConnectionPortInformation);
//----------------------------------------------------------------------------
vtkMPIMToNSocketConnectionPortInformation::vtkMPIMToNSocketConnectionPortInformation()
{
  this->Internals = new vtkMPIMToNSocketConnectionPortInformationInternals;
}

//----------------------------------------------------------------------------
vtkMPIMToNSocketConnectionPortInformation::~vtkMPIMToNSocketConnectionPortInformation()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "All Process Information:\n";
  for (unsigned int i = 0; i < this->Internals->Connections.size(); ++i)
  {
    os << i2 << "P" << i << ":  PortNumber: " << this->Internals->Connections[i].PortNumber << "\n";
    os << i2 << "P" << i << ":  HostName: " << this->Internals->Connections[i].HostName.c_str()
       << "\n";
  }
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::CopyFromObject(vtkObject* obj)
{
  vtkMPIMToNSocketConnection* c = vtkMPIMToNSocketConnection::SafeDownCast(obj);
  if (!c)
  {
    vtkErrorMacro("Cannot get class name from NULL object. Or incorrect object type.");
    return;
  }
  c->GetPortInformation(this);
}

//----------------------------------------------------------------------------
int vtkMPIMToNSocketConnectionPortInformation::GetNumberOfConnections()
{
  return static_cast<int>(this->Internals->Connections.size());
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::SetConnectionInformation(
  unsigned int processNumber, int port, const char* hostname)
{
  if (processNumber >= this->Internals->Connections.size())
  {
    this->Internals->Connections.resize(processNumber + 1);
  }

  this->Internals->Connections[processNumber].HostName = hostname ? hostname : "";
  this->Internals->Connections[processNumber].PortNumber = port;
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::AddInformation(vtkPVInformation* i)
{
  vtkMPIMToNSocketConnectionPortInformation* info =
    vtkMPIMToNSocketConnectionPortInformation::SafeDownCast(i);
  if (!info)
  {
    vtkErrorMacro("Wrong type for AddInformation" << i);
    return;
  }

  if (this->Internals->Connections.size() < info->Internals->Connections.size())
  {
    this->Internals->Connections.resize(info->Internals->Connections.size());
  }

  for (size_t cc = 0; cc < info->Internals->Connections.size(); cc++)
  {
    if (info->Internals->Connections[cc].PortNumber > 0)
    {
      this->Internals->Connections[cc] = info->Internals->Connections[cc];
    }
  }
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->Internals->Connections.size();
  for (size_t i = 0; i < this->Internals->Connections.size(); ++i)
  {
    *css << this->Internals->Connections[i].PortNumber
         << this->Internals->Connections[i].HostName.c_str();
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int numProcesses;
  css->GetArgument(0, 0, &numProcesses);
  this->Internals->Connections.resize(numProcesses);

  int port;
  int pos = 1;
  const char* hostname = nullptr;
  for (int j = 0; j < numProcesses; ++j)
  {
    css->GetArgument(0, pos, &port);
    pos++;
    css->GetArgument(0, pos, &hostname);
    pos++;

    this->Internals->Connections[j].PortNumber = port;
    this->Internals->Connections[j].HostName = hostname ? hostname : "";
  }
}

//----------------------------------------------------------------------------
int vtkMPIMToNSocketConnectionPortInformation::GetProcessPort(unsigned int processNumber)
{
  if (processNumber >= this->Internals->Connections.size())
  {
    vtkErrorMacro("Process number greater than number of processes");
    return 0;
  }

  return this->Internals->Connections[processNumber].PortNumber;
}

//----------------------------------------------------------------------------
const char* vtkMPIMToNSocketConnectionPortInformation::GetProcessHostName(
  unsigned int processNumber)
{
  if (processNumber >= this->Internals->Connections.size())
  {
    vtkErrorMacro("Process number greater than number of processes");
    return nullptr;
  }

  if (this->Internals->Connections[processNumber].HostName.size() == 0)
  {
    return "localhost";
  }

  return this->Internals->Connections[processNumber].HostName.c_str();
}
