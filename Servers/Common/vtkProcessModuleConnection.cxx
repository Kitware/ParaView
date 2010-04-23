/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProcessModuleConnection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVProgressHandler.h"
#include "vtkSocket.h"

class vtkProcessModuleConnectionObserver : public vtkCommand
{
public:
  static vtkProcessModuleConnectionObserver* New()
    {
    return new vtkProcessModuleConnectionObserver;
    }
  void SetTarget(vtkProcessModuleConnection* conn)
    {
    this->Target = conn;
    }
  virtual void Execute(vtkObject *caller, unsigned long eventId, 
    void *callData)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventId, callData);
      }
    this->AbortFlagOn();
    }
protected:
  vtkProcessModuleConnectionObserver()
    {
    this->Target = 0;
    }
  ~vtkProcessModuleConnectionObserver()
    {
    this->Target = 0;
    }
  vtkProcessModuleConnection* Target; 

};

//-----------------------------------------------------------------------------
vtkProcessModuleConnection::vtkProcessModuleConnection()
{
  this->Controller = NULL;
  this->AbortConnection = 0;

  this->Observer = vtkProcessModuleConnectionObserver::New();
  this->Observer->SetTarget(this);

  this->ProgressHandler = vtkPVProgressHandler::New();
}

//-----------------------------------------------------------------------------
vtkProcessModuleConnection::~vtkProcessModuleConnection()
{
  this->ProgressHandler->Delete();
  this->ProgressHandler = 0;
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
}

//-----------------------------------------------------------------------------
vtkCommand* vtkProcessModuleConnection::GetObserver()
{
  return this->Observer;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnection::ExecuteEvent(vtkObject* caller, 
  unsigned long eventId, void* calldata)
{
  switch (eventId)
    {
  case vtkCommand::ErrorEvent:
    // Socket error!
    if (vtkSocket::SafeDownCast(caller))
      {
      this->OnSocketError();
      }
    break;

  case vtkCommand::WrongTagEvent:
    this->OnWrongTagEvent(caller, calldata);
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnection::OnSocketError()
{
  if (!this->AbortConnection)
    {
    vtkDebugMacro("Communication Error. Connection will be closed.");
    this->AbortConnection = 1;
    this->InvokeEvent(vtkCommand::AbortCheckEvent);
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnection::OnWrongTagEvent(
  vtkObject* vtkNotUsed(caller), void* calldata)
{
  int tag = -1;
  int len = -1;
  const char* data = reinterpret_cast<const char*>(calldata);
  const char* ptr = data;
  memcpy(&tag, ptr, sizeof(tag));

  if ( tag != vtkProcessModule::PROGRESS_EVENT_TAG 
    && tag != vtkProcessModule::EXCEPTION_EVENT_TAG)
    {
    vtkErrorMacro("Internal ParaView Error: "
      "Socket Communicator received wrong tag: "
      << tag);
    // Treat as a socket error.
    this->OnSocketError();
    return;
    }

  ptr += sizeof(tag);
  memcpy(&len, ptr, sizeof(len));
  ptr += sizeof(len);
  if (tag == vtkProcessModule::PROGRESS_EVENT_TAG)
    {
    char val = -1;
    val = *ptr;
    ptr ++;
    if ( val < 0 || val > 100 )
      {
      vtkErrorMacro("Received progres not in the range 0 - 100: " << (int)val);
      return;
      }
    this->ProgressHandler->HandleServerProgress(val, ptr);
    }
  else if (tag == vtkProcessModule::EXCEPTION_EVENT_TAG)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(ptr);
    this->OnSocketError();
    }
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnection::Initialize(
  int vtkNotUsed(argc), char** vtkNotUsed(argv), int *vtkNotUsed(partitionId))
{
  this->ProgressHandler->SetConnection(this);
  // returns 0 on success, 1 on error.
  return 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnection::Finalize()
{
  if (this->Controller)
    {
    this->Controller->Finalize(1);
    }
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnection::GetNumberOfPartitions()
{
  if (this->Controller)
    {
    return this->Controller->GetNumberOfProcesses();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnection::GetPartitionId()
{
  if (this->Controller)
    {
    return this->Controller->GetLocalProcessId();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnection::GatherInformation(vtkTypeUInt32 , 
    vtkPVInformation* , vtkClientServerID )
{
  vtkErrorMacro("GatherInformation not support by this type of connection: "
    << this->GetClassName());
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnection::SendStream(vtkTypeUInt32 servers, 
  vtkClientServerStream& stream)
{
  vtkTypeUInt32 sendflag = this->CreateSendFlag(servers);
  if (!this->AbortConnection)
    {
    // Dont send to servers if the connection has been aborted.
    if (sendflag & vtkProcessModule::DATA_SERVER)
      {
      this->SendStreamToDataServer(stream);
      }
    if(sendflag & vtkProcessModule::RENDER_SERVER)
      {
      this->SendStreamToRenderServer(stream);
      }
    if(sendflag & vtkProcessModule::DATA_SERVER_ROOT)
      {
      this->SendStreamToDataServerRoot(stream);
      }
    if(sendflag & vtkProcessModule::RENDER_SERVER_ROOT)
      {
      this->SendStreamToRenderServerRoot(stream);
      }
    }
  if (sendflag & vtkProcessModule::CLIENT)
    {
    this->SendStreamToClient(stream);
    }
  return 0;
}

//----------------------------------------------------------------------------
// send a stream to the data server
int vtkProcessModuleConnection::SendStreamToDataServer(vtkClientServerStream&)
{
  vtkErrorMacro("SendStreamToDataServer called on connection that does not implement it");
  return -1;
}

//----------------------------------------------------------------------------
// send a stream to the data server root mpi process
int vtkProcessModuleConnection::SendStreamToDataServerRoot(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToDataServerRoot called on connection that does not implement it");
  return -1;
}

//----------------------------------------------------------------------------
// send a stream to the render server
int vtkProcessModuleConnection::SendStreamToRenderServer(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToRenderServer called on connection that does not implement it");
  return -1;
}

//----------------------------------------------------------------------------
// send a stream to the render server root mpi process
int vtkProcessModuleConnection::SendStreamToRenderServerRoot(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToRenderServerRoot called on connection that does not implement it");
  return -1;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnection::SendStreamToClient(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToClient called on connection that does not implement it.");
  return -1;
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModuleConnection::GetLastResult(vtkTypeUInt32)
{
  vtkErrorMacro("GetLastResult not supported by " << this->GetClassName());
  static vtkClientServerStream nullStream;
  return nullStream;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleConnection::LoadModule(const char* , const char* )
{
  vtkErrorMacro("LoadModule not support by " << this->GetClassName());  
  return 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModuleConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AbortConnection: " << this->AbortConnection << endl;
  os << indent << "Controller: ";
  if (this->Controller)
    {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
