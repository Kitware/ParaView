/*=========================================================================

Program:   ParaView
Module:    vtkInitializationHelper.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide PARAVIEW_DEPRECATED_IN_5_10_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "vtkInitializationHelper.h"

#include "vtkCLIOptions.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOutputWindow.h"
#include "vtkPVInitializer.h"
#include "vtkPVLogger.h"
#include "vtkPVOptions.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVSession.h"
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
#include <sstream>
#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

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
void vtkInitializationHelper::Initialize(const char* executable, int type, vtkPVOptions*)
{
  vtkInitializationHelper::Initialize(executable, type);
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
  if (!vtkInitializationHelper::InitializeOptions(argc, argv, type, options, addStandardArgs))
  {
    return false;
  }
  return vtkInitializationHelper::InitializeMiscellaneous(type);
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeOptions(
  int argc, char** argv, int type, vtkCLIOptions* options, bool addStandardArgs)
{
  if (vtkProcessModule::GetProcessModule())
  {
    vtkLogF(ERROR, "Process already initialize! `Initialize` should only be called once.");
    vtkInitializationHelper::ExitCode = EXIT_FAILURE;
    return false;
  }

  vtkNew<vtkCLIOptions> tmpoptions;
  options = options ? options : tmpoptions;

  const auto processType = static_cast<vtkProcessModule::ProcessTypes>(type);

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vtkProcessModule::Initialize(processType, argc, argv);

  // Populate options (unless told otherwise).
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  if (addStandardArgs)
  {
    auto pmConfig = vtkProcessModuleConfiguration::GetInstance();
    pmConfig->PopulateOptions(options, processType);
    coreConfig->PopulateOptions(options, processType);
  }

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
  else if (options->GetHelpRequested())
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
  return true;
}

//----------------------------------------------------------------------------
bool vtkInitializationHelper::InitializeMiscellaneous(int type)
{
  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  // this has to happen after process module is initialized and options have
  // been set.
  paraview_initialize();

  // Set multi-server flag to vtkProcessModule
  vtkProcessModule::GetProcessModule()->SetMultipleSessionsSupport(
    coreConfig->GetMultiServerMode());

  // Make sure the ProxyManager get created...
  vtkSMProxyManager::GetProxyManager();

  // Now load any plugins located in the PV_PLUGIN_PATH environment variable.
  // These are always loaded (not merely located).
  vtkNew<vtkPVPluginLoader> loader;
  loader->LoadPluginsFromPluginSearchPath();
  loader->LoadPluginsFromPluginConfigFile();

  vtkInitializationHelper::SaveUserSettingsFileDuringFinalization = false;
  // Load settings files on client-processes.
  if (!coreConfig->GetDisableRegistry() && type != vtkProcessModule::PROCESS_SERVER &&
    type != vtkProcessModule::PROCESS_DATA_SERVER &&
    type != vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    vtkInitializationHelper::LoadSettings();
  }

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->AddCollectionFromString("{ \"standard_presets\": { "
                                    "\"vtkBlockColors\": \"KAAMS\", "
                                    "\"AtomicNumbers\": \"BlueObeliskElements\" "
                                    "} }",
    0.0);

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

  vtkInitializationHelper::ExitCode = EXIT_SUCCESS;
  return true;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(int argc, char** argv, int type, vtkPVOptions*)
{
  vtkInitializationHelper::Initialize(argc, argv, type);
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
    std::string userSettingsFilePath = vtkInitializationHelper::GetUserSettingsFilePath();
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    bool savingSucceeded = settings->SaveSettingsToFile(userSettingsFilePath);
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

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  int myRank = vtkProcessModule::GetProcessModule()->GetPartitionId();

  if (myRank > 0) // don't read files on satellites.
  {
    settings->DistributeSettings();
    return;
  }

  // Load user-level settings
  std::string userSettingsFilePath = vtkInitializationHelper::GetUserSettingsFilePath();
  if (!settings->AddCollectionFromFile(userSettingsFilePath, VTK_DOUBLE_MAX))
  {
    // Loading user settings failed, so we need to create an empty
    // collection with highest priority manually. Otherwise, if a
    // setting is changed, a lower-priority collection such as site
    // settings may receive the modified setting values.
    settings->AddCollectionFromString("{}", VTK_DOUBLE_MAX);
  }

  // Load site-level settings
  const auto& app_dir = vtkProcessModule::GetProcessModule()->GetSelfDir();

  // If the application path ends with lib/paraview-X.X, shared
  // forwarding of the executable was used. Remove that part of the
  // path to get back to the installation root.
  std::string installDirectory = app_dir.substr(0, app_dir.find("/lib/paraview-" PARAVIEW_VERSION));

  // Remove the trailing /bin if it is there.
  if (installDirectory.size() >= 4 &&
    installDirectory.substr(installDirectory.size() - 4) == "/bin")
  {
    installDirectory = installDirectory.substr(0, installDirectory.size() - 4);
  }

  std::vector<std::string> pathsToSearch;
  pathsToSearch.push_back(installDirectory + "/share/paraview-" PARAVIEW_VERSION);
  pathsToSearch.push_back(installDirectory + "/lib/");
  pathsToSearch.push_back(installDirectory);
#if defined(__APPLE__)
  // paths for app
  pathsToSearch.push_back(installDirectory + "/../../..");
  pathsToSearch.push_back(installDirectory + "/../../../../lib");

  // paths when doing a unix style install.
  pathsToSearch.push_back(installDirectory + "/../lib/paraview-" PARAVIEW_VERSION);
#endif
  // On windows configuration files are in the parent directory
  pathsToSearch.push_back(installDirectory + "/../");

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
  std::string organizationName(vtkInitializationHelper::GetOrganizationName());
#if defined(_WIN32)
  const char* appData = vtksys::SystemTools::GetEnv("APPDATA");
  if (!appData)
  {
    return std::string();
  }
  std::string separator("\\");
  std::string directoryPath(appData);
  if (directoryPath[directoryPath.size() - 1] != separator[0])
  {
    directoryPath.append(separator);
  }
  directoryPath += organizationName + separator;
#else
  std::string directoryPath;
  std::string separator("/");

  // Emulating QSettings behavior.
  const char* xdgConfigHome = getenv("XDG_CONFIG_HOME");
  if (xdgConfigHome && strlen(xdgConfigHome) > 0)
  {
    directoryPath = xdgConfigHome;
    if (directoryPath[directoryPath.size() - 1] != separator[0])
    {
      directoryPath += separator;
    }
  }
  else
  {
    const char* home = getenv("HOME");
    if (!home)
    {
      return std::string();
    }
    directoryPath = home;
    if (directoryPath[directoryPath.size() - 1] != separator[0])
    {
      directoryPath += separator;
    }
    directoryPath += ".config/";
  }
  directoryPath += organizationName + separator;
#endif
  return directoryPath;
}

//----------------------------------------------------------------------------
std::string vtkInitializationHelper::GetUserSettingsFilePath()
{
  std::string path = vtkInitializationHelper::GetUserSettingsDirectory();
  path.append(vtkInitializationHelper::GetApplicationName());
  path.append("-UserSettings.json");

  return path;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
