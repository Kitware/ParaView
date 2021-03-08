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
#include "vtkCompositeMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
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

class vtkPVProgressHandler::RMICallback
{
public:
  // Collaboration needs rmi callback to cleanup the progress on secondary clients
  static void LocalCleanupPendingProgress(void* localArg, void* vtkNotUsed(remoteArg),
    int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
  {
    vtkPVProgressHandler* self = reinterpret_cast<vtkPVProgressHandler*>(localArg);
    if (self->GetEnableProgress())
    {
      self->LocalCleanupPendingProgress();
    }
  }

  // Collaboration needs rmi callback to refresh error and warning messages on secondary clients
  static void RefreshMessage(
    void* localArg, void* remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
  {
    vtkPVProgressHandler* self = reinterpret_cast<vtkPVProgressHandler*>(localArg);
    if (self->GetEnableProgress())
    {
      const unsigned char* rawdata = reinterpret_cast<unsigned char*>(remoteArg);
      vtkMultiProcessStream stream;
      stream.SetRawData(rawdata, remoteArgLength);

      std::string message;
      int type;
      stream >> message >> type;
      self->RefreshMessage(message.c_str(), type, /*is_local*/ false);
    }
  }
};

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

#ifdef PV_DISABLE_PROGRESS_HANDLING
    this->DisableProgressHandling = true;
#else
    this->DisableProgressHandling = false;
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
  this->Session = nullptr;
  this->Internals = new vtkInternals();
  this->LastProgress = 0;
  this->LastProgressText = nullptr;

  // use higher frequency for client while lower for server (or batch).
  this->ProgressInterval =
    vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_CLIENT ? 0.1 : 1.0;
  this->AddedHandlers = false;

  if (auto owindow = vtkOutputWindow::GetInstance())
  {
    owindow->AddObserver(vtkCommand::TextEvent, this, &vtkPVProgressHandler::OnMessageEvent);
    owindow->AddObserver(vtkCommand::ErrorEvent, this, &vtkPVProgressHandler::OnMessageEvent);
    owindow->AddObserver(vtkCommand::WarningEvent, this, &vtkPVProgressHandler::OnMessageEvent);
  }
}

//----------------------------------------------------------------------------
vtkPVProgressHandler::~vtkPVProgressHandler()
{
  this->SetLastProgressText(nullptr);
  this->SetSession(nullptr);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RegisterProgressEvent(vtkObject* object, int id)
{
  if (object && (object->IsA("vtkAlgorithm") || object->IsA("vtkExporter") ||
                  object->IsA("vtkSMAnimationSceneWriter")))
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

#ifndef PV_DISABLE_PROGRESS_HANDLING
  this->Internals->DisableProgressHandling = conn == nullptr;

  // In symmetric mode, we disable progress as it is not supported.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!this->Internals->DisableProgressHandling && pm)
  {
    this->Internals->DisableProgressHandling = pm->GetSymmetricMPIMode();
  }
#endif

  // NOTE: SetSession is called in constructor of vtkPVSession and that's too
  // early to be using any virtual methods on the session. So most
  // initialization happens in PrepareProgress() and AddHandlers().
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::AddHandlers()
{
  SKIP_IF_DISABLED();
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
      rs_controller->AddRMICallback(
        &RMICallback::LocalCleanupPendingProgress, this, vtkPVProgressHandler::CLEANUP_TAG_RMI);
      rs_controller->AddRMICallback(
        &RMICallback::RefreshMessage, this, vtkPVProgressHandler::MESSAGE_EVENT_TAG_RMI);
    }
    if (ds_controller)
    {
      ds_controller->GetCommunicator()->AddObserver(
        vtkCommand::WrongTagEvent, this, &vtkPVProgressHandler::OnWrongTagEvent);
      ds_controller->AddRMICallback(
        &RMICallback::LocalCleanupPendingProgress, this, vtkPVProgressHandler::CLEANUP_TAG_RMI);
      ds_controller->AddRMICallback(
        &RMICallback::RefreshMessage, this, vtkPVProgressHandler::MESSAGE_EVENT_TAG_RMI);
    }
  }
  this->AddedHandlers = true;
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::PrepareProgress()
{
  SKIP_IF_DISABLED();
  this->InvokeEvent(vtkCommand::StartEvent, this);
  this->Internals->EnableProgress = true;
}

//----------------------------------------------------------------------------
bool vtkPVProgressHandler::GetEnableProgress()
{
  return this->Internals->EnableProgress;
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
  if (client_controller != nullptr)
  {
    char temp = 0;
    vtkCompositeMultiProcessController* collabController =
      vtkCompositeMultiProcessController::SafeDownCast(client_controller);
    if (collabController)
    {
      // In collaboration, the secondary clients are cleaned up with RMI call
      vtkMultiProcessController* secondaryClientController;
      vtkMultiProcessController* activeClientController = collabController->GetActiveController();
      for (int i = 0; i < collabController->GetNumberOfControllers(); i++)
      {
        secondaryClientController = collabController->GetController(i);
        if (secondaryClientController != activeClientController)
        {
          secondaryClientController->TriggerRMI(1, vtkPVProgressHandler::CLEANUP_TAG_RMI);
        }
      }
    }
    client_controller->Send(&temp, 1, 1, vtkPVProgressHandler::CLEANUP_TAG);
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

  this->LocalCleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::LocalCleanupPendingProgress()
{
  SKIP_IF_DISABLED();
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
  if (this->Internals->ProgressTimer->GetElapsedTime() < this->ProgressInterval)
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
    std::vector<unsigned char> buffer(message_size);

    double le_progress = progress;
    vtkByteSwap::SwapLE(&progress);
    memcpy(buffer.data(), &le_progress, sizeof(double));

    memcpy(buffer.data() + sizeof(double), progress_text, progress_text_len);
    buffer[progress_text_len + sizeof(double)] = 0;

    vtkCompositeMultiProcessController* collabController =
      vtkCompositeMultiProcessController::SafeDownCast(client_controller);
    if (collabController)
    {
      // In collaboration, the all clients need to update their progress
      for (int i = 0; i < collabController->GetNumberOfControllers(); i++)
      {
        client_controller = collabController->GetController(i);
        client_controller->Send(
          buffer.data(), message_size, 1, vtkPVProgressHandler::PROGRESS_EVENT_TAG);
      }
    }
    else
    {
      client_controller->Send(
        buffer.data(), message_size, 1, vtkPVProgressHandler::PROGRESS_EVENT_TAG);
    }
  }

  this->SetLastProgressText(progress_text);
  this->LastProgress = static_cast<int>(progress * 100.0);
  // cout << "Progress: " << progress_text << " " << progress * 100 << endl;
  this->InvokeEvent(vtkCommand::ProgressEvent, this);
  this->SetLastProgressText(nullptr);
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
    memcpy(&len, ptr, sizeof(len));
    ptr += sizeof(len);

    auto rawdata = reinterpret_cast<const unsigned char*>(ptr);
    vtkMultiProcessStream stream;
    stream.SetRawData(rawdata, len);

    std::string message;
    int type;
    stream >> message >> type;
    this->RefreshMessage(message.c_str(), type, /*is_local*/ false);
    return true;
  }

  // We won't handle this event, let the default handler take care of it.
  // Default handler is defined in vtkPVSession::OnWrongTagEvent().
  if (tag == vtkPVProgressHandler::PROGRESS_EVENT_TAG)
  {
    // Secondary client PrepareProgress is never called by
    // vtkSMSessionClient::PrepareProgressInternal
    // so we call it when it receives a progress event from the server
    if (!this->GetEnableProgress())
    {
      this->PrepareProgress();
    }

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
void vtkPVProgressHandler::OnMessageEvent(vtkObject*, unsigned long eventid, void* calldata)
{
  switch (eventid)
  {
    case vtkCommand::WarningEvent:
    case vtkCommand::MessageEvent:
    case vtkCommand::ErrorEvent:
    case vtkCommand::TextEvent:
      this->RefreshMessage(reinterpret_cast<const char*>(calldata), eventid, /*is_local*/ true);
      break;
  }
}

//----------------------------------------------------------------------------
void vtkPVProgressHandler::RefreshMessage(const char* message, int etype, bool is_local)
{
  SKIP_IF_DISABLED();

  // to avoid recursive error messages.
  const auto old_value = vtkObject::GetGlobalWarningDisplay();
  vtkObject::GlobalWarningDisplayOff();

  // On server-root-nodes, send the message to the client.
  vtkMultiProcessController* client_controller = this->Session->GetController(vtkPVSession::CLIENT);
  if (client_controller != nullptr && message != nullptr)
  {
    vtkMultiProcessStream stream;
    stream << message << etype;
    auto uc_vector = stream.GetRawData();
    const int bytelength = static_cast<int>(uc_vector.size() * sizeof(unsigned char));

    // only true of server-nodes.
    vtkCompositeMultiProcessController* collabController =
      vtkCompositeMultiProcessController::SafeDownCast(client_controller);
    if (collabController)
    {
      // In collaboration, the messages of the secondary clients are updated with a RMI call
      vtkMultiProcessController* activeClientController = collabController->GetActiveController();
      for (int i = 0; i < collabController->GetNumberOfControllers(); i++)
      {
        auto secondaryClientController = collabController->GetController(i);
        if (secondaryClientController != activeClientController)
        {
          secondaryClientController->TriggerRMI(
            1, uc_vector.data(), bytelength, vtkPVProgressHandler::MESSAGE_EVENT_TAG_RMI);
        }
      }
    }
    client_controller->Send(uc_vector.data(), static_cast<int>(uc_vector.size()), 1,
      vtkPVProgressHandler::MESSAGE_EVENT_TAG);
  }

  if (!is_local)
  {
    switch (etype)
    {
      case vtkCommand::WarningEvent:
        vtkOutputWindowDisplayWarningText(message);
        break;

      case vtkCommand::ErrorEvent:
        vtkOutputWindowDisplayErrorText(message);
        break;

      case vtkCommand::TextEvent:
      case vtkCommand::MessageEvent:
        vtkOutputWindowDisplayText(message);
        break;
    }
  }

  vtkObject::SetGlobalWarningDisplay(old_value);
}
