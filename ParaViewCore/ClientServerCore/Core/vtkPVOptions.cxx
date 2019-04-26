/*=========================================================================

  Module:    vtkPVOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOptions.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptionsXMLParser.h"
#include "vtkProcessModule.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptions);

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  this->SetProcessType(ALLPROCESS);

  // initialize host names
  vtksys::SystemInformation sys_info;
  sys_info.RunOSCheck();
  const char* sys_hostname = sys_info.GetHostname() ? sys_info.GetHostname() : "localhost";
  this->HostName = 0;
  this->SetHostName(sys_hostname);

  // Initialize vtksys::CommandLineArguments
  this->UseRenderingGroup = 0;
  this->ParaViewDataName = 0;
  this->ServersFileName = 0;
  this->TestPlugin = 0;
  this->TestPluginPath = 0;
  this->SetTestPlugin("");
  this->SetTestPluginPath("");
  this->TileDimensions[0] = 0;
  this->TileDimensions[1] = 0;
  this->TileMullions[0] = 0;
  this->TileMullions[1] = 0;
  this->ClientMode = 0;
  this->ServerMode = 0;
  this->MultiClientMode = 0;
  this->DisableFurtherConnections = 0;
  this->MultiClientModeWithErrorMacro = 0;
  this->MultiServerMode = 0;
  this->RenderServerMode = 0;
  this->SymmetricMPIMode = 0;
  this->TellVersion = 0;
  this->EnableStreaming = 0;
  this->SatelliteMessageIds = 0;
  this->PrintMonitors = 0;
  this->ServerURL = 0;
  this->ReverseConnection = 0;
  this->UseStereoRendering = 0;
  this->UseOffscreenRendering = 0;
  this->EGLDeviceIndex = -1;
  this->ConnectID = 0;
  this->LogFileName = 0;
  this->StereoType = 0;
  this->SetStereoType("Anaglyph");
  this->Timeout = 0;
  this->EnableStackTrace = 0;
  this->DisableRegistry = 0;
  this->ForceMPIInitOnClient = 0;
  this->ForceNoMPIInitOnClient = 0;
  this->DisableXDisplayTests = 0;
  this->ForceOffscreenRendering = 0;
  this->ForceOnscreenRendering = 0;
  this->CatalystLivePort = -1;
  this->LogStdErrVerbosity = vtkLogger::VERBOSITY_INVALID;

  if (this->XMLParser)
  {
    this->XMLParser->Delete();
    this->XMLParser = 0;
  }
  this->XMLParser = vtkPVOptionsXMLParser::New();
  this->XMLParser->SetPVOptions(this);
}

//----------------------------------------------------------------------------
vtkPVOptions::~vtkPVOptions()
{
  this->SetHostName(0);
  this->SetServersFileName(0);
  this->SetLogFileName(0);
  this->SetStereoType(0);
  this->SetParaViewDataName(0);
  this->SetServerURL(0);
  this->SetTestPlugin(0);
  this->SetTestPluginPath(0);
}

//----------------------------------------------------------------------------
void vtkPVOptions::Initialize()
{
  switch (vtkProcessModule::GetProcessType())
  {
    case vtkProcessModule::PROCESS_CLIENT:
      this->SetProcessType(PVCLIENT);
      break;

    case vtkProcessModule::PROCESS_SERVER:
      this->SetProcessType(PVSERVER);
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      this->SetProcessType(PVDATA_SERVER);
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      this->SetProcessType(PVRENDER_SERVER);
      break;

    case vtkProcessModule::PROCESS_BATCH:
      this->SetProcessType(PVBATCH);
      break;

    default:
      break;
  }

  this->AddCallback("--verbosity", "-v", &vtkPVOptions::VerbosityArgumentHandler, this,
    "Log verbosity on stderr as an integer in range [-9, 9] "
    "or INFO, WARNING, ERROR, or OFF. Defaults to INFO(0).",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PVSERVER | vtkPVOptions::PVDATA_SERVER |
      vtkPVOptions::PVRENDER_SERVER);

  this->AddCallback("--log", "-l", &vtkPVOptions::LogArgumentHandler, this,
    "Addition log files to generate. Can be specified multiple times. "
    "By default, log verbosity is set to INFO(0) and may be "
    "overridden per file by adding suffix `,verbosity` where verbosity values "
    "are same as those accepted for `--verbosity` argument.",
    vtkPVOptions::ALLPROCESS);

  // On occasion, one would want to force the hostname used by a particular
  // process (overriding the default detected by making System calls). This
  // option makes it possible).
  this->AddArgument("--hostname", 0, &this->HostName,
    "Override the hostname to be used to connect to this process. "
    "By default, the hostname is determined using appropriate system calls.",
    vtkPVOptions::ALLPROCESS);

  this->AddArgument(
    "--cslog", 0, &this->LogFileName, "ClientServerStream log file.", vtkPVOptions::ALLPROCESS);

  this->AddBooleanArgument("--multi-clients", 0, &this->MultiClientMode,
    "Allow server to keep listening for several clients to"
    "connect to it and share the same visualization session.",
    vtkPVOptions::PVDATA_SERVER | vtkPVOptions::PVSERVER);

  this->AddBooleanArgument("--multi-clients-debug", 0, &this->MultiClientModeWithErrorMacro,
    "Allow server to keep listening for several clients to"
    "connect to it and share the same visualization session."
    "While keeping the error macro on the server session for debug.",
    vtkPVOptions::PVDATA_SERVER | vtkPVOptions::PVSERVER);

  this->AddBooleanArgument("--disable-further-connections", 0, &this->DisableFurtherConnections,
    "Disable further connections after the first client connects."
    "Does nothing without --multi-clients enabled.",
    vtkPVOptions::PVDATA_SERVER | vtkPVOptions::PVSERVER);

  this->AddBooleanArgument("--multi-servers", 0, &this->MultiServerMode,
    "Allow client to connect to several pvserver", vtkPVOptions::PVCLIENT);

  this->AddArgument("--data", 0, &this->ParaViewDataName,
    "Load the specified data. "
    "To specify file series replace the numeral with a '.' eg. "
    "my0.vtk, my1.vtk...myN.vtk becomes my..vtk",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);

  this->AddArgument("--server-url", "-url", &this->ServerURL,
    "Set the server-url to connect to when the client starts. "
    "The --server (-s) option supersedes this option, hence use only "
    "one of the two options.",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);

  this->AddArgument("--connect-id", 0, &this->ConnectID,
    "Set the ID of the server and client to make sure they "
    "match. 0 is reserved to imply none specified.",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PVSERVER | vtkPVOptions::PVRENDER_SERVER |
      vtkPVOptions::PVDATA_SERVER);
  this->AddBooleanArgument("--use-offscreen-rendering", 0, &this->UseOffscreenRendering,
    "Render offscreen on the satellite processes."
    " This option only works with software rendering or mangled Mesa on Unix.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER | vtkPVOptions::PVBATCH);
#ifdef VTK_OPENGL_HAS_EGL
  this->AddArgument("--egl-device-index", NULL, &this->EGLDeviceIndex,
    "Render offscreen through the Native Platform Interface (EGL) on the graphics card "
    "specified by the device index.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER | vtkPVOptions::PVBATCH);
#endif
  this->AddBooleanArgument("--stereo", 0, &this->UseStereoRendering,
    "Tell the application to enable stereo rendering",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW | vtkPVOptions::PVRENDER_SERVER |
      vtkPVOptions::PVSERVER | vtkPVOptions::PVBATCH);
  this->AddArgument("--stereo-type", 0, &this->StereoType,
    "Specify the stereo type. This valid only when "
    "--stereo is specified. Possible values are "
    "\"Crystal Eyes\", \"Red-Blue\", \"Interlaced\", "
    "\"Dresden\", \"Anaglyph\", \"Checkerboard\",\"SplitViewportHorizontal\"",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW | vtkPVOptions::PVRENDER_SERVER |
      vtkPVOptions::PVSERVER | vtkPVOptions::PVBATCH);

  this->AddBooleanArgument("--reverse-connection", "-rc", &this->ReverseConnection,
    "Have the server connect to the client.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER | vtkPVOptions::PVSERVER);

  this->AddArgument("--tile-dimensions-x", "-tdx", this->TileDimensions,
    "Size of tile display in the number of displays in each row of the display.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-dimensions-y", "-tdy", this->TileDimensions + 1,
    "Size of tile display in the number of displays in each column of the display.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-mullion-x", "-tmx", this->TileMullions,
    "Size of the gap between columns in the tile display, in pixels.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-mullion-y", "-tmy", this->TileMullions + 1,
    "Size of the gap between rows in the tile display, in pixels.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER);

  this->AddArgument("--timeout", 0, &this->Timeout,
    "Time (in minutes) since connecting with a client "
    "after which the server may timeout. The client typically shows warning "
    "messages before the server times out.",
    vtkPVOptions::PVDATA_SERVER | vtkPVOptions::PVSERVER);

  this->AddBooleanArgument(
    "--version", "-V", &this->TellVersion, "Give the version number and exit.");

  this->AddArgument("--servers-file", 0, &this->ServersFileName,
    "Load the specified configuration servers file (.pvsc). This option replaces "
    "the default user's configuration servers file.",
    vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);

  this->AddBooleanArgument("--symmetric", "-sym", &this->SymmetricMPIMode,
    "When specified, the python script is processed symmetrically on all processes.",
    vtkPVOptions::PVBATCH);

  this->AddBooleanArgument("--enable-streaming", 0, &this->EnableStreaming,
    "EXPERIMENTAL: When specified, view-based streaming is enabled for certain "
    "views and representation types.",
    vtkPVOptions::ALLPROCESS);

  this->AddBooleanArgument("--enable-satellite-message-ids", "-satellite",
    &this->SatelliteMessageIds,
    "When specified, server side messages shown on client show rank of originating process",
    vtkPVOptions::PVSERVER);

  this->AddArgument("--test-plugin", 0, &this->TestPlugin,
    "Specify the name of the plugin to load for testing", vtkPVOptions::ALLPROCESS);

  this->AddArgument("--test-plugin-path", 0, &this->TestPluginPath,
    "Specify the path where more plugins can be found."
    "This is typically used when testing plugins.",
    vtkPVOptions::ALLPROCESS);

  this->AddBooleanArgument("--print-monitors", 0, &this->PrintMonitors,
    "Print detected monitors and exit (Windows only).");

  this->AddBooleanArgument(
    "--enable-bt", 0, &this->EnableStackTrace, "Enable stack trace signal handler.");

  this->AddBooleanArgument("--disable-registry", "-dr", &this->DisableRegistry,
    "Do not use registry when running ParaView (for testing).");

  this->AddBooleanArgument("--disable-xdisplay-test", 0, &this->DisableXDisplayTests,
    "When specified, all X-display tests and OpenGL version checks are skipped. Use this option if "
    "you are getting remote-rendering disabled errors and you are positive that "
    "the X environment is set up properly and your OpenGL support is adequate (experimental).",
    vtkPVOptions::PVSERVER | vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVBATCH);

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  // We add these here so that "--help" on the process can print these variables
  // out. Note the code in vtkProcessModule::Initialize() doesn't really rely on
  // the vtkPVOptions parsing these arguments since vtkPVOptions is called on to
  // parse the arguments only after MPI has been initialized.
  this->AddBooleanArgument("--mpi", 0, &this->ForceMPIInitOnClient,
    "Initialize MPI on processes, if possible. "
    "Cannot be used with --no-mpi.");
  this->AddBooleanArgument("--no-mpi", 0, &this->ForceNoMPIInitOnClient,
    "Don't initialize MPI on processes. "
    "Cannot be used with --mpi.");
#endif

  this->AddBooleanArgument("--force-offscreen-rendering", nullptr, &this->ForceOffscreenRendering,
    "If supported by the build and platform, create headless (offscreen) render windows "
    "for rendering results.",
    vtkPVOptions::PVSERVER | vtkPVOptions::PVBATCH | vtkPVOptions::PVCLIENT |
      vtkPVOptions::PVRENDER_SERVER);

  this->AddBooleanArgument("--force-onscreen-rendering", nullptr, &this->ForceOnscreenRendering,
    "If supported by the build and platform, create on-screen render windows "
    "for rendering results.",
    vtkPVOptions::PVSERVER | vtkPVOptions::PVBATCH | vtkPVOptions::PVCLIENT |
      vtkPVOptions::PVRENDER_SERVER);
}

//----------------------------------------------------------------------------
int vtkPVOptions::LogArgumentHandler(
  const char* vtkNotUsed(argument), const char* cvalue, void* call_data)
{
  if (cvalue == nullptr)
  {
    return 0;
  }

  std::string value(cvalue);
  auto verbosity = vtkLogger::VERBOSITY_INFO;
  auto separator = value.find_last_of(',');
  if (separator != std::string::npos)
  {
    verbosity = vtkLogger::ConvertToVerbosity(value.substr(separator + 1).c_str());
    if (verbosity == vtkLogger::VERBOSITY_INVALID)
    {
      // invalid verbosity specified.
      return 0;
    }
    // remove the ",..." part from filename.
    value = value.substr(0, separator);
  }

  auto pm = vtkProcessModule::GetProcessModule();
  if (pm->GetNumberOfLocalPartitions() > 1)
  {
    value += "." + std::to_string(pm->GetPartitionId());
  }

  auto self = reinterpret_cast<vtkPVOptions*>(call_data);
  self->LogFiles.push_back(std::make_pair(value, verbosity));
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::VerbosityArgumentHandler(
  const char* vtkNotUsed(argument), const char* value, void* call_data)
{
  if (value != nullptr)
  {
    auto self = reinterpret_cast<vtkPVOptions*>(call_data);
    self->LogStdErrVerbosity = vtkLogger::ConvertToVerbosity(value);
    return (self->LogStdErrVerbosity != vtkLogger::VERBOSITY_INVALID);
  }
  return 0;
}

namespace
{
static void OnAtExit()
{
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);
}

static void UpdateThreadName()
{
  // set thread name based on application.
  auto pm = vtkProcessModule::GetProcessModule();
  std::string tname_suffix;
  if (pm->GetNumberOfLocalPartitions() > 1)
  {
    tname_suffix = "." + std::to_string(pm->GetPartitionId());
  }
  switch (vtkProcessModule::GetProcessType())
  {
    case vtkProcessModule::PROCESS_CLIENT:
      vtkLogger::SetThreadName("paraview" + tname_suffix);
      break;
    case vtkProcessModule::PROCESS_SERVER:
      vtkLogger::SetThreadName("pvserver" + tname_suffix);
      break;
    case vtkProcessModule::PROCESS_DATA_SERVER:
      vtkLogger::SetThreadName("pvdatserver" + tname_suffix);
      break;
    case vtkProcessModule::PROCESS_RENDER_SERVER:
      vtkLogger::SetThreadName("pvrenderserver" + tname_suffix);
      break;
    case vtkProcessModule::PROCESS_BATCH:
      vtkLogger::SetThreadName("pvbatch" + tname_suffix);
      break;
    default:
      break;
  }
}
}

//----------------------------------------------------------------------------
int vtkPVOptions::PostProcess(int argc, const char* const* argv)
{
  if (this->LogStdErrVerbosity == vtkLogger::VERBOSITY_INVALID)
  {
    // to avoid posting preamble in init, we do this trick.
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);
    vtkLogger::Init(argc, const_cast<char**>(argv), nullptr);
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_INFO);

    // this helps use avoid showing the "atexit" INFO message generated by
    // loguru when exiting the app unless verbosity was explicitly specified on
    // the command line.
    atexit(OnAtExit);
  }
  else
  {
    vtkLogger::SetStderrVerbosity(static_cast<vtkLogger::Verbosity>(this->LogStdErrVerbosity));
    vtkLogger::Init(argc, const_cast<char**>(argv), nullptr);
  }

  for (const auto& log_pair : this->LogFiles)
  {
    vtkLogger::LogToFile(log_pair.first.c_str(), vtkLogger::TRUNCATE,
      static_cast<vtkLogger::Verbosity>(log_pair.second));
  }

  UpdateThreadName();

  switch (this->GetProcessType())
  {
    case vtkPVOptions::PVCLIENT:
      this->ClientMode = 1;
      break;
    case vtkPVOptions::PARAVIEW:
      break;
    case vtkPVOptions::PVRENDER_SERVER:
      this->RenderServerMode = 1;
      VTK_FALLTHROUGH;
    case vtkPVOptions::PVDATA_SERVER:
    case vtkPVOptions::PVSERVER:
      this->ServerMode = 1;
      break;
    case vtkPVOptions::PVBATCH:
    case vtkPVOptions::ALLPROCESS:
      break;
  }

  if (this->TileDimensions[0] > 0 || this->TileDimensions[1] > 0)
  {
    this->TileDimensions[0] = std::max(1, this->TileDimensions[0]);
    this->TileDimensions[1] = std::max(1, this->TileDimensions[1]);
  }

#ifdef PARAVIEW_ALWAYS_SECURE_CONNECTION
  if ((this->ClientMode || this->ServerMode) && !this->ConnectID)
  {
    this->SetErrorMessage("You need to specify a connect ID (--connect-id).");
    return 0;
  }
#endif

  // do this here for simplicity since it's
  // a universal option. The current kwsys implementation
  // is for POSIX compliant OS's, and a NOOP on others
  // but passing the flag on for all ensures that when
  // implementations for non-POSIX OS's are finished they're
  // enabled as well.
  if (this->EnableStackTrace)
  {
    vtksys::SystemInformation::SetStackTraceOnError(1);
  }

  if (this->ForceOffscreenRendering && this->ForceOnscreenRendering)
  {
    vtkWarningMacro(
      "`--force-offscreen-rendering` and `--force-onscreen-rendering` cannot be specified "
      "at the same time. `--force-offscreen-rendering` will be used.");
    this->ForceOnscreenRendering = 0;
  }

  if (this->UseOffscreenRendering)
  {
    vtkWarningMacro("`--use-offscreen-rendering` is deprecated. Use `--force-offscreen-rendering` "
                    "to force offscreen if needed.");
    this->ForceOffscreenRendering = 1;
    this->ForceOnscreenRendering = 0; // just in case.
  }

  // This is just till we update all views and client code to handle the
  // "UseOffscreenRendering" properly.
  this->UseOffscreenRendering = this->ForceOffscreenRendering;

  // if we want to debug rendering visually, we need to see the windows.
  if (vtksys::SystemTools::GetEnv("PV_DEBUG_REMOTE_RENDERING"))
  {
    if (this->ForceOffscreenRendering)
    {
      vtkWarningMacro("Since `PV_DEBUG_REMOTE_RENDERING` is set in the environment "
                      "`--force-offscreen-rendering` is ignored.");
    }
    this->ForceOffscreenRendering = 0;
    this->ForceOnscreenRendering = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::WrongArgument(const char* argument)
{
  if (this->Superclass::WrongArgument(argument))
  {
    return 1;
  }

  if (this->ParaViewDataName == NULL && this->GetProcessType() == PVCLIENT)
  {
    // BUG #11199. Assume it's a data file.
    this->SetParaViewDataName(argument);
    if (this->GetUnknownArgument() && strcmp(this->GetUnknownArgument(), argument) == 0)
    {
      this->SetUnknownArgument(0);
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVOptions::DeprecatedArgument(const char* argument)
{
  return this->Superclass::DeprecatedArgument(argument);
}

//----------------------------------------------------------------------------
bool vtkPVOptions::GetIsInTileDisplay() const
{
  return (this->TileDimensions[0] > 0 && this->TileDimensions[1] > 0);
}

//----------------------------------------------------------------------------
bool vtkPVOptions::GetIsInCave() const
{
  return false;
}

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HostName: " << (this->HostName ? this->HostName : "(none)") << endl;

  os << indent
     << "ParaViewDataName: " << (this->ParaViewDataName ? this->ParaViewDataName : "(none)")
     << endl;

  // Everything after this line will be showned in Help/About dialog
  os << indent << "Runtime information:"
     << endl; // important please leave it here, for more info: vtkPVApplication::AddAboutText

  if (this->ClientMode)
  {
    os << indent << "Running as a client\n";
  }

  if (this->ServerMode)
  {
    os << indent << "Running as a server\n";
  }

  if (this->MultiClientMode)
  {
    os << indent << "Allow several clients to connect to a server.\n";
  }

  if (this->DisableFurtherConnections)
  {
    os << indent << "Disable further connections after the first client connects to a server..\n";
  }

  if (this->MultiServerMode)
  {
    os << indent << "Allow a client to connect to multiple servers at the same time.\n";
  }

  if (this->RenderServerMode)
  {
    os << indent << "Running as a render server\n";
  }

  if (this->ClientMode || this->ServerMode || this->RenderServerMode)
  {
    os << indent << "ConnectID is: " << this->ConnectID << endl;
    os << indent << "Reverse Connection: " << (this->ReverseConnection ? "on" : "off") << endl;
  }

  os << indent << "Timeout: " << this->Timeout << endl;
  os << indent << "Stereo Rendering: " << (this->UseStereoRendering ? "Enabled" : "Disabled")
     << endl;

  os << indent << "ForceOnscreenRendering: " << this->ForceOnscreenRendering << endl;
  os << indent << "ForceOffscreenRendering: " << this->ForceOffscreenRendering << endl;
  os << indent << "EGL Device Index: " << this->EGLDeviceIndex << endl;

  os << indent << "Tiled Display: " << (this->TileDimensions[0] ? "Enabled" : "Disabled") << endl;
  if (this->TileDimensions[0])
  {
    os << indent << "With Tile Dimensions: " << this->TileDimensions[0] << ", "
       << this->TileDimensions[1] << endl;
    os << indent << "And Tile Mullions: " << this->TileMullions[0] << ", " << this->TileMullions[1]
       << endl;
  }

  os << indent << "Using RenderingGroup: " << (this->UseRenderingGroup ? "Enabled" : "Disabled")
     << endl;

  if (this->TellVersion)
  {
    os << indent << "Running to display software version.\n";
  }

  os << indent << "ServersFileName: " << (this->ServersFileName ? this->ServersFileName : "(none)")
     << endl;
  os << indent << "LogFileName: " << (this->LogFileName ? this->LogFileName : "(none)") << endl;
  os << indent << "SymmetricMPIMode: " << this->SymmetricMPIMode << endl;
  os << indent << "ServerURL: " << (this->ServerURL ? this->ServerURL : "(none)") << endl;
  os << indent << "EnableStreaming:" << (this->EnableStreaming ? "yes" : "no") << endl;

  os << indent << "EnableStackTrace:" << (this->EnableStackTrace ? "yes" : "no") << endl;

  os << indent << "SatelliteMessageIds " << this->SatelliteMessageIds << std::endl;
  os << indent << "PrintMonitors: " << this->PrintMonitors << std::endl;
  os << indent << "DisableXDisplayTests: " << this->DisableXDisplayTests << endl;
  os << indent << "ForceNoMPIInitOnClient: " << this->ForceNoMPIInitOnClient << endl;
  os << indent << "ForceMPIInitOnClient: " << this->ForceMPIInitOnClient << endl;
  os << indent << "CatalystLivePort: " << this->CatalystLivePort << endl;
}
