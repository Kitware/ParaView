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
#include "vtkPVSynchronizedRenderWindows.h"

#include "vtkCommand.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRemoteConnection.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkPVServerInformation.h"
#include "vtkTilesHelper.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include <assert.h>

class vtkPVSynchronizedRenderWindows::vtkInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkRenderer> > VectorOfRenderers;

  struct RenderWindowInfo
    {
    int Size[2];
    int Position[2];
    unsigned long StartRenderTag;
    unsigned long EndRenderTag;
    vtkSmartPointer<vtkRenderWindow> RenderWindow;

    VectorOfRenderers Renderers;

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

  // Updates the viewport for all the renderers in the collection.
  void UpdateViewports(VectorOfRenderers& renderers, double viewport[4])
    {
    VectorOfRenderers::iterator iter;
    for (iter = renderers.begin(); iter != renderers.end(); ++iter)
      {
      (*iter)->SetViewport(viewport);
      }
    }

  vtkSmartPointer<vtkRenderWindow> SharedRenderWindow;
  unsigned int ActiveId;
};

//----------------------------------------------------------------------------
class vtkPVSynchronizedRenderWindows::vtkObserver : public vtkCommand
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

  vtkPVSynchronizedRenderWindows* Target;
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
    vtkPVSynchronizedRenderWindows* self =
      reinterpret_cast<vtkPVSynchronizedRenderWindows*>(localArg);
    self->Render(id);
    }
};

