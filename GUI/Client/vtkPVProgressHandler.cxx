/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProgressHandler.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProgressHandler.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWindow.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkClientServerInterpreter.h"

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h" // Needed for vtkMPICommunicator::Request
#endif 

#define PVAPPLICATION_PROGRESS_TAG 31415

#include <vtkstd/map>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProgressHandler);
vtkCxxRevisionMacro(vtkPVProgressHandler, "1.7");

//----------------------------------------------------------------------------
//****************************************************************************
class vtkPVProgressHandlerInternal
{
public:
  typedef vtkstd::vector<int> VectorOfInts;
  typedef vtkstd::map<int, VectorOfInts> MapOfVectorsOfInts;
  typedef vtkstd::map<vtkObject*, int> MapOfObjectIds;

  MapOfVectorsOfInts ProgressMap;
  MapOfObjectIds ObjectIdsMap;

#ifdef VTK_USE_MPI
  vtkMPICommunicator::Request ProgressRequest;
#endif
};

//****************************************************************************
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
vtkPVProgressHandler::vtkPVProgressHandler()
{
  this->Internals = new vtkPVProgressHandlerInternal;
  this->Application = 0;
  this->ProgressType = vtkPVProgressHandler::NotSet;
  this->ProgressPending = 0;
  this->MinimumProgressInterval = 0.5;

  this->ProgressTimer = vtkTimerLog::New();
  this->ProgressTimer->StartTimer();

  this->MPIController = 0;
  this->SocketController = 0;

  this->ReceivingProgressReports = 0;
}

