// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInitializationHelper.h"

#include "vtkCLIOptions.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOutputWindow.h"
#include "vtkPVInitializer.h"
#include "vtkPVLogger.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVSession.h"
#include "vtkPVStandardPaths.h"
#include "vtkPVStringFormatter.h"
#include "vtkPVVersion.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"

#include "vtksys/SystemInformation.hxx"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

// Windows-only helper functionality:
#ifdef _WIN32
#include <conio.h> // for getch()
#include <windows.h>
#endif

namespace
{

#ifdef _WIN32
BOOL CALLBACK listMonitorsCallback(
  HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT /*lprcMonitor*/, LPARAM dwData)
{
  std::ostringstream* str = reinterpret_cast<std::ostringstream*>(dwData);

  MONITORINFOEX monitorInfo;
  monitorInfo.cbSize = sizeof(monitorInfo);

  if (GetMonitorInfo(hMonitor, &monitorInfo))
  {
    LPRECT rect = &monitorInfo.rcMonitor;
    *str << "Device: \"" << monitorInfo.szDevice << "\" "
         << "Geometry: " << std::noshowpos << rect->right - rect->left << "x"
         << rect->bottom - rect->top << std::showpos << rect->left << rect->top << " "
         << ((monitorInfo.dwFlags & MONITORINFOF_PRIMARY) ? "(primary)" : "") << std::endl;
  }
  return true;
}
#endif // _WIN32

std::string ListAttachedMonitors()
{
#ifndef _WIN32
  return std::string("Monitor detection only implemented for MS Windows.");
#else  // _WIN32
  std::ostringstream str;
  EnumDisplayMonitors(nullptr, nullptr, listMonitorsCallback, reinterpret_cast<LPARAM>(&str));
  return str.str();
#endif // _WIN32
}

} // end anon namespace

//----------------------------------------------------------------------------
// Initialize statics.
bool vtkInitializationHelper::LoadSettingsFilesDuringInitialization = true;
bool vtkInitializationHelper::SaveUserSettingsFileDuringFinalization = false;
std::string vtkInitializationHelper::OrganizationName = "ParaView";
std::string vtkInitializationHelper::ApplicationName = "GenericParaViewApplication";
int vtkInitializationHelper::ExitCode = 0;

