// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRemotingCoreConfiguration.h"

#include "vtkCLIOptions.h"
#include "vtkDisplayConfiguration.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtk_cli11.h>
#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

namespace
{
// Redefined from vtkRenderWindow.h
static const int VTK_STEREO_CRYSTAL_EYES = 1;
static const int VTK_STEREO_RED_BLUE = 2;
static const int VTK_STEREO_INTERLACED = 3;
static const int VTK_STEREO_LEFT = 4;
static const int VTK_STEREO_RIGHT = 5;
static const int VTK_STEREO_DRESDEN = 6;
static const int VTK_STEREO_ANAGLYPH = 7;
static const int VTK_STEREO_CHECKERBOARD = 8;
static const int VTK_STEREO_SPLITVIEWPORT_HORIZONTAL = 9;
static const int VTK_STEREO_FAKE = 10;
static const int VTK_STEREO_EMULATE = 11;

int ParseStereoType(const std::string& value)
{
  if (value == "Crystal Eyes")
  {
    return VTK_STEREO_CRYSTAL_EYES;
  }
  else if (value == "Red-Blue")
  {
    return VTK_STEREO_RED_BLUE;
  }
  else if (value == "Interlaced")
  {
    return VTK_STEREO_INTERLACED;
  }
  else if (value == "Dresden")
  {
    return VTK_STEREO_DRESDEN;
  }
  else if (value == "Anaglyph")
  {
    return VTK_STEREO_ANAGLYPH;
  }
  else if (value == "Checkerboard")
  {
    return VTK_STEREO_CHECKERBOARD;
  }
  else if (value == "SplitViewportHorizontal")
  {
    return VTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
  }
  throw CLI::ValidationError("Invalid stereo-type specified.");
}
}

vtkStandardNewMacro(vtkRemotingCoreConfiguration);
//----------------------------------------------------------------------------
vtkRemotingCoreConfiguration::vtkRemotingCoreConfiguration()
{
  this->StereoType = VTK_STEREO_ANAGLYPH;

  // initialize host names
  vtksys::SystemInformation sys_info;
  sys_info.RunOSCheck();
  if (auto hostname = sys_info.GetHostname())
  {
    this->HostName = hostname;
  }

  this->DisplayConfiguration = vtkDisplayConfiguration::New();
}

//----------------------------------------------------------------------------
vtkRemotingCoreConfiguration::~vtkRemotingCoreConfiguration()
{
  this->DisplayConfiguration->Delete();
  this->DisplayConfiguration = nullptr;
}

//----------------------------------------------------------------------------
vtkRemotingCoreConfiguration* vtkRemotingCoreConfiguration::GetInstance()
{
  static vtkSmartPointer<vtkRemotingCoreConfiguration> Singleton =
    vtk::TakeSmartPointer(vtkRemotingCoreConfiguration::New());
  return Singleton.GetPointer();
}

//----------------------------------------------------------------------------
void vtkRemotingCoreConfiguration::GetTileDimensions(int dims[2])
{
  const auto tdims = this->GetTileDimensions();
  dims[0] = tdims[0];
  dims[1] = tdims[1];
}

