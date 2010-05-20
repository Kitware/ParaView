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
#include "vtkPVMPICommunicator.h"
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
int vtkMPISelfConnection::Initialize(int argc, char** argv, int *partitionId)
{
  this->Superclass::Initialize(argc, argv, partitionId);

#ifdef VTK_USE_MPI
  // Replace the communicator with vtkPVMPICommunicator which handles progress
  // events better than the conventional vtkMPICommunicator.
  vtkPVMPICommunicator* comm = vtkPVMPICommunicator::New();
  comm->CopyFrom(vtkMPICommunicator::GetWorldCommunicator());
  vtkMPIController::SafeDownCast(this->Controller)->SetCommunicator(comm);
  comm->Delete();
#endif

  if (this->Controller->GetNumberOfProcesses() > 1)
    {// !!!!! For unix, this was done when MPI was defined (even for 1
      // process). !!!!!
    this->Controller->CreateOutputWindow();
    }

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif
  
  int ret = 0;

  *partitionId = this->GetPartitionId();
  if (*partitionId == 0)
    {
    // Root process starts the GUI is there's any.
    ret = this->InitializeRoot(argc, argv);
    }
  else
    {
    // Called for every satellite.
    ret = this->InitializeSatellite(argc, argv);
    }

  return ret;
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::Finalize()
{
  if (this->GetPartitionId() == 0)
    {
    // The root tells all the satellites to finish.
    this->Controller->TriggerRMIOnAllChildren(
      vtkMultiProcessController::BREAK_RMI_TAG);
    }
  this->Superclass::Finalize();
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::InitializeRoot(int , char** )
{
  return 0;
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::RegisterSatelliteRMIs()
{
  this->Controller->AddRMI(vtkMPISelfConnectionProcessRMI, this,
    vtkMPISelfConnection::ROOT_SATELLITE_RMI_TAG);
  this->Controller->AddRMI(vtkMPISelfConnectionGatherInformationRMI, this,
    vtkMPISelfConnection::ROOT_SATELLITE_GATHER_INFORMATION_RMI_TAG);
}

//-----------------------------------------------------------------------------
int vtkMPISelfConnection::InitializeSatellite(int vtkNotUsed(argc), 
  char** vtkNotUsed(argv))
{
  this->RegisterSatelliteRMIs();

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
  // Send to all processess.
  this->SendStreamToServerNodeInternal(-1, stream);
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

  const unsigned char* data;
  size_t length;
  stream.GetData(&data, &length);
  if (remoteId == -1)
    {
    if (length != 0)
      {
      this->Controller->TriggerRMIOnAllChildren((void*)data,
        static_cast<int>(length),
        vtkMPISelfConnection::ROOT_SATELLITE_RMI_TAG);
      }
    // Process stream locally as well.
    this->ProcessStreamLocally(stream);
    }
  else if (remoteId == globalController->GetLocalProcessId())
    {
    this->ProcessStreamLocally(stream);
    }
  else
    {
    if (length != 0)
      {
      this->Controller->TriggerRMI(remoteId, (void*)data,
        static_cast<int>(length),
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
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Assign //dummy command.
    << info->GetClassName()
    << id << vtkClientServerStream::End;
  const unsigned char* sdata;
  size_t slength;
  stream.GetData(&sdata, &slength);
  
  this->Controller->TriggerRMIOnAllChildren((void*)sdata,
    static_cast<int>(slength),
    vtkMPISelfConnection::ROOT_SATELLITE_GATHER_INFORMATION_RMI_TAG);

  // Now, we must collect information from the satellites.
  this->CollectInformation(info);
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
    this->CollectInformation(info);
    }
  else
    {
    vtkErrorMacro("Could not gather information on Satellite.");
    // let the parent know.
    this->CollectInformation(NULL);
    }

  if (o) 
    { 
    o->Delete(); 
    }
}

//-----------------------------------------------------------------------------
void vtkMPISelfConnection::CollectInformation(vtkPVInformation* info)
{
  int myid = this->GetPartitionId();
  int children[2] = {2*myid + 1, 2*myid + 2};
  int parent = myid > 0? (myid-1)/2 : -1;
  int numProcs = this->GetNumberOfPartitions();

  // General rule is: receive from children and send to parent
  for (int childno=0; childno < 2; childno++)
    {
    int childid = children[childno];
    if (childid >= numProcs)
      {
      // Skip non-existant children.
      continue;
      }

    int length;
    this->Controller->Receive(&length, 1, childid, 
      vtkMPISelfConnection::ROOT_SATELLITE_INFO_LENGTH_TAG);
    if (length <= 0)
      {
      vtkErrorMacro("Failed to Gather Information from satellite no: " << childid);
      continue;
      }

    unsigned char* data = new unsigned char[length];
    this->Controller->Receive(data, length, childid,
      vtkMPISelfConnection::ROOT_SATELLITE_INFO_TAG);
    vtkClientServerStream stream;
    stream.SetData(data, length);
    vtkPVInformation* tempInfo = info->NewInstance();
    tempInfo->CopyFromStream(&stream);
    info->AddInformation(tempInfo);
    tempInfo->FastDelete();
    delete [] data; 
    }

  // Now send to parent, if parent is indeed valid.
  if (parent >= 0)
    {
    if (info)
      {
      vtkClientServerStream css;
      info->CopyToStream(&css);
      size_t length;
      const unsigned char* data;
      css.GetData(&data, &length);
      int len = static_cast<int>(length);
      this->Controller->Send(&len, 1, parent,
        vtkMPISelfConnection::ROOT_SATELLITE_INFO_LENGTH_TAG);
      this->Controller->Send(const_cast<unsigned char*>(data),
        length, parent, vtkMPISelfConnection::ROOT_SATELLITE_INFO_TAG);
      }
    else
      {
      int len = 0; 
      this->Controller->Send(&len, 1, parent,
        vtkMPISelfConnection::ROOT_SATELLITE_INFO_LENGTH_TAG);
      }
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
