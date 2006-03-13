/*=========================================================================

  Program:   ParaView
  Module:    vtkMPISelfConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPISelfConnection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkDummyController.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVInformation.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

//----------------------------------------------------------------------------
void vtkMPISelfConnectionProcessRMI(void *localArg, 
  void *remoteArg, int remoteArgLength,
  int vtkNotUsed(remoteProcessId))
{
  vtkMPISelfConnection* conn = (vtkMPISelfConnection*)localArg;
  conn->ProcessStreamLocally(
    reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);
}
//-----------------------------------------------------------------------------
// Called on satellite when Root is requesting Information.
void vtkMPISelfConnectionGatherInformationRMI(void *localArg, 
  void *remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  vtkClientServerStream stream;
  stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);
  
  vtkMPISelfConnection* self = (vtkMPISelfConnection*)localArg;
  self->GatherInformationSatellite(stream);
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMPISelfConnection);
vtkCxxRevisionMacro(vtkMPISelfConnection, "1.2");
//-----------------------------------------------------------------------------
vtkMPISelfConnection::vtkMPISelfConnection()
{
  // Remove the Controller created by Superclass.
  if (this->Controller)
    {
    this->Controller->Delete();
    }
  // The self controller is always the global controller.
#ifdef VTK_USE_MPI
  this->Controller = vtkMPIController::New();
#else
  this->Controller = vtkDummyController::New();
#endif  
  vtkMultiProcessController::SetGlobalController(this->Controller);
}

//-----------------------------------------------------------------------------
vtkMPISelfConnection::~vtkMPISelfConnection()
{
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::Initialize(int argc, char** argv)
{
  this->Controller->Initialize(&argc, &argv, 1);
  
  if (this->Controller->GetNumberOfProcesses() > 1)
    {// !!!!! For unix, this was done when MPI was defined (even for 1
      // process). !!!!!
    this->Controller->CreateOutputWindow();
    }

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif
  
  int ret = 0;
  
  if (this->GetPartitionId() == 0)
    {
    // Root process starts the GUI is there's any.
    ret = this->InitializeRoot(argc, argv);
    }
  else
    {
    // Called for every satellite.
    ret = this->InitializeSatellite(argc, argv);
    // It's essential to call finalize on the satellites here explicitly,
    // the reason being as follows:
    // Since vtkProcessModule::Exit() only gets called on the root node,
    // vtkMPISelfConnection::Finalize() never gets called on the satellites,
    // leaving the Controller unfinalized on the satellites, which is bad.
    this->Finalize();
    }

  return ret;
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::Finalize()
{
  if (this->GetPartitionId() == 0)
    {
    // The root tells all the satellites to finish.
    int numProcs = this->GetNumberOfPartitions();
    for (int id=1; id < numProcs; id++)
      {
      this->Controller->TriggerRMI(id,
        vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }
  this->Superclass::Finalize();
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::InitializeRoot(int , char** )
{
  return 0;
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::InitializeSatellite(int vtkNotUsed(argc), 
  char** vtkNotUsed(argv))
{
  this->Controller->AddRMI(vtkMPISelfConnectionProcessRMI, this,
    vtkMPISelfConnection::ROOT_SATELLITE_RMI_TAG);
  this->Controller->AddRMI(vtkMPISelfConnectionGatherInformationRMI, this,
    vtkMPISelfConnection::ROOT_SATELLITE_GATHER_INFORMATION_RMI_TAG);

  // TODO: This call must not start RMI processing. 
  // We delegate that to ConnectionManager, or don't we?
  return this->Controller->ProcessRMIs();
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkMPISelfConnection::CreateSendFlag(vtkTypeUInt32 servers)
{
  vtkTypeUInt32 sendflag = 0;

  // The choice here is whether to send it to the ROOT or to every process.

  if (servers & vtkProcessModule::CLIENT)
    {
    sendflag |= vtkProcessModule::DATA_SERVER_ROOT;
    }
  // data server goes to data server
  if(servers & vtkProcessModule::DATA_SERVER)
    {
    sendflag |= vtkProcessModule::DATA_SERVER;
    }

  // render server goes to data server
  if(servers & vtkProcessModule::RENDER_SERVER)
    {
    sendflag |= vtkProcessModule::DATA_SERVER;
    }

  // render server root goes to data server root.
  if(servers & vtkProcessModule::RENDER_SERVER_ROOT)
    {
    sendflag |= vtkProcessModule::DATA_SERVER_ROOT;
    }

  // data server root goes to data server root.
  if(servers & vtkProcessModule::DATA_SERVER_ROOT)
    {
    sendflag |= vtkProcessModule::DATA_SERVER_ROOT;
    }

  // catch condition to ensure that the message is not 
  // sent to the DATA_SERVER_ROOT twice.
  if (sendflag & vtkProcessModule::DATA_SERVER &&
    sendflag & vtkProcessModule::DATA_SERVER_ROOT)
    {
    sendflag = vtkProcessModule::DATA_SERVER;
    }

  
  return sendflag;
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::SendStreamToDataServerRoot(vtkClientServerStream& stream)
{
  this->SendStreamToServerNodeInternal(0, stream);
  return 0;
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::SendStreamToDataServer(vtkClientServerStream& stream)
{
  // First send the command to the other server nodes, so
  // they can get to work first
  int numPartitions = this->GetNumberOfPartitions();
  for(int i=1; i < numPartitions; ++i)
    {
    this->SendStreamToServerNodeInternal(i, stream);
    }

  // Now process the stream locally.
  this->SendStreamToServerNodeInternal(0, stream);
  return 0;
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::SendStreamToServerNodeInternal(int remoteId,
  vtkClientServerStream& stream)
{
  vtkMultiProcessController* globalController = 
    vtkMultiProcessController::GetGlobalController();
  if (!globalController)
    {
    vtkErrorMacro("Global controller not set!");
    return;
    }

  if (remoteId == globalController->GetLocalProcessId())
    {
    this->ProcessStreamLocally(stream);
    }
  else
    {
    const unsigned char* data;
    size_t length;
    stream.GetData(&data, &length);
    if (length != 0)
      {
      this->Controller->TriggerRMI(remoteId, (void*)data, length,
        vtkMPISelfConnection::ROOT_SATELLITE_RMI_TAG);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::GatherInformation(vtkTypeUInt32 serverFlags, 
  vtkPVInformation* info, vtkClientServerID id)
{
  if (this->GetPartitionId() != 0)
    {
    vtkErrorMacro("GatherInformation cannot be called directly on satellites!");
    return;
    }
  
  // collect self information.
  this->Superclass::GatherInformation(serverFlags, info, id);
  if (info->GetRootOnly() || this->GetNumberOfPartitions() == 1)
    {
    // If not required to collect info from satellites, we are done.
    return;
    }
  this->GatherInformationRoot(info, id);
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::GatherInformationRoot(vtkPVInformation* info,
  vtkClientServerID id)
{
  int numProcs = this->GetNumberOfPartitions();
  int cc;
  
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Assign //dummy command.
    << info->GetClassName()
    << id << vtkClientServerStream::End;
  const unsigned char* sdata;
  size_t slength;
  stream.GetData(&sdata, &slength);
  
  for (cc = 1; cc < numProcs; cc++)
    {
    // Tell all satellites to gather information.
    this->Controller->TriggerRMI(cc, (void*)sdata, slength,
      vtkMPISelfConnection::ROOT_SATELLITE_GATHER_INFORMATION_RMI_TAG);
    }

  // Now, we must collect information from the satellites.
  vtkPVInformation* tempInfo = info->NewInstance();

  for (cc=1; cc < numProcs; cc++)
    {
    int length;
    this->Controller->Receive(&length, 1, cc, 
      vtkMPISelfConnection::ROOT_SATELLITE_INFO_LENGTH_TAG);
    if (length <= 0)
      {
      vtkErrorMacro("Failed to Gather Information from satellite no: " << cc);
      continue;
      }
    
    unsigned char* data = new unsigned char[length];
    this->Controller->Receive(data, length, cc,
      vtkMPISelfConnection::ROOT_SATELLITE_INFO_TAG);
    stream.SetData(data, length);
    tempInfo->CopyFromStream(&stream);
    info->AddInformation(tempInfo);
    delete [] data; 
    }
  tempInfo->Delete();
}


//-----------------------------------------------------------------------------
void vtkMPISelfConnection::GatherInformationSatellite(vtkClientServerStream&
  stream)
{
  const char* infoClassName;
  vtkClientServerID id;
  stream.GetArgument(0, 0, &infoClassName);
  stream.GetArgument(0, 1, &id);
  
  vtkObject* o = vtkInstantiator::CreateInstance(infoClassName);
  vtkPVInformation* info = vtkPVInformation::SafeDownCast(o);
  vtkObject* object = vtkObject::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(id));
  
  if (info && object)
    {
    info->CopyFromObject(object);
    
    vtkClientServerStream css;
    info->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = length;
    this->Controller->Send(&len, 1, 0,
      vtkMPISelfConnection::ROOT_SATELLITE_INFO_LENGTH_TAG);
    this->Controller->Send(const_cast<unsigned char*>(data),
      length, 0, vtkMPISelfConnection::ROOT_SATELLITE_INFO_TAG);
    }
  else
    {
    vtkErrorMacro("Could not gather information on Satellite.");
    // let root know.
    int len = 0; 
    this->Controller->Send(&len, 1, 0,
      vtkMPISelfConnection::ROOT_SATELLITE_INFO_LENGTH_TAG);
    }
  
  if (o) 
    { 
    o->Delete(); 
    }
}


//-----------------------------------------------------------------------------
int vtkMPISelfConnection::LoadModule(const char* name, const char* directory)
{
  const char* paths[] = { directory, 0};
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int localResult = pm->GetInterpreter()->Load(name, paths);

#ifdef VTK_USE_MPI
  vtkMPICommunicator* communicator = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if (!communicator)
    {
    return 0;
    }

  // Gather load results to process 0.
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myid = this->Controller->GetLocalProcessId();
  if (numProcs > 1)
    {
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
#endif
  return localResult;
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
