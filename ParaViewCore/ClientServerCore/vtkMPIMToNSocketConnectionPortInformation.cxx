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
#include "vtkObjectFactory.h"
#include "vtkMPIMToNSocketConnection.h"
#include <vtkstd/string>
#include <vtkstd/vector>

class vtkMPIMToNSocketConnectionPortInformationInternals
{
public:
  struct NodeInformation
  {
    int PortNumber;
    vtkstd::string HostName;
    NodeInformation()
      {
      this->PortNumber = -1;
      }
  };
  vtkstd::vector<NodeInformation> ServerInformation;
};



vtkStandardNewMacro(vtkMPIMToNSocketConnectionPortInformation);

//----------------------------------------------------------------------------
vtkMPIMToNSocketConnectionPortInformation::vtkMPIMToNSocketConnectionPortInformation()
{
  this->Internals = new vtkMPIMToNSocketConnectionPortInformationInternals;
  this->HostName = 0;
  this->NumberOfConnections = 0;
  this->ProcessNumber = 0;
  this->PortNumber = 0;
}

//----------------------------------------------------------------------------
vtkMPIMToNSocketConnectionPortInformation::~vtkMPIMToNSocketConnectionPortInformation()
{
  delete this->Internals;
  this->SetHostName(0);
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "\n";
  os << indent << "HostName: "
     << (this->HostName?this->HostName:"(none)") << "\n";
  os << indent << "NumberOfConnections: " << this->NumberOfConnections << "\n";
  os << indent << "ProcessNumber: " << this->ProcessNumber << "\n";
  os << indent << "PortNumber: " << this->PortNumber << "\n";
  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "All Process Information:\n";
  for(unsigned int i = 0; i < this->Internals->ServerInformation.size(); ++i)
    {
    os << i2 << "P" << i << ":  PortNumber: " << this->Internals->ServerInformation[i].PortNumber << "\n";
    os << i2 << "P" << i << ":  HostName: " << this->Internals->ServerInformation[i].HostName.c_str() << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::CopyFromObject(vtkObject* obj)
{
  vtkMPIMToNSocketConnection* c = vtkMPIMToNSocketConnection::SafeDownCast(obj);
  if(!c)
    {
    vtkErrorMacro("Cannot get class name from NULL object. Or incorrect object type.");
    return;
    }
  c->GetPortInformation(this);
}


void vtkMPIMToNSocketConnectionPortInformation::SetPortNumber(unsigned int processNumber,
                                                              int port)
{
  if(this->Internals->ServerInformation.size() == 0)
    {
    this->Internals->ServerInformation.resize(this->NumberOfConnections);
    }
  if(processNumber >= this->Internals->ServerInformation.size())
    {
      return;
    }
  this->Internals->ServerInformation[processNumber].PortNumber = port;
}

void vtkMPIMToNSocketConnectionPortInformation::SetHostName(unsigned int processNumber,
                                                            const char* hostname)
{
  if(this->Internals->ServerInformation.size() == 0)
    {
    this->Internals->ServerInformation.resize(this->NumberOfConnections);
    }
  if(processNumber >= this->Internals->ServerInformation.size())
    {
      return;
    }
  this->Internals->ServerInformation[processNumber].HostName = hostname;
}

//----------------------------------------------------------------------------
void vtkMPIMToNSocketConnectionPortInformation::AddInformation(vtkPVInformation* i)
{
  vtkMPIMToNSocketConnectionPortInformation* info 
    = vtkMPIMToNSocketConnectionPortInformation::SafeDownCast(i);
  if(!info)
    {
    vtkErrorMacro("Wrong type for AddInformation" << i);
    return;
    }

  for (size_t cc=0; cc < info->Internals->ServerInformation.size(); cc++)
    {
    if (info->Internals->ServerInformation[cc].PortNumber > 0)
      {
      this->SetPortNumber(
        static_cast<unsigned int>(cc),
        info->Internals->ServerInformation[cc].PortNumber);
      }
    }
  this->SetPortNumber(info->ProcessNumber, info->PortNumber);
}

//----------------------------------------------------------------------------
void
vtkMPIMToNSocketConnectionPortInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply 
       << this->HostName
       << this->NumberOfConnections
       << this->ProcessNumber
       << this->PortNumber
       << this->Internals->ServerInformation.size();
  for(unsigned int i=0; i <  this->Internals->ServerInformation.size(); ++i)
    {
    *css << this->Internals->ServerInformation[i].PortNumber
         << this->Internals->ServerInformation[i].HostName.c_str();
    }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkMPIMToNSocketConnectionPortInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* hostname = 0;
  css->GetArgument(0, 0, &hostname);
  this->SetHostName(hostname);
  int i = 0;
  css->GetArgument(0, 1, &i);
  this->SetNumberOfConnections(i); 
  css->GetArgument(0, 2, &i);
  this->SetProcessNumber(i);
  css->GetArgument(0, 3, &i);
  this->SetPortNumber(i);
  int numProcesses;
  css->GetArgument(0, 4, &numProcesses);
  this->Internals->ServerInformation.resize(numProcesses);

  int port;
  int pos = 5;
  for(int j =0; j < numProcesses; ++j)
    {
    css->GetArgument(0, pos, &port);
    pos++;
    css->GetArgument(0, pos, &hostname);
    pos++;
    this->Internals->ServerInformation[j].PortNumber = port;
    this->Internals->ServerInformation[j].HostName = hostname;
    }
}



int vtkMPIMToNSocketConnectionPortInformation::GetProcessPort(unsigned int processNumber)
{
  if(this->Internals->ServerInformation.size() == 0 && processNumber == 0)
    {
    return this->PortNumber;
    }
  if(processNumber >= this->Internals->ServerInformation.size())
    {
    vtkErrorMacro("Process number greater than number of processes");
    return 0;
    }
  return this->Internals->ServerInformation[processNumber].PortNumber;
}

const char* vtkMPIMToNSocketConnectionPortInformation::GetProcessHostName(unsigned int processNumber)
{
  if(this->Internals->ServerInformation.size() == 0 && processNumber == 0)
    {
    return this->GetHostName();
    }
  if(processNumber >= this->Internals->ServerInformation.size())
    {
    vtkErrorMacro("Process number greater than number of processes");
    return 0;
    }
  if(this->Internals->ServerInformation[processNumber].HostName.size() == 0)
    {
    return this->GetHostName();
    }
  return this->Internals->ServerInformation[processNumber].HostName.c_str();
}
