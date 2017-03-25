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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVOptions.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkTimerLog.h"

#include <map>
#include <string>

// define this variable to disable progress all together. This may be useful to
// doing really large runs.
//#define PV_DISABLE_PROGRESS_HANDLING

#define SKIP_IF_DISABLED()                                                                         \
  if (this->Internals->DisableProgressHandling)                                                    \
  {                                                                                                \
    return;                                                                                        \
  }

inline const char* vtkGetProgressText(vtkObjectBase* o)
{
  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(o);
  if (alg && alg->GetProgressText())
  {
    return alg->GetProgressText();
  }

  return o->GetClassName();
}

//----------------------------------------------------------------------------
class vtkPVProgressHandler::vtkInternals
{
public:
  typedef std::map<void*, int> MapOfObjectToInt;
  MapOfObjectToInt RegisteredObjects;

  // Disables progress all together.
  bool DisableProgressHandling;

  // Flag indicating if progresses are currently being "observed", i.e. we are
  // between calls to PrepareProgress() and CleanupPendingProgress().
  bool EnableProgress;

  vtkNew<vtkTimerLog> ProgressTimer;
  vtkInternals()
  {
    this->EnableProgress = false;
    this->DisableProgressHandling = false;

#ifdef PV_DISABLE_PROGRESS_HANDLING
    this->DisableProgressHandling = true;
#else
    // In symmetric mode, we disable progress although that's not really
    // necessary anymore. Since the progress is no longer collected from anyone
    // but the root node, it really doesn't matter if progress is enabled.
    if (vtkProcessModule* pm = vtkProcessModule::GetProcessModule())
    {
      this->DisableProgressHandling = pm->GetSymmetricMPIMode();
    }
#endif
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
  this->LastProgress = 0;
  this->LastProgressText = NULL;
  this->LastMessage = NULL;
  this->ProgressFrequency = 1.0; // seconds
  this->AddedHandlers = false;

  // Add observer to MessageEvents.
  vtkOutputWindow::GetInstance()->AddObserver(
    vtkCommand::MessageEvent, this, &vtkPVProgressHandler::OnMessageEvent);
}

//----------------------------------------------------------------------------
vtkPVProgressHandler::~vtkPVProgressHandler()
{
  this->SetLastProgressText(NULL);
  this->SetLastMessage(NULL);
  this->SetSession(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RegisterProgressEvent(vtkObject* object, int id)
{
  if (object && (object->IsA("vtkAlgorithm") || object->IsA("vtkKdTree") ||
                  object->IsA("vtkExporter") || object->IsA("vtkSMAnimationSceneWriter")))
  {
    this->Internals->RegisteredObjects[object] = id;
    object->AddObserver(vtkCommand::ProgressEvent, this, &vtkPVProgressHandler::OnProgressEvent);
    object->AddObserver(vtkCommand::MessageEvent, this, &vtkPVProgressHandler::OnMessageEvent);
  }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::SetSession(vtkPVSession* conn)
{
  if (this->Session != conn)
  {
    this->Session = conn;
    this->AddedHandlers = false;
    this->Modified();
  }

  // NOTE: SetSession is called in constructor of vtkPVSession and that's too
  // early to be using any virtual methods on the session. So most
  // initialization happens in PrepareProgress().
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrepareProgress()
{
  if (this->AddedHandlers == false)
  {
    vtkMultiProcessController* ds_controller =
      this->Session->GetController(vtkPVSession::DATA_SERVER_ROOT);
    vtkMultiProcessController* rs_controller =
      this->Session->GetController(vtkPVSession::RENDER_SERVER_ROOT);
    if (rs_controller && rs_controller != ds_controller)
    {
      rs_controller->GetCommunicator()->AddObserver(
        vtkCommand::WrongTagEvent, this, &vtkPVProgressHandler::OnWrongTagEvent);
    }
    if (ds_controller)
    {
      ds_controller->GetCommunicator()->AddObserver(
        vtkCommand::WrongTagEvent, this, &vtkPVProgressHandler::OnWrongTagEvent);
    }
  }
  this->AddedHandlers = true;

#ifndef PV_DISABLE_PROGRESS_HANDLING
  SKIP_IF_DISABLED();
  this->Internals->DisableProgressHandling = this->Session ? this->Session->IsMultiClients() : true;
#endif

  SKIP_IF_DISABLED();

  this->InvokeEvent(vtkCommand::StartEvent, this);
  this->Internals->EnableProgress = true;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::CleanupPendingProgress()
{
  SKIP_IF_DISABLED();

  if (!this->Internals->EnableProgress)
  {
#ifndef NDEBUG
    // warn only in debug builds.
    vtkWarningMacro("Non-critical internal ParaView Error: "
                    "Got request for cleanup pending progress after being cleaned up");
#endif
    return;
  }

  vtkMultiProcessController* mpiController = vtkMultiProcessController::GetGlobalController();
  if (mpiController && mpiController->GetNumberOfProcesses() > 1)
  {
    mpiController->Barrier();
  }

  // On the server-node (render-server root or data-server root), we send a
  // reply back to the client saying we are done cleaning up.
  // Now, if there exists a client-controller, send reply to the client.
  vtkMultiProcessController* client_controller = this->Session->GetController(vtkPVSession::CLIENT);
  if (client_controller != NULL)
  {
    char temp = 0;
    client_controller->Send(&temp, 1, 1, CLEANUP_TAG);
  }

  // On client-node (or builtin client) mode, we wait till the server-root-nodes
  // send us a cleanup message. While we wait on this receive, we will
  // consume any progress messages sent by the server.
  vtkMultiProcessController* ds_controller =
    this->Session->GetController(vtkPVSession::DATA_SERVER_ROOT);
  vtkMultiProcessController* rs_controller =
    this->Session->GetController(vtkPVSession::RENDER_SERVER_ROOT);
  if (ds_controller)
  {
    char temp = 0;
    ds_controller->Receive(&temp, 1, 1, CLEANUP_TAG);
  }
  if (rs_controller && rs_controller != ds_controller)
  {
    char temp = 0;
    rs_controller->Receive(&temp, 1, 1, CLEANUP_TAG);
  }

  this->Internals->EnableProgress = false;
  this->InvokeEvent(vtkCommand::EndEvent, this);
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::OnProgressEvent(vtkObject* caller, unsigned long eventid, void* calldata)
{
  SKIP_IF_DISABLED();
  if (!this->Internals->EnableProgress || eventid != vtkCommand::ProgressEvent)
  {
    return;
  }

  // Try to clamp frequent progress events.
  this->Internals->ProgressTimer->StopTimer();
  // cout <<"Elapsed: " << this->Internals->ProgressTimer->GetElapsedTime() <<
  //  endl;
  if (this->Internals->ProgressTimer->GetElapsedTime() < this->ProgressFrequency)
  {
    return;
  }

  this->Internals->ProgressTimer->StartTimer();

  double progress = *reinterpret_cast<double*>(calldata);
  if (progress < 0 || progress > 1.0)
  {
#ifndef NDEBUG
    // warn only in debug builds.
    vtkWarningMacro(<< caller->GetClassName() << " reported invalid progress (" << progress
                    << "). Value must be between [0, 1]. Clamping to this range.");
#endif
    progress = (progress < 0) ? 0 : progress;
    progress = (progress > 1.0) ? 1.0 : progress;
  }

  std::string text = ::vtkGetProgressText(caller);
  this->RefreshProgress(text.c_str(), progress);
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RefreshProgress(const char* progress_text, double progress)
{
  // On server-root-nodes, send the progress message to the client.
  vtkMultiProcessController* client_controller = this->Session->GetController(vtkPVSession::CLIENT);
  if (client_controller)
  {
    // only true of server-nodes.
    int progress_text_len = static_cast<int>(strlen(progress_text));
    int message_size = progress_text_len + sizeof(double) + sizeof(char);
    unsigned char* buffer = new unsigned char[message_size];

    double le_progress = progress;
    vtkByteSwap::SwapLE(&progress);
    memcpy(buffer, &le_progress, sizeof(double));

    memcpy(buffer + sizeof(double), progress_text, progress_text_len);
    buffer[progress_text_len + sizeof(double)] = 0;

    client_controller->Send(buffer, message_size, 1, vtkPVProgressHandler::PROGRESS_EVENT_TAG);
    delete[] buffer;
  }

  this->SetLastProgressText(progress_text);
  this->LastProgress = static_cast<int>(progress * 100.0);
  // cout << "Progress: " << progress_text << " " << progress * 100 << endl;
  this->InvokeEvent(vtkCommand::ProgressEvent, this);
  this->SetLastProgressText(NULL);
  this->LastProgress = 0;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVProgressHandler::OnWrongTagEvent(vtkObject*, unsigned long eventid, void* calldata)
{
  if (eventid != vtkCommand::WrongTagEvent)
  {
    return false;
  }

  int tag = -1;
  int len = -1;
  const char* data = reinterpret_cast<const char*>(calldata);
  const char* ptr = data;
  memcpy(&tag, ptr, sizeof(tag));

  if (tag == vtkPVProgressHandler::MESSAGE_EVENT_TAG)
  {
    ptr += sizeof(tag);
    ptr += sizeof(len);
    this->RefreshMessage(ptr);
    return true;
  }

  // We won't handle this event, let the default handler take care of it.
  // Default handler is defined in vtkPVSession::OnWrongTagEvent().
  if (tag == vtkPVProgressHandler::PROGRESS_EVENT_TAG)
  {
    ptr += sizeof(tag);
    memcpy(&len, ptr, sizeof(len));
    ptr += sizeof(len);

    double progress = 0.0;
    memcpy(&progress, ptr, sizeof(double));
    ptr += sizeof(progress);

#ifdef VTK_WORDS_BIGENDIAN
    // Progress is sent in little-endian form. We need to convert it to  big
    // endian.
    vtkByteSwap::SwapLE(&progress);
#endif

    this->RefreshProgress(ptr, progress);
    return true;
  }

  // We won't handle this event, let the default handler take care of it.
  // Default handler is defined in vtkPVSession::OnWrongTagEvent().
  return false;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::OnMessageEvent(
  vtkObject* vtkNotUsed(caller), unsigned long eventid, void* calldata)
{
  if (eventid != vtkCommand::MessageEvent)
  {
    return;
  }

  const char* message = reinterpret_cast<const char*>(calldata);
  this->RefreshMessage(message);
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RefreshMessage(const char* message)
{
  // On server-root-nodes, send the message to the client.
  vtkMultiProcessController* client_controller = this->Session->GetController(vtkPVSession::CLIENT);
  if (client_controller != NULL && message != NULL)
  {
    // only true of server-nodes.
    client_controller->Send(
      message, strlen(message) + 1, 1, vtkPVProgressHandler::MESSAGE_EVENT_TAG);
  }

  this->SetLastMessage(message);
  this->InvokeEvent(vtkCommand::MessageEvent, const_cast<char*>(message));
}
