/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSynchronizedRenderWindowsClient.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"

class vtkPVSynchronizedRenderWindowsClient::vtkInternals
{
public:
  struct RenderWindowInfo
    {
    int Size[2];
    int Position[2];
    unsigned long StartRenderTag;
    unsigned long EndRenderTag;
    vtkSmartPointer<vtkRenderWindow> RenderWindow;
    vtkstd::vector<vtkRenderer> Renderers;

    RenderWindowInfo()
      {
      this->Size[0] = this->Size[1] = 0;
      this->Position[0] = this->Position[1] = 0;
      this->StartRenderTag = this->EndRenderTag = 0;
      }
    };

  typedef vtkstd::map<unsigned int, RenderWindowInfo> RenderWindowsMap;
  RenderWindowsMap RenderWindows;

  unsigned int GetKey(vtkRenderWindow* win)
    {
    RenderWindowsMap::iterator iter;
    for (iter = this->RenderWindows.begin(); iter != this->RenderWindows.end();
      ++iter)
      {
      if (iter->second.RenderWindow == win)
        {
        return iter->first;
        }
      }
    return 0;
    }

  vtkSmartPointer<vtkRenderWindow> SharedRenderWindow;
};

//----------------------------------------------------------------------------
class vtkPVSynchronizedRenderWindowsClient::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
    {
    vtkObserver* obs = new vtkObserver();
    obs->Target = NULL;
    return obs;
    }

  virtual void Execute(vtkObject *ocaller, unsigned long eventId, void *)
    {
    vtkRenderWindow* renWin = vtkRenderWindow::SafeDownCast(ocaller);
    if (this->Target && this->Target->GetEnabled())
      {
      switch (eventId)
        {
      case vtkCommand::StartEvent:
        this->Target->HandleStartRender(renWin);
        break;

      case vtkCommand::EndEvent:
        this->Target->HandleEndRender(renWin);
        break;

      case vtkCommand::AbortCheckEvent:
        this->Target->HandleAbortRender(renWin);
        break;
        }
      }
    }

  vtkPVSynchronizedRenderWindowsClient* Target;
};

//----------------------------------------------------------------------------
namespace
{
  void RenderRMI(void *localArg,
    void *remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
    {
    vtkMultiProcessStream stream;
    stream.SetRawData(reinterpret_cast<unsigned char*>(remoteArg),
      remoteArgLength);
    unsigned int id = 0;
    stream >> id;
    vtkPVSynchronizedRenderWindowsClient* self =
      reinterpret_cast<vtkPVSynchronizedRenderWindowsClient*>(localArg);
    vtkRenderWindow* window = self->GetRenderWindow(id);
    if (window)
      {
      window->Render();
      }
    }
};

vtkStandardNewMacro(vtkPVSynchronizedRenderWindowsClient);
vtkCxxRevisionMacro(vtkPVSynchronizedRenderWindowsClient, "$Revision$");
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindowsClient::vtkPVSynchronizedRenderWindowsClient()
{
  this->Mode = INVALID;
  this->ClientServerController = 0;
  this->ParallelController = 0;
  this->ClientServerRMITag = 0;
  this->ParallelRMITag = 0;
  this->Internals = new vtkInternals();
  this->Observer = vtkObserver::New();
  this->Observer->Target = this;
  this->Enabled = true;
  this->RenderEventPropagation = true;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro(
      "vtkPVSynchronizedRenderWindowsClient cannot be used in the current\n"
      "setup. Aborting for debugging purposes.");
    abort();
    }

  if (pm->GetActiveRemoteConnection() == NULL)
    {
    this->Mode = BUILTIN;
    if (pm->GetNumberOfLocalPartitions() > 1)
      {
      this->Mode = BATCH;
      }
    }
  else if (pm->GetActiveRemoteConnection()->IsA("vtkClientConnection"))
    {
    this->Mode = SERVER;
    }
  else if (pm->GetActiveRemoteConnection()->IsA("vtkServerConnection"))
    {
    this->Mode = CLIENT;
    }

