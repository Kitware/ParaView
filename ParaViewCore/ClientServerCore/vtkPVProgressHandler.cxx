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

#include "vtkAlgorithm.h"
#include "vtkByteSwap.h"
#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkTimerLog.h"
#include "vtkPVConfig.h" // For PARAVIEW_USE_MPI

#ifdef PARAVIEW_USE_MPI
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

#include <vector>
#include <deque>
#include <string>
#include <map>

#if defined(_WIN32) && !defined(__CYGWIN__)
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif

// define this variable to disable progress all together. This may be useful to
// doing really large runs.
// #define PV_DISABLE_PROGRESS_HANDLING

#define SKIP_IF_DISABLED()\
  if (this->Internals->DisableProgressHandling) { return; }

inline const char* vtkGetProgressText(vtkObjectBase* o)
{
  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(o);
  if (alg && alg->GetProgressText())
    {
    return alg->GetProgressText();
    }

  return o->GetClassName();
}

// When in parallel, we want to collect progress reported by all processess and
// then report the lowest progress. That's managed by this class.
class vtkProgressStore
{
  class vtkRow
    {
  public:
    int ObjectID;
    std::vector<double> Progress;
    std::vector<std::string> Text;

    // Returns true if there's some progress to report for this row.
    // NOTE: This is a non-idempotent method.
    bool Report(std::string& txt, double& progress)
      {
      progress = VTK_DOUBLE_MAX;
      for (unsigned int cc=0; cc < this->Progress.size(); cc++)
        {
        if (this->Progress[cc] >= 0.0 && this->Progress[cc] < progress)
          {
          progress = this->Progress[cc];
          txt = this->Text[cc];
          if (this->Progress[cc] >= 1.0)
            {
            // Report 100% only once.
            this->Progress[cc] = -1;
            }
          }
        }
      return (progress < VTK_DOUBLE_MAX);
      }

    // Returns if this row has any valid progress.
    bool ReadyToClean()
      {
      for (unsigned int cc=0; cc < this->Progress.size(); cc++)
        {
        if (this->Progress[cc] != -1)
          {
          return false;
          }
        }
      return true;
      }
    };
  typedef std::deque<vtkRow> ListOfRows;
  ListOfRows Rows;

  vtkRow& Find(int objectId)
    {
    ListOfRows::iterator iter;
    for (iter = this->Rows.begin(); iter != this->Rows.end(); ++iter)
      {
      if (iter->ObjectID == objectId)
        {
        return (*iter);
        }
      }
    int numProcs = this->GetNumberOfProcesses();
    vtkRow row;
    row.ObjectID = objectId; 
    this->Rows.push_back(row);
    this->Rows.back().Progress.resize(numProcs, -1);
    this->Rows.back().Text.resize(numProcs);
    return this->Rows.back();
    }

  int GetNumberOfProcesses()
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    if (pm->GetPartitionId()==0 && pm->GetNumberOfLocalPartitions() > 1)
      {
      return pm->GetNumberOfLocalPartitions();
      }
    return 2;
    }

public:
  void AddLocalProgress(int objectId, const std::string& txt,
    double progress)
    {
    vtkRow& row = this->Find(objectId);
    row.Text[0] = txt;
    row.Progress[0] = progress;
    }

  void AddRemoteProgress(int remoteId, int objectId,
    const std::string& txt, double progress)
    {
    vtkRow& row = this->Find(objectId);
    row.Text[remoteId] = txt;
    row.Progress[remoteId] = progress;
    }

  // Returns the progress to report. This is a non-idempotent method (esp. when
  // any of the filters is reporting 100%).
  bool GetProgress(int &objectId, std::string& txt, double& progress)
    {
    ListOfRows::iterator iter;
    for (iter = this->Rows.begin(); iter != this->Rows.end(); ++iter)
      {
      if (iter->Report(txt, progress))
        {
        objectId = iter->ObjectID;
        if (iter->ReadyToClean())
          {
          this->Rows.erase(iter);
          }
        return true;
        }
      }
    return false;
    }

  void Clear()
    {
    this->Rows.clear();
    }
};


