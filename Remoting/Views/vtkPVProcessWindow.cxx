// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVProcessWindow.h"

#include "vtkCallbackCommand.h"
#include "vtkDisplayConfiguration.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <string>

//============================================================================
namespace
{
static vtkRenderWindow* ProcessWindowSingleton = nullptr;
static bool ProcessWindowSingletonPrepared = false;
static int PVProcessWindowSingletonCleanerNiftyCounter = 0;
static void DeleteProcessWindowSingleton()
{
  if (::ProcessWindowSingleton != nullptr)
  {
    ::ProcessWindowSingleton->Delete();
    ::ProcessWindowSingleton = nullptr;
    ::ProcessWindowSingletonPrepared = false;
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
vtkPVProcessWindow::vtkPVProcessWindow() = default;

//----------------------------------------------------------------------------
vtkPVProcessWindow::~vtkPVProcessWindow() = default;

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

//-----------------------------------------------------------------------------
void vtkPVProcessWindow::PrepareForRendering()
{
  if (::ProcessWindowSingleton != nullptr && ::ProcessWindowSingletonPrepared == false)
  {
    // calling Window::Initialize() should have done the trick, but it doesn't,
    // so we call render (see OSMesa errors reported here paraview/paraview#18938).
    ::ProcessWindowSingleton->Render();
    ::ProcessWindowSingletonPrepared = true;
  }
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

  auto config = vtkRemotingCoreConfiguration::GetInstance();

  vtkRenderWindow* window = nullptr;
  if (config->GetIsInTileDisplay())
  {
    window = vtkPVProcessWindow::NewTileDisplayWindow();
  }
  else if (config->GetIsInCave())
  {
    window = vtkPVProcessWindow::NewCAVEWindow();
  }
  else if (!config->GetForceOnscreenRendering() || config->GetForceOffscreenRendering())
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

  // if active-stereo is requested, let's create a stereo capable window.
  if (config->GetUseStereoRendering() && config->GetStereoType() == VTK_STEREO_CRYSTAL_EYES)
  {
    window->SetStereoCapableWindow(true);
    window->SetStereoType(VTK_STEREO_CRYSTAL_EYES);
  }

  window->DoubleBufferOn();
  window->AlphaBitPlanesOn();
  window->SetMultiSamples(0);

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
  auto config = vtkRemotingCoreConfiguration::GetInstance();

  int tile_dims[2];
  config->GetTileDimensions(tile_dims);

  vtkRenderWindow* window = nullptr;

  // number of tiles may be less than the number of ranks; in that case we
  // don't create an on-screen window for those extra ranks unless explicitly
  // requested.
  if (((pm->GetPartitionId() < tile_dims[0] * tile_dims[1]) &&
        !config->GetForceOffscreenRendering()) ||
    config->GetForceOnscreenRendering())
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

  if (auto pv_shared_window_size = vtksys::SystemTools::GetEnv("PV_SHARED_WINDOW_SIZE"))
  {
    auto parts = vtksys::SystemTools::SplitString(pv_shared_window_size, 'x');
    if (parts.size() == 2)
    {
      const int w = std::max(50, std::atoi(parts[0].c_str()));
      const int h = std::max(50, std::atoi(parts[1].c_str()));
      window->SetSize(w, h);
    }
    else
    {
      const int sz = std::max(50, std::atoi(parts[0].c_str()));
      window->SetSize(sz, sz);
    }
  }
  else if (vtksys::SystemTools::GetEnv("PV_ICET_WINDOW_BORDERS"))
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
  auto config = vtkRemotingCoreConfiguration::GetInstance();
  auto caveConfig = config->GetDisplayConfiguration();

  vtkRenderWindow* window = nullptr;
  // unless forced offscreen, create an on-screen window.
  if (config->GetForceOffscreenRendering())
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
  vtkTuple<int, 4> geometry(0);
  const int idx = pm->GetPartitionId();
  const bool fullscreen = caveConfig->GetFullScreen();
  const bool showborders = caveConfig->GetShowBorders();
  const bool coverable = caveConfig->GetCoverable(idx);
  window->SetCoverable(coverable ? 1 : 0);

  if (!fullscreen)
  {
    geometry = caveConfig->GetGeometry(idx);
  }
  const bool geometryValid = geometry[2] > 0 && geometry[3] > 0;

  const int stereoType = config->GetStereoType();
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
  if (auto pv_shared_window_size = vtksys::SystemTools::GetEnv("PV_SHARED_WINDOW_SIZE"))
  {
    auto parts = vtksys::SystemTools::SplitString(pv_shared_window_size, 'x');
    if (parts.size() == 2)
    {
      const int w = std::max(50, std::atoi(parts[0].c_str()));
      const int h = std::max(50, std::atoi(parts[1].c_str()));
      window->SetSize(w, h);
    }
    else
    {
      const int sz = std::max(50, std::atoi(parts[0].c_str()));
      window->SetSize(sz, sz);
    }
    window->SetBorders(1);
  }
  else if (vtksys::SystemTools::GetEnv("PV_ICET_WINDOW_BORDERS"))
  {
    window->SetSize(400, 400);
    window->SetBorders(1);
  }
  else if (fullscreen)
  {
    window->SetFullScreen(1);
  }
  else
  {
    // Use the specified geometry
    const int x = geometryValid ? geometry[0] : 0;
    const int y = geometryValid ? geometry[1] : 0;
    const int w = geometryValid ? geometry[2] : 400;
    const int h = geometryValid ? geometry[3] : 400;
    window->SetFullScreen(0);
    window->SetPosition(x, y);
    window->SetSize(w, h);
  }
  return window;
}