  // Setup the controllers for the communication.
  switch (this->Mode)
    {
  case BUILTIN:
    // nothing to do.
    break;

  case BATCH:
    this->SetParallelController(vtkMultiProcessController::GetGlobalController());
    break;

  case SERVER:
    this->SetParallelController(vtkMultiProcessController::GetGlobalController());
    this->SetClientServerController(pm->GetActiveRenderServerSocketController());
    break;

  case CLIENT:
    this->SetClientServerController(pm->GetActiveRenderServerSocketController());
    break;

  default:
    vtkErrorMacro("Invalid process type.");
    abort();
    }
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindowsClient::~vtkPVSynchronizedRenderWindowsClient()
{
  this->SetClientServerController(0);
  this->SetParallelController(0);

  delete this->Internals;
  this->Internals = 0;

  this->Observer->Target = NULL;
  this->Observer->Delete();
  this->Observer = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SetClientServerController(
  vtkMultiProcessController* controller)
{
  if (this->ClientServerController == controller)
    {
    return;
    }

  if (this->ClientServerController && this->ClientServerRMITag)
    {
    this->ClientServerController->RemoveRMICallback(this->ClientServerRMITag);
    }

  vtkSetObjectBodyMacro(
    ClientServerController, vtkMultiProcessController, controller);
  this->ClientServerRMITag = 0;

  // Only the server processes needs to listen to SYNC_RENDER_TAG triggers from
  // the client.
  if (controller && this->Mode == SERVER)
    {
    this->ClientServerRMITag =
      controller->AddRMICallback(::RenderRMI, this, SYNC_RENDER_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SetParallelController(
  vtkMultiProcessController* controller)
{
  if (this->ParallelController == controller)
    {
    return;
    }

  if (this->ParallelController && this->ParallelRMITag)
    {
    this->ParallelController->RemoveRMICallback(this->ParallelRMITag);
    }

  vtkSetObjectBodyMacro(
    ParallelController, vtkMultiProcessController, controller);
  this->ParallelRMITag = 0;

  // Only satellites listen to the SYNC_RENDER_TAG triggers from the root.
  if (controller &&
     (this->Mode == SERVER || this->Mode == BATCH) &&
     controller->GetLocalProcessId() > 0)
    {
    this->ParallelRMITag =
      controller->AddRMICallback(::RenderRMI, this, SYNC_RENDER_TAG);
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVSynchronizedRenderWindowsClient:NewRenderWindow()
{
  switch (this->Mode)
    {
  case BUILTIN:
  case CLIENT:
    // client always creates new window for each view in the multi layout
    // configuration.
    return vtkRenderWindow::New();

  case SERVER:
  case BATCH:
    // all views share the same render window.
    if (!this->Internals->SharedRenderWindow)
      {
      this->Internals->SharedRenderWindow =
        vtkSmartPointer<vtkRenderWindow>::New();
      }
    this->Internals->SharedRenderWindow->Register(this);
    return this->Internals->SharedRenderWindow;

  case INVALID:
    abort();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::AddRenderWindow(
  unsigned int id, vtkRenderWindow* renWin)
{
  assert(renWin != NULL && id != 0);

  if (this->Internals->RenderWindows.find(id) !=
    this->Internals->RenderWindows.end() &&
    this->Internals->RenderWindows[id].RenderWindow != NULL)
    {
    vtkErrorMacro("ID for render window already in use: " << id);
    return;
    }

  this->Internals->RenderWindows[id].RenderWindow = renWin;
  if (!renWin->HasObserver(vtkCommand::StartEvent, this->Observer))
    {
    this->Internals->RenderWindow[id].StartRenderTag =
      renWin->AddObserver(vtkCommand::StartEvent, this->Observer);
    }
  if (!renWin->HasObserver(vtkCommand::EndEvent, this->Observer))
    {
    this->Internals->RenderWindow[id].EndRenderTag =
      renWin->AddObserver(vtkCommand::EndEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::RemoveRenderWindow(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    if (iter->second.StartRenderTag)
      {
      iter->second.RemoveObserver(iter->second.StartRenderTag);
      }
    if (iter->second.EndRenderTag)
      {
      iter->second.RemoveObserver(iter->second.EndRenderTag);
      }
    this->Internals->RenderWindows.erase(iter);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::AddRenderer(unsigned int id,
  vtkRenderer* renderer)
{
  this->Internals->RenderWindows[id].Renderers.push_back(renderer);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::RemoveAllRenderers(unsigned int id,
  vtkRenderer* renderer)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    iter->second.Renderers.clear();
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVSynchronizedRenderWindowsClient::GetRenderWindow(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    return this->Internals->RenderWindows.RenderWindow;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SetWindowSize(unsigned int id,
  int width, int height)
{
  this->Internals->RenderWindow[id].Size[0] = width;
  this->Internals->RenderWindow[id].Size[1] = height;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SetWindowPosition(unsigned int id,
  int px, int py)
{
  this->Internals->RenderWindow[id].Position[0] = px;
  this->Internals->RenderWindow[id].Position[1] = py;
}

//----------------------------------------------------------------------------
const int *vtkPVSynchronizedRenderWindowsClient::GetWindowSize(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    return iter->second.Size;
    }
  return NULL;
}

//----------------------------------------------------------------------------
const int *vtkPVSynchronizedRenderWindowsClient::GetWindowPosition(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    return iter->second.Position;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::HandleStartRender(vtkRenderWindow* renWin)
{
  // This method is called when a render window starts rendering. This is called
  // on all processes. The response is different on all the processes based on
  // the mode/configuration.

  switch (this->Mode)
    {
  case CLIENT:
    this->ClientStartRender(renWin);
    break;

  case SERVER:
  case BATCH:
    if (this->ParallelController->GetLocalProcessId() == 0)
      {
      // root node.
      this->RootStartRender(renWin);
      }
    else
      {
      this->SatelliteStartRender(renWin);
      }
    break;

  case BUILTIN:
  default:
    return;
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::ClientStartRender(vtkRenderWindow* renWin)
{
  // In client-server mode, the client needs to collect the window layouts and
  // then the active window specific parameters.
  if (this->RenderEventPropagation)
    {
    // Tell the server-root to start rendering.
    vtkMultiProcessStream stream;
    stream << this->Internals->GetKey(renWin);
    vtkstd::vector<unsigned char> data;
    stream.GetRawData(data);
    this->ClientServerController->TriggerRMIOnAllChildren(
      &data[0], static_cast<int>(data.size()), SYNC_MULTI_RENDER_WINDOW_TAG);
    }
  // when this->RenderEventPropagation, we assume that the server activates the
  // correct render window somehow.

  // Pass in the information about the layout for all the windows.
  // TODO: This gets called when rendering each render window. However, this
  // information does not necessarily get invalidated that frequently. Can we be
  // smart about it?
  this->SaveWindowLayout(stream);

  // TODO: We may want to pass tile-scale/tile-viewport and desired update rate
  // to the server as well (similar to
  // vtkSynchronizedRenderWindows::RenderWindowInfo).

  vtkMultiProcessStream stream;
  windowInfo.Save(stream);
  this->ClientServerController->Broadcast(stream, 0);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::RootStartRender(vtkRenderWindow* renWin)
{
  if (this->ClientServerController)
    {
    // * Get window layout from the server. $CODE_GET_LAYOUT_AND_UPDATE$
    }

  // * Ensure layout i.e. all renders have correct viewports and hide inactive
  //   renderers.
  this->EnsureLayout(renWin);

  if (this->ParallelController->GetNumberOfProcesses() <= 1)
    {
    return;
    }

  if (this->RenderEventPropagation)
    {
    // * Tell the satellites to start rendering.
    vtkMultiProcessStream stream;
    stream << this->Internals->GetKey(renWin);
    vtkstd::vector<unsigned char> data;
    stream.GetRawData(data);
    this->ParallelController->TriggerRMIOnAllChildren(
      &data[0], static_cast<int>(data.size()), SYNC_MULTI_RENDER_WINDOW_TAG);
    }

  // * Send the layout and window params to the satellites.
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SatelliteStartRender(
  vtkRenderWindow* renWin)
{
  // * Get window layout from the server. $CODE_GET_LAYOUT_AND_UPDATE$

  // * Ensure layout i.e. all renders have correct viewports and hide inactive
  //   renderers.
  this->EnsureLayout(renWin);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SaveWindowLayout(
  vtkMultiProcessStream& stream)
{
  int full_size[2] = {0, 0};
  stream << static_cast<unsigned int>(this->Internals->RenderWindowsMap.size());

  vtkInternals::RenderWindowsMap::iterator iter;
  for (iter = this->Internals->RenderWindows.begin();
    iter != this->Internals->RenderWindows.end(); ++iter)
    {
    const int *actual_size = iter->second.Size;
    const int *position = iter->second.Position;

    full_size[0] = full_size[0] > (position[0] + actual_size[0])?
      full_size[0] : position[0] + actual_size[0];
    full_size[1] = full_size[1] > (position[1] + actual_size[1])?
      full_size[1] : position[1] + actual_size[1];
    stream << iter->first << position[0] << position[1]
      << actual_size[0] << actual_size[1];
    }
  // Now push the full size.
  stream << full_size[0] << full_size[1];
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