class vtkPVProgressHandler::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New() { return new vtkObserver(); }
  void SetTarget(vtkPVProgressHandler* target)
    {
    this->Target = target;
    }
  
  virtual void Execute(vtkObject* wdg, unsigned long event, void* calldata)
    {
    if (this->Target && event == vtkCommand::ProgressEvent)
      {
      this->Target->OnProgressEvent(wdg, *reinterpret_cast<double*>(calldata));
      }
    if (this->Target && event == vtkCommand::WrongTagEvent)
      {
      if (this->Target->OnWrongTagEvent(calldata))
        {
        this->AbortFlagOn();
        }
      }
    }
protected:
  vtkObserver() { this->Target = 0; }
  vtkPVProgressHandler* Target;
};

#define ASYNCREQUESTDATA_MAX_SIZE (129+sizeof(int)*3)

//----------------------------------------------------------------------------
class vtkPVProgressHandler::vtkInternals
{
public:
  typedef std::map<vtkObject*, int> MapOfObjectToInt;
  MapOfObjectToInt RegisteredObjects;

  vtkProgressStore ProgressStore;
#ifdef PARAVIEW_USE_MPI
  vtkMPICommunicator::Request AsyncRequest;
#endif
  bool AsyncRequestValid;
  char AsyncRequestData[ASYNCREQUESTDATA_MAX_SIZE];
  bool EnableProgress;
  bool ForceAsyncRequestReceived;

  bool DisableProgressHandling;

  vtkTimerLog* ProgressTimer;
  vtkInternals()
    {
    this->AsyncRequestValid = false;
    this->EnableProgress = false;
    this->ForceAsyncRequestReceived = false;
    this->ProgressTimer = vtkTimerLog::New();
    this->ProgressTimer->StartTimer();
#ifdef PV_DISABLE_PROGRESS_HANDLING
    this->DisableProgressHandling = true;
#else
    this->DisableProgressHandling = false;
#endif
    }

  ~vtkInternals()
    {
    this->ProgressTimer->Delete();
    this->ProgressTimer = 0;
    }

  int GetIDFromObject(vtkObject* obj)
    {
    if (this->RegisteredObjects.find(obj) != this->RegisteredObjects.end())
      {
      return this->RegisteredObjects[obj];
      }
    return 0;
    }
};

vtkStandardNewMacro(vtkPVProgressHandler);
//----------------------------------------------------------------------------
vtkPVProgressHandler::vtkPVProgressHandler()
{
  this->Session = 0;
  this->Internals = new vtkInternals();
  this->Observer = vtkPVProgressHandler::vtkObserver::New();
  this->Observer->SetTarget(this);
  this->LastProgress = 0;
  this->LastProgressText = NULL;
  this->ProgressFrequency = 2.0; // seconds
  this->AddedHandlers = false;
}

