/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIProcessModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVMPIProcessModule.h"
#include "vtkObjectFactory.h"
#include "vtkInstantiator.h"

#include "vtkClientServerStream.h"
#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkMultiProcessController.h"
#include "vtkPVApplication.h"
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
#include "vtkString.h"
#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVInformation.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"

#define VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG 397529

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMPIProcessModule);
vtkCxxRevisionMacro(vtkPVMPIProcessModule, "1.15.4.14");

int vtkPVMPIProcessModuleCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);



// external global variable.
vtkMultiProcessController *VTK_PV_UI_CONTROLLER = NULL;


//----------------------------------------------------------------------------
void vtkPVSlaveScript(void *localArg, void *remoteArg,
                      int vtkNotUsed(remoteArgLength),
                      int vtkNotUsed(remoteProcessId))
{
  vtkPVApplication *self = (vtkPVApplication *)(localArg);

  //cerr << " ++++ SlaveScript: " << ((char*)remoteArg) << endl;
  self->SimpleScript((char*)remoteArg);
}


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
    vtkPVApplication *pvApp = this->GetPVApplication();
    // This is for the SGI pipes option.
    pvApp->SetNumberOfPipes(numProcs);

#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
    pvApp->SetupTrapsForSignals(myId);
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

    if (pvApp->GetStartGUI())
      {
      pvApp->Script("wm withdraw .");
      pvApp->Start(this->ArgumentCount,this->Arguments);
      }
    else
      {
      pvApp->Exit();
      }
    this->ReturnValue = pvApp->GetExitStatus();
    }
  else
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    this->Controller->AddRMI(vtkPVSlaveScript, (void *)(pvApp),
                             VTK_PV_SLAVE_SCRIPT_RMI_TAG);
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

  this->Interpreter->SetLogFile("pvClient.out");

  // Go through the motions.
  // This indirection is not really necessary and is just to mimick the
  // threaded controller.
  this->Controller->SetSingleMethod(vtkPVMPIProcessModuleInit,(void*)(this));
  this->Controller->SingleMethodExecute();

  this->Controller->Finalize();

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


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::BroadcastSimpleScript(const char *str)
{
  int id, num;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  num = this->GetNumberOfPartitions();

  int len = vtkString::Length(str);
  if (!str || (len < 1))
    {
    return;
    }

  for (id = 1; id < num; ++id)
    {
    this->RemoteSimpleScript(id, str);
    }

  // Do reverse order, because 0 will block.
  this->Application->SimpleScript(str);
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::RemoteSimpleScript(int remoteId, const char *str)
{
  int length;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  // send string to evaluate.
  length = vtkString::Length(str) + 1;
  if (length <= 1)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() == remoteId)
    {
    this->Application->SimpleScript(str);
    return;
    }

  this->Controller->TriggerRMI(remoteId, const_cast<char*>(str),
                               VTK_PV_SLAVE_SCRIPT_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendStreamToClient()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendStreamToServer()
{
  this->SendStreamToServerInternal();
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendStreamToServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendStreamToClientAndServer()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendStreamToClientAndServerRoot()
{
  this->SendStreamToServerRoot();
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendStreamToServerInternal()
{
  // First send the command to the other server nodes.
  int numPartitions = this->GetNumberOfPartitions();
  for(int i=1; i < numPartitions; ++i)
    {
    this->SendStreamToServerNodeInternal(i);
    }

  // Now process the stream locally.
  this->Interpreter->ProcessStream(*this->ClientServerStream);
}

//----------------------------------------------------------------------------
void
vtkPVMPIProcessModule::SendStreamToServerNodeInternal(int remoteId)
{
  if(remoteId == this->Controller->GetLocalProcessId())
    {
    this->Interpreter->ProcessStream(*this->ClientServerStream);
    }
  else
    {
    const unsigned char* data;
    size_t length;
    this->ClientServerStream->GetData(&data, &length);
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
int vtkPVMPIProcessModule::LoadModuleInternal(const char* name)
{
  // Make sure we have a communicator.
  vtkMPICommunicator* communicator = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if(!communicator)
    {
    return 0;
    }

  // Try to load the module on the local process.
  int localResult = this->Interpreter->Load(name);

  // Gather load results to process 0.
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myid = this->Controller->GetLocalProcessId();
  int* results = new int[numProcs];
  communicator->Gather(&localResult, results, numProcs, 0);

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
int vtkPVMPIProcessModule::LoadModuleInternal(const char* name)
{
  return this->Superclass::LoadModuleInternal(name);
}
#endif
