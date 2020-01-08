/*=========================================================================

  Program:   ParaView
  Module:    TestSimple.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"

class Server
{
public:
  Server();
  ~Server() { ClientServerInterpreter->Delete(); }
  void GetResultMessageData(const unsigned char**, size_t*);
  void ProcessMessage(const unsigned char*, size_t);
  void PrintObjects();

private:
  vtkClientServerInterpreter* ClientServerInterpreter;
  vtkClientServerStream ServerStream;
};

class ClientManager
{
public:
  void SetServer(Server* s) { this->server = s; }
  const vtkClientServerStream* GetResultMessage();
  vtkClientServerID GetUniqueID()
  {
    static vtkClientServerID id(3);
    ++id.ID;
    return id;
  }
  void RunTests();
  vtkClientServerStream stream;
  Server* server;
};

// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);

Server::Server()
{
  this->ClientServerInterpreter = vtkClientServerInterpreter::New();
  vtkCommonCS_Initialize(this->ClientServerInterpreter);
  vtkFilteringCS_Initialize(this->ClientServerInterpreter);
  vtkImagingCS_Initialize(this->ClientServerInterpreter);
  vtkGraphicsCS_Initialize(this->ClientServerInterpreter);
  vtkIOCS_Initialize(this->ClientServerInterpreter);
  vtkRenderingCS_Initialize(this->ClientServerInterpreter);
}

void Server::GetResultMessageData(const unsigned char** data, size_t* len)
{
  const vtkClientServerStream& ames = this->ClientServerInterpreter->GetLastResult();
  if (!(ames.GetNumberOfMessages() > 0 && ames.GetData(data, len)))
  {
    *data = 0;
    *len = 0;
  }
}

void Server::ProcessMessage(const unsigned char* msg, size_t length)
{
  if (!this->ClientServerInterpreter->ProcessStream(msg, length))
  {
    cerr << "error in process message\n";
  }
}

void Server::PrintObjects()
{
}

const vtkClientServerStream* ClientManager::GetResultMessage()
{
  const unsigned char* data;
  size_t len;
  // simulate getting a message over a socket from the server
  server->GetResultMessageData(&data, &len);

  // now create a message on the client
  vtkClientServerStream* result = new vtkClientServerStream;
  result->SetData(data, len);
  return result;
}

void ClientManager::RunTests()
{
  vtkClientServerID instance_id = this->GetUniqueID();
  stream << vtkClientServerStream::New << "vtkObject" << instance_id << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << instance_id << "DebugOn" << vtkClientServerStream::End;
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "GetClassName"
         << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  const char* name;
  if (this->GetResultMessage()->GetArgument(0, 0, &name))
  {
    cerr << name << "\n";
  }
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "SetReferenceCount" << 10
         << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "GetReferenceCount"
         << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  int refcount;
  if (this->GetResultMessage()->GetArgument(0, 0, &refcount))
  {
    cerr << refcount << "\n";
  }
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "SetReferenceCount" << 1
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Delete << instance_id << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
}

int TestSimple(int, char**)
{
  Server server;
  ClientManager cmgr;
  cmgr.SetServer(&server);
  cmgr.RunTests();
  return 0;
}

// new names
// vtkClientServerInterpreter - vtkClientServerInterpreter - process data, len

// // vtkClientServerMessage - vtkClientServerMessage -
// // vtkClientServerStream - vtkClientServerStream  - has the unique id stuff and GetData(data,
// len)

// vtkPVClientServerModule - should have vtkClientServerStream
// If(this->ClientMode)
//   {
//     initialize
//       add method FlushMessages()
//       CallRMI
//       }
// else
// {
//   vtkClientServerInterpreter

//     add RMI