//----------------------------------------------------------------------------
vtkPVProgressHandler::~vtkPVProgressHandler()
{
  this->SetLastProgressText(NULL);
  this->SetSession(0);
  delete this->Internals;
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->Observer = 0;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RegisterProgressEvent(vtkObject* object, int id)
{
  if (object && (object->IsA("vtkAlgorithm") || object->IsA("vtkKdTree")))
    {
    this->Internals->RegisteredObjects[object] = id;
    object->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::SetSession(vtkPVSession* conn)
{
  if (this->Session != conn)
    {
    this->Session = conn;
    this->Modified();
    }

  // NOTE: SetSession is called in constructor of vtkPVSession and that's too
  // early to be using any virtual methods on the session. So most
  // initialization happens in PrepareProgress().
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrepareProgress()
{
#ifndef PV_DISABLE_PROGRESS_HANDLING
  this->Internals->DisableProgressHandling =
    this->Session? this->Session->IsMultiClients() : true;
#endif

  SKIP_IF_DISABLED();

  this->InvokeEvent(vtkCommand::StartEvent, this);
  this->Internals->EnableProgress = true;

  if (this->AddedHandlers == false)
    {
    vtkMultiProcessController* ds_controller = this->Session->GetController(
      vtkPVSession::DATA_SERVER_ROOT);
    vtkMultiProcessController* rs_controller = this->Session->GetController(
      vtkPVSession::RENDER_SERVER_ROOT);
    if (rs_controller && rs_controller != ds_controller)
      {
      rs_controller->GetCommunicator()->AddObserver(
        vtkCommand::WrongTagEvent, this->Observer);
      }
    if (ds_controller)
      {
      ds_controller->GetCommunicator()->AddObserver(
        vtkCommand::WrongTagEvent, this->Observer);
      }
    }
  this->AddedHandlers = true;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::CleanupPendingProgress()
{
  SKIP_IF_DISABLED();

  if (!this->Internals->EnableProgress)
    {
    vtkErrorMacro("Non-critical internal ParaView Error: "
      "Got request for cleanup pending progress after being cleaned up");
    return;
    }

  // The role of this method to  consume all progress messages that may have
  // been put in the message queue by other processes.

  // Receive progress from all children and then send the "Cleaned" signal
  // to the parent (if any).

  // Receive progress from satellites, if any.
  this->CleanupSatellites();

  // Now, if there exists a client-controller, send reply to the client.
  vtkMultiProcessController* client_controller =
    this->Session->GetController(vtkPVSession::CLIENT);
  if (client_controller != NULL)
    {
    char temp=0;
    client_controller->Send(&temp, 1, 1, CLEANUP_TAG);
    }

  // Now if there exists a server-controller, wait for reply from the servers.

  // Receive CLEANUP_TAG from Server. While we wait on this receive, we will
  // consume any progress messages sent by the server.
  vtkMultiProcessController* ds_controller = this->Session->GetController(
    vtkPVSession::DATA_SERVER_ROOT);
  vtkMultiProcessController* rs_controller = this->Session->GetController(
    vtkPVSession::RENDER_SERVER_ROOT);
  if (ds_controller)
    {
    char temp=0;
    ds_controller->Receive(&temp, 1, 1, CLEANUP_TAG);
    }
  if (rs_controller && rs_controller != ds_controller)
    {
    char temp=0;
    rs_controller->Receive(&temp, 1, 1, CLEANUP_TAG);
    }

  this->Internals->ProgressStore.Clear();
  this->Internals->EnableProgress = false;
  this->InvokeEvent(vtkCommand::EndEvent, this);
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::CleanupSatellites()
{
#ifdef PARAVIEW_USE_MPI
  vtkMPIController* controller = vtkMPIController::SafeDownCast(
    vtkMultiProcessController::GetGlobalController());
  if (controller && controller->GetNumberOfProcesses() > 1)
    {
    int myId = controller->GetLocalProcessId();
    int numProcs = controller->GetNumberOfProcesses();

    // As we wait on these receives, we will consume any progress messages sent
    // by the satellites
    if (myId == 0)
      {
      for (int cc=1; cc < numProcs; cc++)
        {
        int temp = 0;
        controller->Receive(&temp, 1,
          vtkMultiProcessController::ANY_SOURCE,
          vtkPVProgressHandler::CLEANUP_TAG);
        }
      }
    else
      {
      // Send the CLEANUP_TAG to the root node.
      controller->Send(&myId, 1, 0, vtkPVProgressHandler::CLEANUP_TAG);
      }
    if (this->Internals->AsyncRequestValid)
      {
      this->Internals->AsyncRequestValid = false;
      if (!this->Internals->ForceAsyncRequestReceived &&
        !this->Internals->AsyncRequest.Test())
        {
        this->Internals->AsyncRequest.Cancel();
        }
      this->Internals->ForceAsyncRequestReceived = false;
      }
    }
#endif
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::OnProgressEvent(vtkObject* obj,
  double progress)
{
  SKIP_IF_DISABLED();

  if (!this->Internals->EnableProgress)
    {
    return;
    }
  std::string text = ::vtkGetProgressText(obj);
  if (text.size() > 128)
    {
    vtkWarningMacro("Progress text is tuncated to 128 characters.");
    text = text.substr(0, 128);
    }
  int id = this->Internals->GetIDFromObject(obj);
  this->Internals->ProgressStore.AddLocalProgress(id, text, progress);
  this->RefreshProgress();
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RefreshProgress()
{
  int id;
  double progress;
  std::string text;

  // NOTE: All the sends/receives have to be non-blocking.

  // Collect progress from all satellites.
  this->GatherProgress();

  vtkMultiProcessController* client_controller =
    this->Session->GetController(vtkPVSession::CLIENT);
  vtkMultiProcessController* ds_controller =
    this->Session->GetController(vtkPVSession::DATA_SERVER_ROOT);
  vtkMultiProcessController* rs_controller =
    this->Session->GetController(vtkPVSession::RENDER_SERVER_ROOT);
  if (rs_controller && rs_controller == ds_controller)
    {
    rs_controller = NULL;
    }

  if (client_controller)
    {
    this->SendProgressToClient(client_controller);
    }
  if (ds_controller)
    {
    this->ReceiveProgressFromServer(ds_controller);
    }
  if (rs_controller)
    {
    this->ReceiveProgressFromServer(rs_controller);
    }

  if ( (this->Session->GetProcessRoles() & vtkPVSession::CLIENT) ==
    vtkPVSession::CLIENT)
    {
    // Display progress locally.
    if (this->Internals->ProgressStore.GetProgress(id, text, progress))
      {
      this->SetLocalProgress(static_cast<int>(progress*100.0), text.c_str());
      }
    }
}

//----------------------------------------------------------------------------
bool vtkPVProgressHandler::GetIsRoot()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return (pm->GetPartitionId() == 0);
}

//----------------------------------------------------------------------------
bool vtkPVProgressHandler::ReportProgress(double progress)
{
  this->Internals->ProgressTimer->StopTimer();
  if (progress <= 0.0 || progress >= 1.0 ||
    this->Internals->ProgressTimer->GetElapsedTime() > this->ProgressFrequency)
    {
    this->Internals->ProgressTimer->StartTimer();
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::SendProgressToClient(
  vtkMultiProcessController* controller)
{
  // Called on Root node of the server process to send the progress to the
  // client.
  int id;
  std::string text;
  double progress;
  if (this->Internals->ProgressStore.GetProgress(id, text, progress))
    {
    if (this->ReportProgress(progress))
      {
      char buffer[1026];
      buffer[0] = static_cast<int>(progress*100.0);
      SNPRINTF(buffer+1, 1024, "%s", text.c_str());
      int len = static_cast<int>(strlen(buffer+1)) + 2;
      controller->Send(buffer, len, 1, vtkPVProgressHandler::PROGRESS_EVENT_TAG);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::ReceiveProgressFromServer(vtkMultiProcessController* vtkNotUsed(ctr))
{
  // Nothing to do here. We cannot do non-block receive on SocketController.
  // All progress events from the server will be received as a consequence of
  // vtkCommand::WrongTag event in which case HandleServerProgress() gets
  // called.
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::HandleServerProgress(int progress, const char* text)
{
  SKIP_IF_DISABLED();

  //cout << "Progress: " << (text? text : "(none)") << "==" << progress << endl;
  this->SetLastProgressText(text);
  this->LastProgress = progress;
  this->InvokeEvent(vtkCommand::ProgressEvent, this);
  this->SetLastProgressText(NULL);
  this->LastProgress = 0;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::SetLocalProgress(int progress, const char* text)
{
  if (this->ReportProgress(progress/100.0))
    {
    this->SetLastProgressText(text);
    this->LastProgress = progress;
    this->InvokeEvent(vtkCommand::ProgressEvent, this);
    this->SetLastProgressText(NULL);
    this->LastProgress = 0;
    //cout << "Progress: " << (text? text : "(none)") << "==" << progress << endl;
    }
}

//----------------------------------------------------------------------------
int vtkPVProgressHandler::GatherProgress()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetNumberOfLocalPartitions() == 1)
    {
    return 0;
    }

  if (pm->GetPartitionId() == 0)
    {
    // Get any progress events from satellites.
    return this->ReceiveProgressFromSatellites();
    }
  else
    {
    // Send local progress to the root node. 
    this->SendProgressToRoot();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::MarkAsyncRequestReceived()
{
  this->Internals->ForceAsyncRequestReceived = true;
}

//----------------------------------------------------------------------------
int vtkPVProgressHandler::ReceiveProgressFromSatellites()
{
  int req_count = 0;
#ifdef PARAVIEW_USE_MPI
  if (this->Internals->AsyncRequestValid &&
    (this->Internals->ForceAsyncRequestReceived || 
     this->Internals->AsyncRequest.Test()))
    {
    int pid, oid, progress;

    memcpy(&pid, this->Internals->AsyncRequestData, sizeof(int));
    vtkByteSwap::SwapLE(&pid);

    memcpy(&oid, this->Internals->AsyncRequestData + sizeof(int), sizeof(int));
    vtkByteSwap::SwapLE(&oid);

    memcpy(&progress, this->Internals->AsyncRequestData + sizeof(int)*2, sizeof(int));
    vtkByteSwap::SwapLE(&progress);

    std::string text = reinterpret_cast<const char*>(
      this->Internals->AsyncRequestData + sizeof(int)*3);
    //cout << "----Received: " << text.c_str() << ": " << progress << endl;

    this->Internals->ProgressStore.AddRemoteProgress(
      pid, oid, text, progress/100.0); 
    req_count++;
    this->Internals->AsyncRequestValid = false;
    this->Internals->ForceAsyncRequestReceived =false;
    }

  vtkMPIController* controller = vtkMPIController::SafeDownCast(
    vtkMultiProcessController::GetGlobalController());
  if (this->Internals->AsyncRequestValid == false)
    {
    controller->NoBlockReceive(this->Internals->AsyncRequestData,
      ASYNCREQUESTDATA_MAX_SIZE,
      vtkMultiProcessController::ANY_SOURCE,
      vtkPVProgressHandler::PROGRESS_EVENT_TAG,
      this->Internals->AsyncRequest);
    this->Internals->AsyncRequestValid = true;
    return req_count + this->ReceiveProgressFromSatellites();
    }
#endif
  return req_count;
}

//----------------------------------------------------------------------------
vtkMPICommunicatorOpaqueRequest* vtkPVProgressHandler::GetAsyncRequest()
{
#ifdef PARAVIEW_USE_MPI
  if (this->Internals->AsyncRequestValid &&
    !this->Internals->ForceAsyncRequestReceived)
    {
    return this->Internals->AsyncRequest.Req;
    }
#endif
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::SendProgressToRoot()
{
#ifdef PARAVIEW_USE_MPI
  if (this->Internals->AsyncRequestValid &&
    this->Internals->AsyncRequest.Test())
    {
    // This simply marks whether the previously sent message caused any errors.
    // This does not imply that the message has been received by the root node.
    this->Internals->AsyncRequestValid =false;
    }

  if (this->Internals->AsyncRequestValid == false)
    {
    double progress;
    int id;
    std::string text;
    if (this->Internals->ProgressStore.GetProgress(id, text, progress))
      {
      if (this->ReportProgress(progress))
        {
        vtkByteSwap::SwapLE(&id);

        int i_progress = static_cast<int>(progress*100.0);
        vtkByteSwap::SwapLE(&i_progress);

        int myId = vtkProcessModule::GetProcessModule()->GetPartitionId();
        vtkByteSwap::SwapLE(&myId);

        char buf[20];
        sprintf(buf, "(%d)", myId);
        text += buf;


        memcpy(this->Internals->AsyncRequestData, &myId, sizeof(int));
        memcpy(this->Internals->AsyncRequestData + sizeof(int), &id, sizeof(int));
        memcpy(this->Internals->AsyncRequestData + sizeof(int)*2, &i_progress,
          sizeof(int));
        memcpy(this->Internals->AsyncRequestData + sizeof(int)*3, text.c_str(),
          text.size()+1);

        vtkIdType messageSize = sizeof(int)*3 + text.size() + 1;
        vtkMPIController* controller = vtkMPIController::SafeDownCast(
          vtkMultiProcessController::GetGlobalController());
        controller->NoBlockSend(this->Internals->AsyncRequestData,
          messageSize, 0,
          vtkPVProgressHandler::PROGRESS_EVENT_TAG,
          this->Internals->AsyncRequest);
        //cout << "Sent To Root: " << text.c_str() << ": " << i_progress<< endl;
        this->Internals->AsyncRequestValid = true;
        }
      }
    }
#endif
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVProgressHandler::OnWrongTagEvent(void* calldata)
{
  int tag = -1;
  int len = -1;
  const char* data = reinterpret_cast<const char*>(calldata);
  const char* ptr = data;
  memcpy(&tag, ptr, sizeof(tag));

  // We won't handle this event, let the default handler take care of it.
  // Default handler is defined in vtkPVSession::OnWrongTagEvent().
  if ( tag != vtkPVProgressHandler::PROGRESS_EVENT_TAG)
    {
    return false;
    }

  ptr += sizeof(tag);
  memcpy(&len, ptr, sizeof(len));
  ptr += sizeof(len);
  char val = -1;
  val = *ptr;
  ptr ++;
  if ( val < 0 || val > 100 )
    {
    vtkErrorMacro("Received progress not in the range 0 - 100: " << (int)val);
    }
  else
    {
    this->HandleServerProgress(val, ptr);
    }
  return true;
}
