/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMPIProcessModule.h"
#include "vtkObjectFactory.h"
#include "vtkInstantiator.h"

#include "vtkClientServerStream.h"
#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkMultiProcessController.h"
#include "vtkDataSet.h"
#include "vtkSource.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkMapper.h"
#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVInformation.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"

#define VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG 397529

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMPIProcessModule);
vtkCxxRevisionMacro(vtkPVMPIProcessModule, "1.5");


// external global variable.
vtkMultiProcessController *VTK_PV_UI_CONTROLLER = NULL;


//----------------------------------------------------------------------------
void vtkPVSendStreamToServerNodeRMI(void *localArg, void *remoteArg,
                                    int remoteArgLength,
                                    int vtkNotUsed(remoteProcessId))
{
  vtkPVMPIProcessModule* self =
    reinterpret_cast<vtkPVMPIProcessModule*>(localArg);
  self->GetInterpreter()
    ->ProcessStream(reinterpret_cast<unsigned char*>(remoteArg),
                    remoteArgLength);
}


//----------------------------------------------------------------------------
vtkPVMPIProcessModule::vtkPVMPIProcessModule()
{
  this->Controller = NULL;

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;
}

//----------------------------------------------------------------------------
vtkPVMPIProcessModule::~vtkPVMPIProcessModule()
{
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }


  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;;
}

