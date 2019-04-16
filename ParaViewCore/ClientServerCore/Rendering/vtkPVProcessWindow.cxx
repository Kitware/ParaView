/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessWindow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProcessWindow.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <string>

//============================================================================
namespace
{
static vtkRenderWindow* ProcessWindowSingleton = nullptr;
static int PVProcessWindowSingletonCleanerNiftyCounter = 0;
static void DeleteProcessWindowSingleton()
{
  if (::ProcessWindowSingleton != nullptr)
  {
    ::ProcessWindowSingleton->Delete();
    ::ProcessWindowSingleton = nullptr;
  }
}
}

//----------------------------------------------------------------------------
vtkPVProcessWindowSingletonCleaner::vtkPVProcessWindowSingletonCleaner()
{
  ++::PVProcessWindowSingletonCleanerNiftyCounter;
}

//----------------------------------------------------------------------------
vtkPVProcessWindowSingletonCleaner::~vtkPVProcessWindowSingletonCleaner()
{
  --::PVProcessWindowSingletonCleanerNiftyCounter;
  if (::PVProcessWindowSingletonCleanerNiftyCounter == 0)
  {
    ::DeleteProcessWindowSingleton();
  }
}

//============================================================================

//----------------------------------------------------------------------------
vtkPVProcessWindow::vtkPVProcessWindow()
{
}

//----------------------------------------------------------------------------
vtkPVProcessWindow::~vtkPVProcessWindow()
{
}

//----------------------------------------------------------------------------
void vtkPVProcessWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVProcessWindow::GetRenderWindow()
{
  if (::ProcessWindowSingleton == nullptr)
  {
    ::ProcessWindowSingleton = vtkPVProcessWindow::NewWindow();
  }
  return ::ProcessWindowSingleton;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVProcessWindow::NewWindow()
{
  static bool attempted_once = false;
  if (attempted_once)
  {
    return nullptr;
  }
  attempted_once = true;

  auto pm = vtkProcessModule::GetProcessModule();
  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_SERVER:
    case vtkProcessModule::PROCESS_RENDER_SERVER:
    case vtkProcessModule::PROCESS_BATCH:
      break;

    case vtkProcessModule::PROCESS_CLIENT:
    case vtkProcessModule::PROCESS_DATA_SERVER:
    case vtkProcessModule::PROCESS_INVALID:
      // no process-window in these mode.
      return nullptr;
  }

  auto pvoptions = pm->GetOptions();

  vtkRenderWindow* window = nullptr;
  if (pvoptions->GetIsInTileDisplay())
  {
    window = vtkPVProcessWindow::NewTileDisplayWindow();
  }
  else if (pvoptions->GetIsInCave())
  {
    window = vtkPVProcessWindow::NewCAVEWindow();
  }
  else if (!pvoptions->GetForceOnscreenRendering() || pvoptions->GetForceOffscreenRendering())
  {
    // this may be a headless window if ParaView was built with headless
    // capabilities.
    vtkSmartPointer<vtkRenderWindow> renWindow =
      vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
    renWindow->Register(nullptr);
    window = renWindow;
  }
  else
  {
    window = vtkRenderWindow::New();
  }

  window->DoubleBufferOn();
  window->AlphaBitPlanesOn();

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);
  window->AddRenderer(renderer);

  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_SERVER:
    case vtkProcessModule::PROCESS_RENDER_SERVER:
      window->SetWindowName(
        (std::string("ParaView Server #") + std::to_string(pm->GetPartitionId())).c_str());
      break;

    case vtkProcessModule::PROCESS_BATCH:
      window->SetWindowName(
        (std::string("ParaView Batch #") + std::to_string(pm->GetPartitionId())).c_str());
      break;
    default:
      break;
  }

  // when the process module is finalized, let's destroy the
  // ProcessWindowSingleton as well. Delaying it until libraries are unloaded
  // can cause issues with tools like `apitrace`.
  vtkNew<vtkCallbackCommand> observer;
  observer->SetCallback(
    [](vtkObject*, unsigned long, void*, void*) { ::DeleteProcessWindowSingleton(); });
  pm->AddObserver(vtkCommand::ExitEvent, observer);
  return window;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVProcessWindow::NewTileDisplayWindow()
{
  auto pm = vtkProcessModule::GetProcessModule();
  auto pvoptions = pm->GetOptions();

  int tile_dims[2];
  pvoptions->GetTileDimensions(tile_dims);

  vtkRenderWindow* window = nullptr;

  // number of tiles may be less than the number of ranks; in that case we
  // don't create an on-screen window for those extra ranks unless explicitly
  // requested.
  if (((pm->GetPartitionId() < tile_dims[0] * tile_dims[1]) &&
        !pvoptions->GetForceOffscreenRendering()) ||
    pvoptions->GetForceOnscreenRendering())
  {
    window = vtkRenderWindow::New();
  }
  else
  {
    vtkSmartPointer<vtkRenderWindow> renWindow =
      vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
    renWindow->Register(nullptr);
    window = renWindow;
  }

  if (vtksys::SystemTools::GetEnv("PV_ICET_WINDOW_BORDERS"))
  {
    window->SetSize(400, 400);
  }
  else
  {
    window->SetFullScreen(1);
  }
  return window;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVProcessWindow::NewCAVEWindow()
{
  auto pm = vtkProcessModule::GetProcessModule();
  auto pvserveroptions = vtkPVServerOptions::SafeDownCast(pm->GetOptions());
  assert(pvserveroptions != nullptr);

  vtkRenderWindow* window = nullptr;
  // unless forced offscreen, create an on-screen window.
  if (pvserveroptions->GetForceOffscreenRendering())
  {
    vtkSmartPointer<vtkRenderWindow> renWindow =
      vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
    renWindow->Register(nullptr);
    window = renWindow;
  }
  else
  {
    window = vtkRenderWindow::New();
  }

  // Check if a custom window geometry was requested
  int* geometry = nullptr;
  const int idx = pm->GetPartitionId();
  const bool fullscreen = pvserveroptions->GetFullScreen(idx);
  const bool showborders = pvserveroptions->GetShowBorders(idx);
  if (!fullscreen)
  {
    geometry = pvserveroptions->GetGeometry(idx);
    // If the geometry has not been defined, it will be 0 0 0 0. Unset the
    // geometry pointer in this case.
    if (geometry[0] <= 0 && geometry[1] <= 0 && geometry[2] <= 0 && geometry[3] <= 0)
    {
      geometry = nullptr;
    }
  }

  const int stereoType = pvserveroptions->GetStereoType(idx);
  if (stereoType != -1)
  {
    if (stereoType > 0)
    {
      window->SetStereoType(stereoType);
    }
    window->SetStereoRender(stereoType != 0);
  }

  window->SetBorders(showborders ? 1 : 0);
  // Preserve old behavior for PV_ICET_WINDOW_BORDERS env var
  if (vtksys::SystemTools::GetEnv("PV_ICET_WINDOW_BORDERS"))
  {
    window->SetSize(400, 400);
  }
  else if (fullscreen)
  {
    window->SetFullScreen(1);
  }
  else
  {
    // Use the specified geometry
    int x = geometry ? geometry[0] : 0;
    int y = geometry ? geometry[1] : 0;
    int w = geometry ? geometry[2] : 400;
    int h = geometry ? geometry[3] : 400;
    window->SetFullScreen(0);
    window->SetPosition(x, y);
    window->SetSize(w, h);
  }
  return window;
}