//----------------------------------------------------------------------------
vtkPVProgressHandler::~vtkPVProgressHandler()
{
  if (this->ProgressPending)
    {
    this->Internals->ProgressRequest.Cancel();
    }
  this->ProgressTimer->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RegisterProgressEvent(vtkObject* po, int id)
{
  this->Internals->ObjectIdsMap[po] = id;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::DetermineProgressType(vtkPVApplication* app)
{
  if ( this->ProgressType != vtkPVProgressHandler::NotSet )
    {
    return;
    }
  vtkDebugMacro("Determine progress type");

  int client = app->GetClientMode();
  int server = app->GetServerMode();
  int local_process = 0;
  int num_processes = 1;
#ifdef VTK_USE_MPI
  this->MPIController = vtkMPIController::SafeDownCast(
    app->GetProcessModule()->GetController());
  if ( this->MPIController )
    {
    local_process = this->MPIController->GetLocalProcessId();
    num_processes = this->MPIController->GetNumberOfProcesses();
    }
#endif
  this->SocketController = app->GetSocketController();

  if ( client )
    {
    this->ProgressType = vtkPVProgressHandler::ClientServerClient;
    }
  else
    {
    if ( server )
      {
      if ( local_process > 0 )
        {
        this->ProgressType = vtkPVProgressHandler::SatelliteMPI;
        }
      else
        {
        if ( num_processes > 1 )
          {
          this->ProgressType = vtkPVProgressHandler::ClientServerServerMPI;
          }
        else
          {
          this->ProgressType = vtkPVProgressHandler::ClientServerServer;
          }
        }
      }
    else
      {
      if ( local_process > 0 )
        {
        this->ProgressType = vtkPVProgressHandler::SatelliteMPI;
        }
      else
        {
        if ( num_processes > 1 )
          {
          this->ProgressType = vtkPVProgressHandler::SingleProcessMPI;
          }
        else
          {
          this->ProgressType = vtkPVProgressHandler::SingleProcess;
          }
        }
      }
    }
  if ( this->ProgressType == vtkPVProgressHandler::NotSet )
    {
    vtkErrorMacro("Internal ParaView errorr. Progress is not set.");
    vtkPVApplication::Abort();
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::InvokeProgressEvent(vtkPVApplication* app,
  vtkObject *o, int val, const char* filter)
{
  this->DetermineProgressType(app);

  if ( !this->ReceivingProgressReports && 
    this->ProgressType != vtkPVProgressHandler::SingleProcess && 
    this->ProgressType != vtkPVProgressHandler::ClientServerClient )
    {
    return;
    }

  switch( this->ProgressType )
    {
  case vtkPVProgressHandler::SingleProcess:
    vtkDebugMacro("This is the gui and I got the progress: " << val);
    this->LocalDisplayProgress(app, o->GetClassName(), val);
    break;
  case vtkPVProgressHandler::SingleProcessMPI:
    vtkDebugMacro("This is the gui and I got progress. I need to handle children. " << val);
    this->InvokeRootNodeProgressEvent(app, o, val);
    break;
  case vtkPVProgressHandler::SatelliteMPI:
    vtkDebugMacro("I am satellite and I need to send progress to the node 0: " << val);
    this->InvokeSatelliteProgressEvent(app, o, val);
    break;
  case vtkPVProgressHandler::ClientServerClient:
    // No need to receive since the event handling will take care of that
    vtkDebugMacro("This is gui and I got the progress from the server: " << val);
    if ( !filter )
      {
      filter = o->GetClassName();
      }
    this->LocalDisplayProgress(app, filter, val);
    break;
  case vtkPVProgressHandler::ClientServerServer:
    vtkDebugMacro("This is non-mpi server and I need to send progress to client: " << val);
    this->InvokeRootNodeServerProgressEvent(app, o, val);
    break;
  case vtkPVProgressHandler::ClientServerServerMPI:
    vtkDebugMacro("This is mpi server and I need to send progress to client: " << val);
    this->InvokeRootNodeServerProgressEvent(app, o, val);
    break;
  default:
    vtkErrorMacro("Internal ParaView error. Progress type is set to some unknown value");
    vtkPVApplication::Abort();
    }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::LocalDisplayProgress(
  vtkPVApplication* app, const char* filter, int progress)
{
  if ( !filter )
    {
    abort();
    }
  if(!app->GetMainWindow())
    {
    return;
    }
  app->GetMainWindow()->SetProgress(filter, progress);
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::InvokeRootNodeProgressEvent(
  vtkPVApplication* app, vtkObject* o, int myprogress)
{
  int id = -1;
  int progress = -1;
  vtkPVProgressHandlerInternal::MapOfObjectIds::iterator it 
    = this->Internals->ObjectIdsMap.find(o);
  if ( it != this->Internals->ObjectIdsMap.end() )
    {
    this->HandleProgress(0, it->second, myprogress);
    }
  while ( this->ReceiveProgressFromSatellite(&id, &progress) )
    {
    vtkClientServerID nid;
    nid.ID = id;
    vtkObjectBase* base = app->GetProcessModule()->GetInterpreter()->GetObjectFromID(nid);
    if ( base )
      {
      this->LocalDisplayProgress(app, base->GetClassName(), progress);
      }
    else
      {
      //vtkErrorMacro("Internal ParaView error. Got progress from unknown object id" << id << ".");
      //vtkPVApplication::Abort();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::InvokeRootNodeServerProgressEvent(
  vtkPVApplication* app, vtkObject* o, int myprogress)
{
  int id = -1;
  int progress = -1;
  vtkPVProgressHandlerInternal::MapOfObjectIds::iterator it 
    = this->Internals->ObjectIdsMap.find(o);
  if ( it != this->Internals->ObjectIdsMap.end() )
    {
    this->HandleProgress(0, it->second, myprogress);
    }
  while ( this->ReceiveProgressFromSatellite(&id, &progress) );
  vtkClientServerID nid;
  nid.ID = id;
  vtkObjectBase* base = app->GetProcessModule()->GetInterpreter()->GetObjectFromID(nid);
  if ( base )
    {
    char buffer[1024];
    buffer[0] = progress;
    sprintf(buffer+1, "%s", base->GetClassName());
    int len = strlen(buffer+1) + 2;
    this->SocketController->Send(buffer, len, 1, PVAPPLICATION_PROGRESS_TAG);
    }
  else
    {
    //vtkErrorMacro("Internal ParaView error. Got progress from unknown object id" << id << ".");
    //vtkPVApplication::Abort();
    }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::InvokeSatelliteProgressEvent(
  vtkPVApplication*, vtkObject* o, int progress)
{
#ifdef VTK_USE_MPI
  if (this->ProgressPending && this->Internals->ProgressRequest.Test())
    {
    this->ProgressPending=0;
    }
#endif

  this->ProgressTimer->StopTimer();
  double delT = this->ProgressTimer->GetElapsedTime();

  if (delT > this->MinimumProgressInterval && progress)
    {
    this->ProgressTimer->StartTimer();
    if (!this->ProgressPending)
      {
      vtkPVProgressHandlerInternal::MapOfObjectIds::iterator it 
        = this->Internals->ObjectIdsMap.find(o);
      if ( it != this->Internals->ObjectIdsMap.end() )
        {
#ifdef VTK_USE_MPI
        this->Progress[0] = this->MPIController->GetLocalProcessId();
        this->Progress[1] = it->second;
        this->Progress[2] = progress;

        this->MPIController->NoBlockSend(
          this->Progress, 
          3, 0, PVAPPLICATION_PROGRESS_TAG, 
          this->Internals->ProgressRequest);
#endif
        this->ProgressPending=1;
        }
      else
        {
        vtkErrorMacro("Internal ParaView error: Got progresss from something not observed.");
        vtkPVApplication::Abort();
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVProgressHandler::ReceiveProgressFromSatellite(int *id, int* progress)
{
  int rec = 0;
#ifdef VTK_USE_MPI
  if ( this->MPIController )
    {
    if (!this->ProgressPending)
      {
      this->MPIController->NoBlockReceive(this->Progress, 3, 
        vtkMultiProcessController::ANY_SOURCE,
        PVAPPLICATION_PROGRESS_TAG, 
        this->Internals->ProgressRequest);
      }
    if (this->Internals->ProgressRequest.Test() )
      {
      this->HandleProgress(this->Progress[0],
        this->Progress[1],
        this->Progress[2]);
      rec ++;
      this->MPIController->NoBlockReceive(this->Progress, 3, 
        vtkMultiProcessController::ANY_SOURCE, 
        PVAPPLICATION_PROGRESS_TAG, 
        this->Internals->ProgressRequest);
      }
    this->ProgressPending=1;
    }
#endif
  int minprog = 101;
  int filter = -1;
  vtkPVProgressHandlerInternal::MapOfVectorsOfInts::iterator it;
  vtkPVProgressHandlerInternal::VectorOfInts::iterator vit;
  for ( it = this->Internals->ProgressMap.begin();
    it != this->Internals->ProgressMap.end();
    it ++ )
    {
    for ( vit = it->second.begin(); 
      vit != it->second.end();
      vit ++ )
      {
      if ( *vit < minprog )
        {
        minprog = *vit;
        filter = it->first;
        }
      }
    }
  *progress = minprog;
  *id = filter;
  return rec;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::HandleProgress(int processid, int filterid, int progress)
{
  vtkPVProgressHandlerInternal::VectorOfInts* vect 
    = &this->Internals->ProgressMap[filterid];
#ifdef VTK_USE_MPI
  vect->resize(this->MPIController->GetNumberOfProcesses());
#else
  vect->resize(processid < (int)vect->size()?vect->size():processid+1);
#endif
  (*vect)[processid] = progress;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrepareProgress(vtkPVApplication* app)
{
  vtkDebugMacro("Prepare progress receiving");
  this->DetermineProgressType(app);
  vtkPVProgressHandlerInternal::MapOfVectorsOfInts::iterator it;
  vtkPVProgressHandlerInternal::VectorOfInts::iterator vit;
  for ( it = this->Internals->ProgressMap.begin();
    it != this->Internals->ProgressMap.end();
    it ++ )
    {
    for ( vit = it->second.begin(); 
      vit != it->second.end();
      vit ++ )
      {
      *vit = 200;
      }
    }

  this->ReceivingProgressReports = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::CleanupPendingProgress(vtkPVApplication* app)
{
  if ( !this->ReceivingProgressReports )
    {
    vtkErrorMacro("Internal ParaView Error: Got request for cleanup pending progress after being cleaned up");
    vtkPVApplication::Abort();
    }
  vtkDebugMacro("Cleanup all pending progress events");
  int id = -1;
  int progress = -1;
  switch ( this->ProgressType )
    {
    case vtkPVProgressHandler::SingleProcessMPI:
    case vtkPVProgressHandler::ClientServerServerMPI:
      while ( this->ReceiveProgressFromSatellite(&id, &progress) ) 
        {
        vtkClientServerID nid;
        nid.ID = id;
        vtkObjectBase* base = 
          app->GetProcessModule()->GetInterpreter()->GetObjectFromID(nid, 1);
        if ( base )
          {
          if ( this->ProgressType == vtkPVProgressHandler::SingleProcessMPI )
            {
            this->LocalDisplayProgress(app, base->GetClassName(), progress);
            }
          else
            {
            char buffer[1024];
            buffer[0] = progress;
            sprintf(buffer+1, "%s", base->GetClassName());
            int len = strlen(buffer+1) + 2;
            this->SocketController->Send(buffer, len, 1, PVAPPLICATION_PROGRESS_TAG);
            }
          }
        }
    }
  this->ReceivingProgressReports = 0;
  // WARNING
  // should really synchronize and cleanup all progresses.
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