//----------------------------------------------------------------------------
// Each process starts with this method.  One process is designated as
// "master" and starts the application.  The other processes are slaves to
// the application.
void vtkPVMPIProcessModuleInit(
  vtkMultiProcessController *vtkNotUsed(controller), void *arg )
{
  vtkPVMPIProcessModule *self = (vtkPVMPIProcessModule *)arg;
  self->Initialize();
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::Initialize()
{
  int myId, numProcs;

  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  if (myId ==  0)
    { // The last process is for UI.
    this->ReturnValue = this->GUIHelper->
      RunGUIStart(this->ArgumentCount,this->Arguments, numProcs, myId);
    }
  else
    {
    this->Controller->AddRMI(vtkPVSendStreamToServerNodeRMI, this,
                             VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG);
    this->Controller->ProcessRMIs();
    }
}
//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::Start(int argc, char **argv)
{
  // Initialize the MPI controller.
  this->Controller = vtkMultiProcessController::New();
  this->Controller->Initialize(&argc, &argv, 1);
  vtkMultiProcessController::SetGlobalController(this->Controller);

  if (this->Controller->GetNumberOfProcesses() > 1)
    { // !!!!! For unix, this was done when MPI was defined (even for 1
      // process). !!!!!
    this->Controller->CreateOutputWindow();
    }
  this->ArgumentCount = argc;
  this->Arguments = argv;

  this->CreateLogFile("NodeLog");

  // Go through the motions.
  // This indirection is not really necessary and is just to mimick the
  // threaded controller.
  this->Controller->SetSingleMethod(vtkPVMPIProcessModuleInit,(void*)(this));
  this->Controller->SingleMethodExecute();

  this->Controller->Finalize(1);

  return this->ReturnValue;
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::Exit()
{
  int id, myId, num;

  // Send a break RMI to each of the slaves.
  num = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();
  for (id = 0; id < num; ++id)
    {
    if (id != myId)
      {
      this->Controller->TriggerRMI(id,
                                   vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::GetPartitionId()
{
  if (this->Controller)
    {
    return this->Controller->GetLocalProcessId();
    }
  return 0;
}

vtkTypeUInt32 vtkPVMPIProcessModule::CreateSendFlag(vtkTypeUInt32 servers)
{
  vtkTypeUInt32 sendflag = 0;
  // client goes to client
  if(servers & CLIENT)
    {
    sendflag |= CLIENT;
    }
  // data server goes to data server
  if(servers & DATA_SERVER)
    {
    sendflag |= DATA_SERVER;
    }
  // render server goes to data server
  if(servers & RENDER_SERVER)
    {
    sendflag |= DATA_SERVER;
    }
  // render server root goes to client
  if(servers & RENDER_SERVER_ROOT)
    {
    sendflag |= CLIENT;
    }
  // data server root goes to client
  if(servers & DATA_SERVER_ROOT)
    {
    sendflag |= CLIENT;
    }
  if((sendflag & CLIENT) && (sendflag & DATA_SERVER))
    {
    sendflag = DATA_SERVER;
    }
  
  return sendflag;
}

// send a stream to the client
int vtkPVMPIProcessModule::SendStreamToClient(vtkClientServerStream& stream)
{
  this->Interpreter->ProcessStream(stream);
  return 0;
}

int vtkPVMPIProcessModule::SendStreamToDataServer(vtkClientServerStream& stream)
{  
  // First send the command to the other server nodes, so
  // they can get to work first
  int numPartitions = this->GetNumberOfPartitions();
  for(int i=1; i < numPartitions; ++i)
    {
    this->SendStreamToServerNodeInternal(i, stream);
    }
  // Now process the stream locally.
  this->Interpreter->ProcessStream(stream);
  return 0;
}


int vtkPVMPIProcessModule::SendStreamToDataServerRoot(vtkClientServerStream&)
{
  vtkErrorMacro("SendStreamToDataServerRoot should not be called by vtkPVMPIProcessModule");
  return -1;
}

int vtkPVMPIProcessModule::SendStreamToRenderServer(vtkClientServerStream&)
{
  vtkErrorMacro("SendStreamToRenderServer should not be called by vtkPVMPIProcessModule");
  return -1;
}

int vtkPVMPIProcessModule::SendStreamToRenderServerRoot(vtkClientServerStream&)
{
  vtkErrorMacro("SendStreamToRenderServerRoot should not be called by vtkPVMPIProcessModule");
  return -1;
}

  

//----------------------------------------------------------------------------
void
vtkPVMPIProcessModule::SendStreamToServerNodeInternal(
  int remoteId, vtkClientServerStream& stream)
{
  if (stream.GetNumberOfMessages() < 1)
    {
    return;
    }

  if(remoteId == this->Controller->GetLocalProcessId())
    {
    this->Interpreter->ProcessStream(stream);
    }
  else
    {
    const unsigned char* data;
    size_t length;
    stream.GetData(&data, &length);
    this->Controller->TriggerRMI(remoteId, (void*)data, length,
                                 VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::GetNumberOfPartitions()
{
  if (this->Controller)
    {
    return this->Controller->GetNumberOfProcesses();
    }
  return 1;
}



//----------------------------------------------------------------------------
// This method is broadcast to all processes.
void
vtkPVMPIProcessModule::GatherInformationInternal(const char* infoClassName,
                                                 vtkObject *object)
{
  vtkClientServerStream css;
  int myId = this->Controller->GetLocalProcessId();

  if (object == NULL)
    {
    vtkErrorMacro("Object id must be wrong.");
    return;
    }

  // Create a temporary information object.
  vtkObject* o = vtkInstantiator::CreateInstance(infoClassName);
  vtkPVInformation* tmpInfo = vtkPVInformation::SafeDownCast(o);
  if (tmpInfo == NULL)
    {
    vtkErrorMacro("Could not create information object " << infoClassName);
    if (o) { o->Delete(); }
    // How to exit without blocking ???
    return;
    }
  o = NULL;

  if(myId != 0)
    {
    if(tmpInfo->GetRootOnly())
      {
      // Root-only and we are not the root.  Do nothing.
      tmpInfo->Delete();
      return;
      }
    tmpInfo->CopyFromObject(object);
    tmpInfo->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = static_cast<int>(length);
    this->Controller->Send(&len, 1, 0, 498798);
    this->Controller->Send(const_cast<unsigned char*>(data), len, 0, 498799);
    tmpInfo->Delete();
    return;
    }

  // This is node 0.  First get our own information.
  this->TemporaryInformation->CopyFromObject(object);

  if(!tmpInfo->GetRootOnly())
    {
    // Merge information from other nodes.
    int numProcs = this->Controller->GetNumberOfProcesses();
    for(int idx = 1; idx < numProcs; ++idx)
      {
      int length;
      this->Controller->Receive(&length, 1, idx, 498798);
      unsigned char* data = new unsigned char[length];
      this->Controller->Receive(data, length, idx, 498799);
      css.SetData(data, length);
      tmpInfo->CopyFromStream(&css);
      this->TemporaryInformation->AddInformation(tmpInfo);
      delete [] data;
      }
    }
  tmpInfo->Delete();
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
int vtkPVMPIProcessModule::LoadModuleInternal(const char* name,
                                              const char* directory)
{
  // Make sure we have a communicator.
  vtkMPICommunicator* communicator = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if(!communicator)
    {
    return 0;
    }

  // Try to load the module on the local process.
  int localResult = this->Superclass::LoadModuleInternal(name, directory);

  // Gather load results to process 0.
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myid = this->Controller->GetLocalProcessId();
  int* results = new int[numProcs];
  communicator->Gather(&localResult, results, 1, 0);

  int globalResult = 1;
  if(myid == 0)
    {
    for(int i=0; i < numProcs; ++i)
      {
      if(!results[i])
        {
        globalResult = 0;
        }
      }
    }

  delete [] results;

  return globalResult;
}
#else
int vtkPVMPIProcessModule::LoadModuleInternal(const char* name,
                                              const char* directory)
{
  return this->Superclass::LoadModuleInternal(name, directory);
}
#endif