vtkStandardNewMacro(vtkPVSynchronizedRenderWindows);
vtkCxxRevisionMacro(vtkPVSynchronizedRenderWindows, "$Revision$");
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindows::vtkPVSynchronizedRenderWindows()
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
      "vtkPVSynchronizedRenderWindows cannot be used in the current\n"
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
vtkPVSynchronizedRenderWindows::~vtkPVSynchronizedRenderWindows()
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
void vtkPVSynchronizedRenderWindows::SetClientServerController(
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

  // Only the server processes needs to listen to SYNC_MULTI_RENDER_WINDOW_TAG
  // triggers from the client.
  if (controller && this->Mode == SERVER)
    {
    this->ClientServerRMITag =
      controller->AddRMICallback(::RenderRMI, this, SYNC_MULTI_RENDER_WINDOW_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetParallelController(
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

  // Only satellites listen to the SYNC_MULTI_RENDER_WINDOW_TAG
  // triggers from the root.
  if (controller &&
     (this->Mode == SERVER || this->Mode == BATCH) &&
     controller->GetLocalProcessId() > 0)
    {
    this->ParallelRMITag =
      controller->AddRMICallback(::RenderRMI, this, SYNC_MULTI_RENDER_WINDOW_TAG);
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVSynchronizedRenderWindows::NewRenderWindow()
{
  switch (this->Mode)
    {
  case BUILTIN:
  case CLIENT:
      {
      // client always creates new window for each view in the multi layout
      // configuration.
      vtkRenderWindow* window = vtkRenderWindow::New();
      window->DoubleBufferOn(); //FIXME;
      window->AlphaBitPlanesOn();
      return window;
      }

  case SERVER:
  case BATCH:
    // all views share the same render window.
    if (!this->Internals->SharedRenderWindow)
      {
      vtkRenderWindow* window = vtkRenderWindow::New();
      window->DoubleBufferOn(); //FIXME
      window->AlphaBitPlanesOn();
      // SwapBuffers should be ON only on root node in BATCH mode
      // or when operating in tile-display mode.
      bool swap_buffers = true;
      swap_buffers |= (this->Mode == BATCH &&
        this->ParallelController->GetLocalProcessId() == 0);
      //FIXME: for tile-displays
      window->SetSwapBuffers(swap_buffers? 1 : 0);
      this->Internals->SharedRenderWindow.TakeReference(window);
      }
    this->Internals->SharedRenderWindow->Register(this);
    return this->Internals->SharedRenderWindow;

  case INVALID:
    abort();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::AddRenderWindow(
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
    this->Internals->RenderWindows[id].StartRenderTag =
      renWin->AddObserver(vtkCommand::StartEvent, this->Observer);
    }
  if (!renWin->HasObserver(vtkCommand::EndEvent, this->Observer))
    {
    this->Internals->RenderWindows[id].EndRenderTag =
      renWin->AddObserver(vtkCommand::EndEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::RemoveRenderWindow(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    if (iter->second.StartRenderTag)
      {
      iter->second.RenderWindow->RemoveObserver(iter->second.StartRenderTag);
      }
    if (iter->second.EndRenderTag)
      {
      iter->second.RenderWindow->RemoveObserver(iter->second.EndRenderTag);
      }
    this->Internals->RenderWindows.erase(iter);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::AddRenderer(unsigned int id,
  vtkRenderer* renderer)
{
  this->Internals->RenderWindows[id].Renderers.push_back(renderer);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::RemoveAllRenderers(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    iter->second.Renderers.clear();
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVSynchronizedRenderWindows::GetRenderWindow(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    return iter->second.RenderWindow;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetWindowSize(unsigned int id,
  int width, int height)
{
  this->Internals->RenderWindows[id].Size[0] = width;
  this->Internals->RenderWindows[id].Size[1] = height;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetWindowPosition(unsigned int id,
  int px, int py)
{
  this->Internals->RenderWindows[id].Position[0] = px;
  this->Internals->RenderWindows[id].Position[1] = py;
}

//----------------------------------------------------------------------------
const int *vtkPVSynchronizedRenderWindows::GetWindowSize(unsigned int id)
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
const int *vtkPVSynchronizedRenderWindows::GetWindowPosition(unsigned int id)
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
void vtkPVSynchronizedRenderWindows::Render(unsigned int id)
{
  cout << "Rendering: " << id << endl;
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter == this->Internals->RenderWindows.end())
    {
    return;
    }

  // disable all other renderers.
  vtkRendererCollection* renderers = iter->second.RenderWindow->GetRenderers();
  renderers->InitTraversal();
  while (vtkRenderer* ren = renderers->GetNextItem())
    {
    ren->DrawOff();
    }

  vtkInternals::VectorOfRenderers::iterator iterRen;
  for (iterRen = iter->second.Renderers.begin();
    iterRen != iter->second.Renderers.end(); ++iterRen)
    {
    iterRen->GetPointer()->DrawOn();
    }

  // FIXME: When root node tries to communicate to the satellites the active
  // id, there's no clean way of determining the active id since on root node
  // the render window is shared among all views. Hence, we have this hack :(.
  this->Internals->ActiveId = id;
  iter->second.RenderWindow->Render();
  this->Internals->ActiveId = 0;
  cout << "Done Rendering: " << id << endl;
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
void vtkPVSynchronizedRenderWindows::HandleEndRender(vtkRenderWindow*)
{
  switch (this->Mode)
    {
  case CLIENT:
    this->ClientServerController->Barrier();
    break;

  case SERVER:
    this->ClientServerController->Barrier();
    break;

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

  vtkMultiProcessStream stream;

  // Pass in the information about the layout for all the windows.
  // TODO: This gets called when rendering each render window. However, this
  // information does not necessarily get invalidated that frequently. Can we be
  // smart about it?
  this->SaveWindowAndLayout(renWin, stream);

  this->ClientServerController->Broadcast(stream, 0);

  this->UpdateWindowLayout();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::RootStartRender(vtkRenderWindow* renWin)
{
  if (this->ClientServerController)
    {
    // * Get window layout from the server. $CODE_GET_LAYOUT_AND_UPDATE$
    vtkMultiProcessStream stream;
    this->ClientServerController->Broadcast(stream, 1);

    // Load the layout for all the windows from the client.
    this->LoadWindowAndLayout(renWin, stream);
    }

  // * Ensure layout i.e. all renders have correct viewports and hide inactive
  //   renderers.
  this->UpdateWindowLayout();

  if (this->ParallelController->GetNumberOfProcesses() <= 1)
    {
    return;
    }

  if (this->RenderEventPropagation)
    {
    // * Tell the satellites to start rendering.
    vtkMultiProcessStream stream;
    stream << this->Internals->ActiveId;
    vtkstd::vector<unsigned char> data;
    stream.GetRawData(data);
    this->ParallelController->TriggerRMIOnAllChildren(
      &data[0], static_cast<int>(data.size()), SYNC_MULTI_RENDER_WINDOW_TAG);
    }

  // * Send the layout and window params to the satellites.
  vtkMultiProcessStream stream;
  // Pass in the information about the layout for all the windows.
  // TODO: This gets called when rendering each render window. However, this
  // information does not necessarily get invalidated that frequently. Can we be
  // smart about it?
  this->SaveWindowAndLayout(renWin, stream);

  this->ParallelController->Broadcast(stream, 0);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SatelliteStartRender(
  vtkRenderWindow* renWin)
{
  // * Get window layout from the server. $CODE_GET_LAYOUT_AND_UPDATE$
  if (this->ParallelController)
    {
    vtkMultiProcessStream stream;
    this->ParallelController->Broadcast(stream, 0);

    // Load the layout for all the windows from the root.
    this->LoadWindowAndLayout(renWin, stream);
    }

  // * Ensure layout i.e. all renders have correct viewports and hide inactive
  //   renderers.
  this->UpdateWindowLayout();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SaveWindowAndLayout(
  vtkRenderWindow* window, vtkMultiProcessStream& stream)
{
  int full_size[2] = {0, 0};
  stream << static_cast<unsigned int>(this->Internals->RenderWindows.size());

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

  // Now save the window's tile scale and tile-viewport.
  int tileScale[2];
  double tileViewport[4];
  window->GetTileScale(tileScale);
  window->GetTileViewport(tileViewport);
  stream << tileScale[0] << tileScale[1]
         << tileViewport[0] << tileViewport[1] << tileViewport[2]
         << tileViewport[3]
         << window->GetDesiredUpdateRate();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::LoadWindowAndLayout(
  vtkRenderWindow* window, vtkMultiProcessStream& stream)
{
  unsigned int number_of_windows = 0;
  stream >> number_of_windows;

  if (number_of_windows != static_cast<unsigned int>(
      this->Internals->RenderWindows.size()))
    {
    vtkErrorMacro("Mismatch is render windows on different processes. "
      "Aborting for debugging purposes.");
    abort();
    }

  for (unsigned int cc=0; cc < number_of_windows; cc++)
    {
    int actual_size[2];
    int position[2];
    unsigned int key;
    stream >> key >> position[0] >> position[1]
           >> actual_size[0] >> actual_size[1];

    vtkInternals::RenderWindowsMap::iterator iter =
      this->Internals->RenderWindows.find(key);
    if (iter == this->Internals->RenderWindows.end())
      {
      vtkErrorMacro("Don't know anything about windows with key: " << key);
      continue;
      }

    iter->second.Size[0] = actual_size[0];
    iter->second.Size[1] = actual_size[1];
    iter->second.Position[0] = position[0];
    iter->second.Position[1] = position[1];
    }

  // Now load the full size.
  int full_size[2];
  stream >> full_size[0] >> full_size[1];

  // Now load the window's tile scale and tile-viewport.
  // tile-scale and viewport are overloaded. They are used when rendering large
  // images as well as when rendering in tile-display mode. The code here
  // handles the case when rendering large images.
  int tileScale[2];
  double tileViewport[4];
  double desiredUpdateRate;

  stream >> tileScale[0] >> tileScale[1]
         >> tileViewport[0] >> tileViewport[1] >> tileViewport[2]
         >> tileViewport[3]
         >> desiredUpdateRate;
  window->SetTileScale(tileScale);
  window->SetTileViewport(tileViewport);
  window->SetDesiredUpdateRate(desiredUpdateRate);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::UpdateWindowLayout()
{
  int full_size[2] = {0, 0};
  vtkInternals::RenderWindowsMap::iterator iter;


  // Compute full_size.
  for (iter = this->Internals->RenderWindows.begin();
    iter != this->Internals->RenderWindows.end(); ++iter)
    {
    const int *actual_size = iter->second.Size;
    const int *position = iter->second.Position;

    full_size[0] = full_size[0] > (position[0] + actual_size[0])?
      full_size[0] : position[0] + actual_size[0];
    full_size[1] = full_size[1] > (position[1] + actual_size[1])?
      full_size[1] : position[1] + actual_size[1];
    }

  switch (this->Mode)
    {
  case BUILTIN:
  case CLIENT:
    for (iter = this->Internals->RenderWindows.begin();
      iter != this->Internals->RenderWindows.end(); ++iter)
      {
      const int *actual_size = iter->second.Size;
      const int *position = iter->second.Position;
      // This class only supports full-viewports.
      double viewport[4] = {0, 0, 1, 1};
      this->Internals->UpdateViewports(
        iter->second.Renderers, viewport);
      }
    break;

  case SERVER:
  case BATCH:
      {
      // If we are in tile-display mode, we should update the tile-scale
      // and tile-viewport for the render window. That is required for the camera
      // as well as for the annotations to show correctly.
      vtkPVServerInformation* server_info =
        vtkProcessModule::GetProcessModule()->GetServerInformation(NULL);
      int tile_dims[2];
      tile_dims[0] = server_info->GetTileDimensions()[0];
      tile_dims[1] = server_info->GetTileDimensions()[1];
      bool in_tile_display_mode = (tile_dims[0] > 0 || tile_dims[1] > 0);
      // FIXME: at somepoint we need to set the tile-scale and tile-viewport
      // correctly on the render-window so that 2D annotations show up
      // correctly.
      tile_dims[0] = (tile_dims[0] == 0)? 1 : tile_dims[0];
      tile_dims[1] = (tile_dims[1] == 0)? 1 : tile_dims[1];
      this->Internals->SharedRenderWindow->SetTileScale(tile_dims);
      if (in_tile_display_mode)
        {
        // FIXME: handle full-screen case
        this->Internals->SharedRenderWindow->SetSize(400, 400);
        }
      else
        {
        this->Internals->SharedRenderWindow->SetSize(full_size);
        //this->Internals->SharedRenderWindow->SetPosition(0, 0);
        }

      // Iterate over all (logical) windows and set the viewport on the
      // renderers to reflect the position and size of the actual window on the
      // client side.
      for (iter = this->Internals->RenderWindows.begin();
        iter != this->Internals->RenderWindows.end(); ++iter)
        {
        const int *actual_size = iter->second.Size;
        const int *position = iter->second.Position;

        // This class only supports full-viewports.
        double viewport[4];
        viewport[0] = position[0]/static_cast<double>(full_size[0]);
        viewport[1] = position[1]/static_cast<double>(full_size[1]);
        viewport[2] = (position[0] + actual_size[0])/
          static_cast<double>(full_size[0]);
        viewport[3] = (position[1] + actual_size[1])/
          static_cast<double>(full_size[1]);

        // This viewport is the viewport for the renderers treating the all the
        // tiles as one large display.
        cout << "Current Viewport:" << viewport[0]
          << ", " << viewport[1] << ", " << viewport[2]
          << ", " << viewport[3] << endl;
        this->Internals->UpdateViewports(
          iter->second.Renderers, viewport);
        }
      }
    break;

  case INVALID:
    abort();
    }

}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