//----------------------------------------------------------------------------
void vtkInitializationHelper::SetLoadSettingsFilesDuringInitialization(bool val)
{
  if (vtkProcessModule::GetProcessModule() &&
    val != vtkInitializationHelper::LoadSettingsFilesDuringInitialization)
  {
    vtkGenericWarningMacro("SetLoadSettingsFilesDuringInitialization should be called "
                           "before calling Initialize().");
  }
  else
  {
    vtkInitializationHelper::LoadSettingsFilesDuringInitialization = val;
  }
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::GetLoadSettingsFilesDuringInitialization()
{
  return vtkInitializationHelper::LoadSettingsFilesDuringInitialization;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::SetOrganizationName(const std::string& organizationName)
{
  vtkInitializationHelper::OrganizationName = organizationName;
}

//----------------------------------------------------------------------------
const std::string& vtkInitializationHelper::GetOrganizationName()
{
  return vtkInitializationHelper::OrganizationName;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::SetApplicationName(const std::string& appName)
{
  vtkInitializationHelper::ApplicationName = appName;
}

//----------------------------------------------------------------------------
const std::string& vtkInitializationHelper::GetApplicationName()
{
  return vtkInitializationHelper::ApplicationName;
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::Initialize(const char* executable, int type)
{
  if (!executable || !executable[0])
  {
    vtkLogF(ERROR, "Executable name must be specified!");
    return false;
  }

  std::vector<char*> argv;
  argv.push_back(vtksys::SystemTools::DuplicateString(executable));
  argv.push_back(nullptr);
  return vtkInitializationHelper::Initialize(static_cast<int>(argv.size()) - 1, &argv[0], type);
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::Initialize(vtkStringList* slist, int type)
{
  std::vector<char*> argv;
  for (int cc = 0, max = slist->GetLength(); cc < max; ++cc)
  {
    argv.push_back(const_cast<char*>(slist->GetString(cc)));
  }
  argv.push_back(nullptr);
  return vtkInitializationHelper::Initialize(static_cast<int>(argv.size()) - 1, &argv[0], type);
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::Initialize(
  int argc, char** argv, int type, vtkCLIOptions* options, bool addStandardArgs)
{
  vtkNew<vtkCLIOptions> tmpoptions;
  options = options ? options : tmpoptions;

  if (!vtkInitializationHelper::InitializeProcessModule(argc, argv, type))
  {
    return false;
  }

  if (!vtkInitializationHelper::InitializeGlobalOptions(argc, argv, type, options, addStandardArgs))
  {
    return false;
  }

  if (!vtkInitializationHelper::InitializeSettings(type, true))
  {
    return false;
  }
  if (!vtkInitializationHelper::InitializeOtherOptions(argc, argv, type, options, addStandardArgs))
  {
    return false;
  }
  return vtkInitializationHelper::InitializeOthers();
}

bool vtkInitializationHelper::InitializeProcessModule(int argc, char** argv, int type)
{
  if (vtkProcessModule::GetProcessModule())
  {
    vtkLogF(ERROR, "Process already initialize! `Initialize` should only be called once.");
    vtkInitializationHelper::ExitCode = EXIT_FAILURE;
    return false;
  }

  const auto processType = static_cast<vtkProcessModule::ProcessTypes>(type);

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vtkProcessModule::Initialize(processType, argc, argv);

  return true;
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeGlobalOptions(
  int argc, char** argv, int type, vtkCLIOptions* options, bool addStandardArgs)
{
  // Populate options (unless told otherwise).
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  if (addStandardArgs)
  {
    const auto processType = static_cast<vtkProcessModule::ProcessTypes>(type);
    coreConfig->PopulateGlobalOptions(options, processType);
  }
  bool previous = options->GetStopOnUnrecognizedArgument();
  options->SetStopOnUnrecognizedArgument(false);
  bool status = vtkInitializationHelper::ParseOptions(argc, argv, options, coreConfig, false);
  options->SetStopOnUnrecognizedArgument(previous);
  return status;
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeOtherOptions(
  int argc, char** argv, int type, vtkCLIOptions* options, bool addStandardArgs)
{
  // Populate options (unless told otherwise).
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  if (addStandardArgs)
  {
    auto pmConfig = vtkProcessModuleConfiguration::GetInstance();
    const auto processType = static_cast<vtkProcessModule::ProcessTypes>(type);
    pmConfig->PopulateOptions(options, processType);

    coreConfig->PopulatePluginOptions(options, processType);
    coreConfig->PopulateConnectionOptions(options, processType);
    coreConfig->PopulateRenderingOptions(options, processType);
    coreConfig->PopulateMiscellaneousOptions(options, processType);
  }

  return vtkInitializationHelper::ParseOptions(argc, argv, options, coreConfig, true);
}
//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeOptions(
  int argc, char** argv, int type, vtkCLIOptions* options, bool addStandardArgs)
{
  vtkInitializationHelper::InitializeProcessModule(argc, argv, type);

  vtkNew<vtkCLIOptions> tmpoptions;
  options = options ? options : tmpoptions;

  // Populate options (unless told otherwise).
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  if (addStandardArgs)
  {
    auto pmConfig = vtkProcessModuleConfiguration::GetInstance();
    const auto processType = static_cast<vtkProcessModule::ProcessTypes>(type);
    pmConfig->PopulateOptions(options, processType);
    coreConfig->PopulateOptions(options, processType);
  }

  return vtkInitializationHelper::ParseOptions(argc, argv, options, coreConfig, true);
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::ParseOptions(int argc, char** argv, vtkCLIOptions* options,
  vtkRemotingCoreConfiguration* coreConfig, bool checkForExit)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  const auto rank0 = (controller == nullptr || controller->GetLocalProcessId() == 0);

  if (!options->Parse(argc, argv))
  {
    if (rank0)
    {
      // report error.
      std::ostringstream str;
      str << options->GetLastErrorMessage().c_str() << endl
          << options->GetUsage().c_str() << "Try `--help` for more more information." << endl;
      vtkOutputWindow::GetInstance()->DisplayText(str.str().c_str());
    }
    vtkProcessModule::Finalize();
    vtkInitializationHelper::ExitCode = EXIT_FAILURE;
    return false;
  }

  if (checkForExit)
  {
    if (options->GetHelpRequested())
    {
      if (rank0)
      {
        std::ostringstream str;
        str << options->GetHelp() << endl;

#ifndef _WIN32
        vtkOutputWindow::GetInstance()->DisplayText(str.str().c_str());
#else
        // Pop up a console and reopen stdin, stderr, stdout to it to display help
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONOUT$", "w", stdout);

        std::cout << str.str() << std::endl;

        // Need user input to close the console, otherwise it will close immediately
        std::cout << "Press any key to exit" << std::endl;
        getch();
#endif
      }
      vtkProcessModule::Finalize();
      vtkInitializationHelper::ExitCode = EXIT_SUCCESS;
      return false;
    }
    else if (coreConfig->GetTellVersion())
    {
      if (rank0)
      {
        std::ostringstream str;
        str << "paraview version " << PARAVIEW_VERSION_FULL << "\n";
        vtkOutputWindow::GetInstance()->DisplayText(str.str().c_str());
      }
      vtkProcessModule::Finalize();
      vtkInitializationHelper::ExitCode = EXIT_SUCCESS;
      return false;
    }
    else if (coreConfig->GetPrintMonitors())
    {
      if (rank0)
      {
        const std::string monitors = ListAttachedMonitors();
        vtkOutputWindow::GetInstance()->DisplayText(monitors.c_str());
      }
      vtkProcessModule::Finalize();
      vtkInitializationHelper::ExitCode = EXIT_SUCCESS;
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::InitializePythonVirtualEnvironment()
{
#if PARAVIEW_USE_PYTHON
  auto pmConfig = vtkProcessModuleConfiguration::GetInstance();
  auto venvPath = pmConfig->GetVirtualEnvironmentPath();

  // If a venv argument is specified, set it up here
  if (!venvPath.empty())
  {
    std::stringstream venvScript;
    venvScript << R"SCRIPT(
import os;
import site
import sys
)SCRIPT";

    venvScript << "VENV_BASE = '" << venvPath << "'\n";
    venvScript << R"SCRIPT(
VENV_LOADED = False

# Allow venv environment variable override
if os.environ.get("PV_VENV"):
    VENV_BASE = os.path.abspath(os.environ.get("PV_VENV"))

if not VENV_LOADED and VENV_BASE and os.path.exists(VENV_BASE):
    VENV_LOADED = True
    # Code inspired by virtual-env::bin/active_this.py
    bin_dir = os.path.join(VENV_BASE, "bin")
    os.environ["PATH"] = os.pathsep.join([bin_dir] + os.environ.get("PATH", "").split(os.pathsep))
    os.environ["VIRTUAL_ENV"] = VENV_BASE
    prev_length = len(sys.path)

    if sys.platform == "win32":
        python_libs = os.path.join(VENV_BASE, "Lib/site-packages")
    else:
        python_libs = os.path.join(VENV_BASE, f"lib/python{sys.version_info.major}.{sys.version_info.minor}/site-packages")

    site.addsitedir(python_libs)
    sys.path[:] = sys.path[prev_length:] + sys.path[0:prev_length]
    sys.real_prefix = sys.prefix
    sys.prefix = VENV_BASE
    #
    print(f"ParaView is using venv: {VENV_BASE}")
elif not os.path.exists(VENV_BASE):
    print(f"Could not find the virtual environment path '{VENV_BASE}' specified by --venv argument")
)SCRIPT";
    vtkPythonInterpreter::RunSimpleString(venvScript.str().c_str());
  }
#endif // PARAVIEW_USE_PYTHON
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeMiscellaneous(int type)
{
  bool status = vtkInitializationHelper::InitializeSettings(type, false);
  status &= vtkInitializationHelper::InitializeOthers();
  return status;
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeSettings(int type, bool defaultCoreConfig)
{
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  vtkInitializationHelper::SaveUserSettingsFileDuringFinalization = false;

  if (!coreConfig->GetDisableRegistry())
  {
    vtkInitializationHelper::LoadSettings();
  }

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->AddCollectionFromString("{ \"standard_presets\": { "
                                    "\"vtkBlockColors\": \"KAAMS\", "
                                    "\"AtomicNumbers\": \"BlueObeliskElements\" "
                                    "} }",
    0.0);

  if (defaultCoreConfig)
  {
    // Find the right setting to use for port
    const auto processType = static_cast<vtkProcessModule::ProcessTypes>(type);
    std::string prefix = "cli-options.connection.";
    std::string portSetting;
    switch (processType)
    {
      case vtkProcessModule::PROCESS_DATA_SERVER:
        portSetting = "data-server-port";
        break;

      case vtkProcessModule::PROCESS_RENDER_SERVER:
        portSetting = "render-server-port";
        break;

      case vtkProcessModule::PROCESS_SERVER:
      default:
        portSetting = "server-port";
        break;
    }

    // Check for cli options connection settings and init the core config accordingly
    coreConfig->SetConnectID(
      settings->GetSettingAsInt((prefix + "connect-id").c_str(), coreConfig->GetConnectID()));
    coreConfig->SetHostName(
      settings->GetSettingAsString((prefix + "hostname").c_str(), coreConfig->GetHostName()));
    coreConfig->SetClientHostName(settings->GetSettingAsString(
      (prefix + "client-host").c_str(), coreConfig->GetClientHostName()));
    coreConfig->SetReverseConnection(settings->GetSettingAsInt(
      (prefix + "reverse-connection").c_str(), coreConfig->GetReverseConnection()));
    coreConfig->SetServerPort(
      settings->GetSettingAsInt((prefix + portSetting).c_str(), coreConfig->GetServerPort()));
    coreConfig->SetBindAddress(settings->GetSettingAsString(
      (prefix + "bind-address").c_str(), coreConfig->GetBindAddress()));
    coreConfig->SetTimeout(
      settings->GetSettingAsInt((prefix + "timeout").c_str(), coreConfig->GetTimeout()));
    coreConfig->SetTimeoutCommand(settings->GetSettingAsString(
      (prefix + "timeout-command").c_str(), coreConfig->GetTimeoutCommand()));
    coreConfig->SetTimeoutCommandInterval(settings->GetSettingAsInt(
      (prefix + "timeout-command-interval").c_str(), coreConfig->GetTimeoutCommandInterval()));
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeOthers()
{
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  // this has to happen after process module is initialized and options have
  // been set.
  paraview_initialize();

  // Set multi-server flag to vtkProcessModule
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  assert(pm != nullptr);

  pm->SetMultipleSessionsSupport(coreConfig->GetMultiServerMode());

  // Make sure the ProxyManager get created...
  vtkSMProxyManager::GetProxyManager();

  // Set up any virtual environment prior to loading plugins which may be Python plugins.
#if PARAVIEW_USE_PYTHON
  // Set up virtual environment if requested.
  auto pmConfig = vtkProcessModuleConfiguration::GetInstance();
  if (!pmConfig->GetVirtualEnvironmentPath().empty())
  {
    // Note - this may initialize Python at server startup, even if
    // Python is not invoked later on, which could add some startup
    // cost. We need to do it here because there are many
    // places Python could be initialized in the ParaView and VTK
    // code base, and we can't initialize the virtual environment
    // in all those places.
    InitializePythonVirtualEnvironment();
  }
#endif

  // Now load any plugins located in the PV_PLUGIN_PATH environment variable.
  // These are always loaded (not merely located).
  vtkNew<vtkPVPluginLoader> loader;
  loader->LoadPluginsFromPluginSearchPath();
  loader->LoadPluginsFromPluginConfigFile();

  const char* undefined = "Undefined";

  // push the ENV argument scope which includes environment arguments
  std::string username;
#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
  username = getenv("USERNAME") != nullptr ? getenv("USERNAME") : undefined;
#else
  username = getenv("USER") != nullptr ? getenv("USER") : undefined;
#endif
  vtksys::SystemInformation sysInfo;
  sysInfo.RunOSCheck();
  std::string hostname = sysInfo.GetHostname() != nullptr ? sysInfo.GetHostname() : undefined;
  std::string os = sysInfo.GetOSName() != nullptr ? sysInfo.GetOSName() : undefined;

  vtkPVStringFormatter::PushScope(
    "ENV", fmt::arg("username", username), fmt::arg("hostname", hostname), fmt::arg("os", os));

  // push the GLOBAL argument scope which includes global arguments
  std::string appVersion = PARAVIEW_VERSION_FULL;

  vtkPVStringFormatter::PushScope("GLOBAL", fmt::arg("date", std::chrono::system_clock::now()),
    fmt::arg("appname", vtkInitializationHelper::ApplicationName),
    fmt::arg("appversion", appVersion));

  // until we replace PARAVIEW_SMTESTDRIVER with something cleaner, we have
  // to print this greeting out when the process is launched from
  // smTestDriver
  if (vtksys::SystemTools::HasEnv("PARAVIEW_SMTESTDRIVER"))
  {
    cout << "Process started" << endl;
  }

  // This checks if the environment has VTK_DEFAULT_OPENGL_WINDOW set. If it is not set,
  // it sets the environment variable to the OpenGL window backend specified in the
  // vtkRemotingCoreConfiguration. This is done to ensure that VTK creates the correct OpenGL
  // window, based on the configuration settings.
  if (!vtksys::SystemTools::HasEnv("VTK_DEFAULT_OPENGL_WINDOW"))
  {
    // Set the OpenGL window backend to use based on the configuration
    // settings. This is only done if the environment variable is not already set.
    if (auto glWindowBackend = coreConfig->GetOpenGLWindowBackend();
        glWindowBackend != vtkRemotingCoreConfiguration::OPENGL_WINDOW_BACKEND_DEFAULT)
    {
      std::ostringstream putEnvStream;
      putEnvStream << "VTK_DEFAULT_OPENGL_WINDOW=";
      switch (glWindowBackend)
      {
        case vtkRemotingCoreConfiguration::OPENGL_WINDOW_BACKEND_EGL:
          putEnvStream << "vtkEGLRenderWindow";
          break;
        case vtkRemotingCoreConfiguration::OPENGL_WINDOW_BACKEND_GLX:
          putEnvStream << "vtkXOpenGLRenderWindow";
          break;
        case vtkRemotingCoreConfiguration::OPENGL_WINDOW_BACKEND_OSMESA:
          putEnvStream << "vtkOSOpenGLRenderWindow";
          break;
        case vtkRemotingCoreConfiguration::OPENGL_WINDOW_BACKEND_WIN32:
          putEnvStream << "vtkWin32OpenGLRenderWindow";
          break;
        default:
          break;
      }
      vtksys::SystemTools::PutEnv(putEnvStream.str());
    }
  }
  vtkInitializationHelper::ExitCode = EXIT_SUCCESS;
  return true;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::StandaloneInitialize()
{
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Finalize()
{
  if (vtkInitializationHelper::SaveUserSettingsFileDuringFinalization)
  {
    // Write out settings file(s)
    std::string userSettingsFilePath = vtkPVStandardPaths::GetUserSettingsFilePath();
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    bool savingSucceeded =
      settings->SaveSettingsToFile(userSettingsFilePath, vtkSMSettings::GetUserPriority());
    if (!savingSucceeded)
    {
      vtkGenericWarningMacro(<< "Saving settings file to '" << userSettingsFilePath << "' failed");
    }
  }

  // pop GLOBAL scope
  vtkPVStringFormatter::PopScope();
  // pop ENV scope
  vtkPVStringFormatter::PopScope();

  vtkSMProxyManager::Finalize();
  vtkProcessModule::Finalize();

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::StandaloneFinalize()
{
  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::LoadSettings()
{
  if (vtkInitializationHelper::LoadSettingsFilesDuringInitialization == false)
  {
    return;
  }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  assert(pm != nullptr);
  int myRank = pm->GetPartitionId();

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  if (myRank > 0) // don't read files on satellites.
  {
    settings->DistributeSettings();
    return;
  }

  // Load user-level settings
  std::string userSettingsFilePath = vtkPVStandardPaths::GetUserSettingsFilePath();
  if (!settings->AddCollectionFromFile(userSettingsFilePath, vtkSMSettings::GetUserPriority()))
  {
    // Loading user settings failed, so we need to create an empty
    // collection with highest priority manually. Otherwise, if a
    // setting is changed, a lower-priority collection such as site
    // settings may receive the modified setting values.
    settings->AddCollectionFromString("{}", vtkSMSettings::GetUserPriority());
  }

  // Load site-level settings
  const std::vector<std::string> pathsToSearch = vtkPVStandardPaths::GetInstallDirectories();
  const std::string filename = vtkInitializationHelper::GetApplicationName() + "-SiteSettings.json";
  for (const std::string& path : pathsToSearch)
  {
    const std::string siteSettingsFile = std::string(path).append("/").append(filename);
    if (settings->AddCollectionFromFile(siteSettingsFile, 1.0))
    {
      break;
    }
  }
  settings->DistributeSettings();

  vtkInitializationHelper::SaveUserSettingsFileDuringFinalization = true;
}

//----------------------------------------------------------------------------
std::string vtkInitializationHelper::GetUserSettingsDirectory()
{
  return vtkPVStandardPaths::GetUserSettingsDirectory();
}

//----------------------------------------------------------------------------
std::string vtkInitializationHelper::GetUserSettingsFilePath()
{
  return vtkPVStandardPaths::GetUserSettingsFilePath();
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