//----------------------------------------------------------------------------
const int* vtkRemotingCoreConfiguration::GetTileDimensions()
{
  if (this->TileDimensions[0] > 0 || this->TileDimensions[1] > 0)
  {
    this->TileDimensions[0] = std::max(this->TileDimensions[0], 1);
    this->TileDimensions[1] = std::max(this->TileDimensions[1], 1);
  }
  return this->TileDimensions;
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::GetIsInTileDisplay() const
{
  return this->TileDimensions[0] > 0 || this->TileDimensions[1] > 0;
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::GetIsInCave() const
{
  return this->DisplayConfiguration->GetNumberOfDisplays() > 0;
}

//----------------------------------------------------------------------------
const char* vtkRemotingCoreConfiguration::GetStereoTypeAsString() const
{
  switch (this->StereoType)
  {
    case VTK_STEREO_CRYSTAL_EYES:
      return "Crystal Eyes";
    case VTK_STEREO_RED_BLUE:
      return "Red-Blue";
    case VTK_STEREO_INTERLACED:
      return "Interlaced";
    case VTK_STEREO_LEFT:
      return "Left";
    case VTK_STEREO_RIGHT:
      return "Right";
    case VTK_STEREO_DRESDEN:
      return "Dresden";
    case VTK_STEREO_ANAGLYPH:
      return "Anaglyph";
    case VTK_STEREO_CHECKERBOARD:
      return "Checkerboard";
    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      return "SplitViewportHorizontal";
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkRemotingCoreConfiguration::GetEGLDeviceIndex()
{
  if (this->EGLDeviceIndex != -1)
  {
    return this->EGLDeviceIndex;
  }

  const auto display = this->GetDisplay();
  if (!display.empty())
  {
    try
    {
      this->EGLDeviceIndex = std::stoi(display);
      vtkLogF(TRACE, "Setting EGLDeviceIndex to %d", this->EGLDeviceIndex);
      return this->EGLDeviceIndex;
    }
    catch (std::invalid_argument&)
    {
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
std::string vtkRemotingCoreConfiguration::GetDisplay()
{
  if (this->Displays.empty())
  {
    return {};
  }

  auto controller = vtkMultiProcessController::GetGlobalController();
  const int rank = controller->GetLocalProcessId();
  const int num_ranks = controller->GetNumberOfProcesses();

  const int count = static_cast<int>(this->Displays.size());
  switch (this->DisplaysAssignmentMode)
  {
    case ROUNDROBIN:
      return this->Displays.at(rank % count);

    case CONTIGUOUS:
    {
      // same as diy::ContiguousAssigner::rank()
      const int div = num_ranks / count;
      const int mod = num_ranks % count;
      const int r = rank / (div + 1);
      const int index = (r < mod) ? r : (mod + (rank - (div + 1) * mod) / div);
      return this->Displays.at(index);
    }
  }

  return {};
}

//----------------------------------------------------------------------------
double vtkRemotingCoreConfiguration::GetEyeSeparation() const
{
  // Since EyeSeparation was traditionally specified in the PVX file, we give it
  // first preference.
  if (this->DisplayConfiguration && this->DisplayConfiguration->GetEyeSeparation() != 0)
  {
    return this->DisplayConfiguration->GetEyeSeparation();
  }

  return this->EyeSeparation;
}

//----------------------------------------------------------------------------
void vtkRemotingCoreConfiguration::HandleDisplayEnvironment()
{
  auto display = this->GetDisplay();
  if (!display.empty())
  {
    vtkLogF(TRACE, "Setting environment variable 'DISPLAY=%s'", display.c_str());
    vtksys::SystemTools::PutEnv("DISPLAY=" + display);
  }
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::PopulateOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes ptype)
{
  return this->PopulateGlobalOptions(options, ptype) &&
    this->PopulatePluginOptions(options, ptype) &&
    this->PopulateConnectionOptions(options, ptype) &&
    this->PopulateRenderingOptions(options, ptype) &&
    this->PopulateMiscellaneousOptions(options, ptype);
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::PopulateGlobalOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes vtkNotUsed(ptype))
{
  auto app = options->GetCLI11App();
  app->add_flag("-V,--version", this->TellVersion, "Print version number and exit.");
  app->add_flag("-d,--dr,--disable-registry", this->DisableRegistry,
    "Skip user-specific applications settings and configuration options.");
#if defined(_WIN32)
  app->add_flag("--print-monitors", this->PrintMonitors, "Print monitor information and exit.");
#endif
  return true;
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::PopulatePluginOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes vtkNotUsed(ptype))
{
  auto app = options->GetCLI11App();
  auto groupPlugins = app->add_option_group("Plugins", "Plugin-specific options");
  groupPlugins->add_option("--plugin-search-paths", this->PluginSearchPaths,
    "Specify search paths for plugins when looking up plugins by name.");

  // currently, these really only make sense on client since the Qt client is
  // the one that handles loading the plugins. we should fix that at some point.
  // I am leaving these arguments as is since tests seems to pass them to server
  // processes.
  groupPlugins->add_option("--plugins", this->Plugins, "Specify plugins to load on startup.");

  groupPlugins->add_option(
    "--test-plugin,--test-plugins", this->Plugins, "Use '--plugins' instead.");
  groupPlugins->add_option("--test-plugin-path,--test-plugin-paths", this->PluginSearchPaths,
    "Use '--plugin-search-paths' instead.");
  CLI::deprecate_option(groupPlugins, "--test-plugin");
  CLI::deprecate_option(groupPlugins, "--test-plugin-path");

  return true;
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::PopulateConnectionOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes ptype)
{
  auto app = options->GetCLI11App();
  auto groupConnection = app->add_option_group(
    "Connection options", "Options affecting connections between client/server processes");

  groupConnection
    ->add_option("--connect-id", this->ConnectID,
      "An identifier used to match client-server connections. When non-zero, the client and server "
      "processes "
      "must used identical identifier for the connection to succeed.")
#if PARAVIEW_ALWAYS_SECURE_CONNECTION
    ->required();
#else
    ->default_val(this->ConnectID);
#endif

  groupConnection
    ->add_option("--hostname", this->HostName,
      "Override the hostname to be used to connect to this process. "
      "By default, the hostname is determined using appropriate system calls.")
    ->default_val(this->HostName);

  if (ptype == vtkProcessModule::PROCESS_SERVER || ptype == vtkProcessModule::PROCESS_DATA_SERVER ||
    ptype == vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    groupConnection
      ->add_option("--client-host", this->ClientHostName,
        "Hostname to use to connect to the ParaView client when using reverse-connection mode. "
        "Defaults to 'localhost'.")
      ->default_val(this->ClientHostName);

    groupConnection
      ->add_flag("-r,--rc,--reverse-connection", this->ReverseConnection,
        "Use reverse connection mode, i.e. instead of client connecting to the server(s), "
        "the server(s) will connect back to the client.")
      ->default_val(this->ReverseConnection);

    std::string argnames;
    int port;
    switch (ptype)
    {
      case vtkProcessModule::PROCESS_DATA_SERVER:
        argnames = "-p,--dsp,--data-server-port";
        port = this->ServerPort > 0 ? this->ServerPort : 11111;
        break;

      case vtkProcessModule::PROCESS_RENDER_SERVER:
        argnames = "-p,--rsp,--render-server-port";
        port = this->ServerPort > 0 ? this->ServerPort : 22221;
        break;

      case vtkProcessModule::PROCESS_SERVER:
      default:
        argnames = "-p,--sp,--server-port";
        port = this->ServerPort > 0 ? this->ServerPort : 11111;
        break;
    }

    groupConnection
      ->add_option(argnames, this->ServerPort,
        "Port number to use to listen for connections from the client. In reverse-connection mode, "
        "this is the port number on which the client is listening for connections.")
      ->default_val(port);

    groupConnection
      ->add_option("--bind-address", this->BindAddress,
        "Address to bind the server socket. Used to restrict the network interfaces the client "
        "can connect to.")
      ->default_val(this->BindAddress);

    groupConnection
      ->add_option("--timeout", this->Timeout,
        "Time interval (in minutes) since a connection is established that the server-connection "
        "may timeout. "
        "The client typically shows a warning message before the server times out.")
      ->default_val(this->Timeout);

    groupConnection
      ->add_option("--timeout-command", this->TimeoutCommand,
        "Timeout command allowing server to regularly check remaining time available. When "
        "executed, "
        "the command should write an integer value to stdout, corresponding to the remaining "
        "connection time, in minutes.")
      ->default_val(this->TimeoutCommand);

    groupConnection
      ->add_option("--timeout-command-interval", this->TimeoutCommandInterval,
        "Interval in seconds between consecutive calls to the timeout command. Only applies if "
        "--timeout-command is set.")
      ->default_val(this->TimeoutCommandInterval);
  }
  else if (ptype == vtkProcessModule::PROCESS_CLIENT)
  {
    auto url = groupConnection->add_option("--url,--server-url", this->ServerURL,
      "Server connection URL. On startup, the client will connect to the server using the URL "
      "specified.");

    groupConnection
      ->add_option("-s,--server", this->ServerResourceName,
        "Server resource name. On startup, the client will connect to the server using the "
        "resource specified.")
      ->excludes(url);

    groupConnection->add_option("--servers-file", this->ServerConfigurationsFiles,
      "Additional servers configuration file(s) (.pvsc) to use.");
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::PopulateRenderingOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes ptype)
{
  const bool isRenderingProcess = (ptype != vtkProcessModule::PROCESS_DATA_SERVER);
  if (!isRenderingProcess)
  {
    return true;
  }

  const bool isServer =
    (ptype == vtkProcessModule::PROCESS_RENDER_SERVER || ptype == vtkProcessModule::PROCESS_SERVER);

  auto app = options->GetCLI11App();
  auto group = app->add_option_group("Rendering", "Rendering specific options");

  auto ofnr = group->add_flag("--force-offscreen-rendering", this->ForceOffscreenRendering,
    "If supported by the build and platform, create headless (offscreen) render windows "
    "for rendering results.");

  group
    ->add_flag("--force-onscreen-rendering", this->ForceOnscreenRendering,
      "If supported by the build and platform, create on-screen render windows "
      "for rendering results.")
    ->excludes(ofnr)
    ->envname("PV_DEBUG_REMOTE_RENDERING");

  // Display
  auto displayGroup =
    app->add_option_group("Display Environment", "Display/Device specific settings.");

  displayGroup->add_flag("--disable-xdisplay-test", this->DisableXDisplayTests,
    "Skip all X-display tests and OpenGL version checks. Use this option if "
    "you are getting remote-rendering disabled errors and you are positive that "
    "the X environment is set up properly and your OpenGL support is adequate (experimental).");

  displayGroup->add_option("--displays", this->Displays,
    "Specify a list of rendering display or device ids either as a comma-separated string "
    "or simply specifying the option multiple times. For X-based "
    "systems, this can be the value to set for the DISPLAY environment. For EGL-based "
    "systems, these are the available EGL device indices. When specified these are distributed "
    "among the number of rendering ranks using '--displays-assignment-mode' specified.");

  displayGroup
    ->add_option("--displays-assignment-mode", this->DisplaysAssignmentMode,
      "Specify how to assign displays (specified using '--displays=') among rendering ranks. "
      "Supported values are 'contiguous' and 'round-robin'. Default is 'round-robin'.")
    ->transform([](const std::string& value) {
      if (value == "contiguous")
      {
        return std::to_string(vtkRemotingCoreConfiguration::CONTIGUOUS);
      }
      if (value == "round-robin")
      {
        return std::to_string(vtkRemotingCoreConfiguration::ROUNDROBIN);
      }
      throw CLI::ValidationError("Invalid displays-assignment-mode specified.");
    })
    ->needs("--displays")
    ->default_str("round-robin");

  // Stereo
  auto stereoGroup = app->add_option_group("Stereo", "Stereo rendering options");
  stereoGroup->add_flag("--stereo", this->UseStereoRendering, "Enable stereo rendering.");
  stereoGroup
    ->add_option("--stereo-type", this->StereoType,
      "Specify the stereo type to use. Possible values are 'Crystal Eyes', "
      "'Red-Blue', 'Interlaced', 'Dresden', 'Anaglyph', 'Checkerboard', or "
      "'SplitViewportHorizontal'.")
    ->needs("--stereo")
    ->transform([](const std::string& value) { return std::to_string(::ParseStereoType(value)); });

  stereoGroup
    ->add_option("--eye-separation", this->EyeSeparation, "Specify eye separation distance.")
    ->needs("--stereo");

  if (isServer)
  {
    auto tdGroup = app->add_option_group("Tile Display", "Tile-display specific options");
    tdGroup->add_option("--tdx,--tile-dimensions-x", this->TileDimensions[0],
      "Number of displays in the horizontal direction.");
    tdGroup->add_option("--tdy,--tile-dimensions-y", this->TileDimensions[1],
      "Number of displays in the vertical direction.");

    tdGroup->add_option("--tmx,--tile-mullion-x", this->TileMullions[0],
      "Size of the gap in pixels between displays in the horizontal direction.");
    tdGroup->add_option("--tmy,--tile-mullion-y", this->TileMullions[1],
      "Size of the gap in pixels between displays in the vertical direction.");
  }

  if (ptype == vtkProcessModule::PROCESS_SERVER || ptype == vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    auto caveGroup = app->add_option_group("CAVE", "CAVE specific options");
    caveGroup
      ->add_option(
        "--pvx",
        [this](const CLI::results_t& args) {
          return this->DisplayConfiguration->LoadPVX(args.front().c_str());
        },
        "ParaView CAVE configuration file (typically a `.pvx` file) that "
        "provides the configuration for screens in a CAVE.")
      ->take_first();

    auto pvx = caveGroup
                 ->add_option(
                   "'.pvx' file",
                   [this](const CLI::results_t& args) {
                     return this->DisplayConfiguration->LoadPVX(args.front().c_str());
                   },
                   "ParaView CAVE configuration file.")
                 ->excludes("--pvx")
                 ->take_first();
    CLI::deprecate_option(pvx, "--pvx");
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkRemotingCoreConfiguration::PopulateMiscellaneousOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes ptype)
{
  auto app = options->GetCLI11App();
  auto group = app->add_option_group("Experimental",
    "A collection of miscellaneous options that are largely experimental "
    "or infrequently used.");

  const bool isDataServer =
    ptype == vtkProcessModule::PROCESS_DATA_SERVER || ptype == vtkProcessModule::PROCESS_SERVER;

  if (isDataServer)
  {
    CLI::deprecate_option(group->add_flag("--multi-clients", this->MultiClientMode,
      "Enable the server in a collaborative mode where multiple clients can "
      "connect to the same server."));

    CLI::deprecate_option(
      group->add_flag("--disable-further-connections", this->DisableFurtherConnections,
        "Disable further connections after the first client connects. "
        "Does nothing without --multi-clients enabled."));
  }

  if (ptype == vtkProcessModule::PROCESS_CLIENT)
  {
    group->add_flag("--multi-servers", this->MultiServerMode,
      "Enable the client to connect to multiple independent servers at the same time.");
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkRemotingCoreConfiguration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DisableRegistry: " << this->DisableRegistry << endl;
  os << indent << "PrintMonitors: " << this->PrintMonitors << endl;
  os << indent << "HostName: " << this->HostName.c_str() << endl;
  os << indent << "ClientHostName: " << this->ClientHostName.c_str() << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
  os << indent << "BindAddress: " << this->BindAddress << endl;
  os << indent << "ReverseConnection: " << this->ReverseConnection << endl;
  os << indent << "ConnectID: " << this->ConnectID << endl;
  os << indent << "ServerURL: " << this->ServerURL.c_str() << endl;
  os << indent << "ServerResourceName: " << this->ServerResourceName.c_str() << endl;
  os << indent << "Timeout: " << this->Timeout << endl;
  os << indent << "TimeoutCommand: " << this->TimeoutCommand << endl;
  os << indent << "TimeoutCommandInterval: " << this->TimeoutCommandInterval << endl;
  os << indent << "UseStereoRendering: " << this->UseStereoRendering << endl;
  os << indent << "StereoType: " << this->StereoType << endl;
  os << indent << "EyeSeparation: " << this->GetEyeSeparation() << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0] << ", " << this->TileDimensions[1]
     << endl;
  os << indent << "TileMullions: " << this->TileMullions[0] << ", " << this->TileMullions[1]
     << endl;
  os << indent << "ForceOnscreenRendering: " << this->ForceOnscreenRendering << endl;
  os << indent << "ForceOffscreenRendering: " << this->ForceOffscreenRendering << endl;
  os << indent << "EGLDeviceIndex: " << this->EGLDeviceIndex << endl;
  os << indent << "DisplaysAssignmentMode: " << this->DisplaysAssignmentMode << endl;
  os << indent << "MultiServerMode: " << this->MultiServerMode << endl;
  os << indent << "MultiClientMode: " << this->MultiClientMode << endl;
  os << indent << "DisableFurtherConnections: " << this->DisableFurtherConnections << endl;

  os << indent << "ServerConfigurationsFiles (count=" << this->ServerConfigurationsFiles.size()
     << "):" << endl;
  for (auto& value : this->ServerConfigurationsFiles)
  {
    os << indent.GetNextIndent() << value.c_str() << endl;
  }

  os << indent << "PluginSearchPaths (count=" << this->PluginSearchPaths.size() << "):" << endl;
  for (auto& value : this->PluginSearchPaths)
  {
    os << indent.GetNextIndent() << value.c_str() << endl;
  }

  os << indent << "Plugins(count=" << this->Plugins.size() << "):" << endl;
  for (auto& value : this->Plugins)
  {
    os << indent.GetNextIndent() << value.c_str() << endl;
  }

  os << indent << "Displays (count=" << this->Displays.size() << "):" << endl;
  for (auto& value : this->Displays)
  {
    os << indent.GetNextIndent() << value.c_str() << endl;
  }

  os << indent << "DisplayConfiguration: " << endl;
  this->DisplayConfiguration->PrintSelf(cout, indent.GetNextIndent());
}
