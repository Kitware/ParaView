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
#include "vtkInitializationHelper.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkNew.h"
#include "vtkOutputWindow.h"
#include "vtkPVConfig.h"
#include "vtkPVInitializer.h"
#include "vtkPVOptions.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"

#include <sstream>
#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

// Windows-only helper functionality:
#ifdef _WIN32
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
  EnumDisplayMonitors(NULL, NULL, listMonitorsCallback, reinterpret_cast<LPARAM>(&str));
  return str.str();
#endif // _WIN32
}

} // end anon namespace

bool vtkInitializationHelper::LoadSettingsFilesDuringInitialization = true;

bool vtkInitializationHelper::SaveUserSettingsFileDuringFinalization = false;

std::string vtkInitializationHelper::OrganizationName = "ParaView";
std::string vtkInitializationHelper::ApplicationName = "GenericParaViewApplication";

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
void vtkInitializationHelper::Initialize(const char* executable, int type)
{
  vtkInitializationHelper::Initialize(executable, type, NULL);
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(const char* executable, int type, vtkPVOptions* options)
{
  if (!executable)
  {
    vtkGenericWarningMacro("Executable name has to be defined.");
    return;
  }

  // Pass the program name to make option parser happier
  vtkSmartPointer<vtkPVOptions> newoptions = options;
  if (!options)
  {
    newoptions = vtkSmartPointer<vtkPVOptions>::New();
  }

  std::vector<char*> argv;
  argv.push_back(vtksys::SystemTools::DuplicateString(executable));
  if (newoptions->GetForceNoMPIInitOnClient())
  {
    argv.push_back(vtksys::SystemTools::DuplicateString("--no-mpi"));
  }
  if (newoptions->GetForceMPIInitOnClient())
  {
    argv.push_back(vtksys::SystemTools::DuplicateString("--mpi"));
  }

  argv.push_back(nullptr);
  vtkInitializationHelper::Initialize(
    static_cast<int>(argv.size()) - 1, &argv[0], type, newoptions);

  for (auto tofree : argv)
  {
    delete[] tofree;
  }
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(int argc, char** argv, int type, vtkPVOptions* options)
{
  if (vtkProcessModule::GetProcessModule())
  {
    vtkGenericWarningMacro("Process already initialize. Skipping.");
    return;
  }

  if (!options)
  {
    vtkGenericWarningMacro("vtkPVOptions must be specified.");
    return;
  }

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vtkProcessModule::Initialize(static_cast<vtkProcessModule::ProcessTypes>(type), argc, argv);

  std::ostringstream sscerr;
  if (argv && !options->Parse(argc, argv))
  {
    if (options->GetUnknownArgument())
    {
      sscerr << "Got unknown argument: " << options->GetUnknownArgument()
             << ". Could you have misspelled your Python file path or name?" << endl;
    }
    if (options->GetErrorMessage())
    {
      sscerr << "Error: " << options->GetErrorMessage() << endl;
    }
    options->SetHelpSelected(1);
  }
  if (options->GetHelpSelected())
  {
    sscerr << options->GetHelp() << endl;
    vtkOutputWindow::GetInstance()->DisplayText(sscerr.str().c_str());
    // TODO: indicate to the caller that application must quit.
  }

  if (options->GetTellVersion())
  {
    std::ostringstream str;
    str << "paraview version " << PARAVIEW_VERSION_FULL << "\n";
    vtkOutputWindow::GetInstance()->DisplayText(str.str().c_str());
    // TODO: indicate to the caller that application must quit.
  }

  if (options->GetPrintMonitors())
  {
    std::string monitors = ListAttachedMonitors();
    vtkOutputWindow::GetInstance()->DisplayText(monitors.c_str());
    // TODO: indicate to the caller that application must quit.
  }

  vtkProcessModule::GetProcessModule()->SetOptions(options);

  // this has to happen after process module is initialized and options have
  // been set.
  paraview_initialize();

  // Set multi-server flag to vtkProcessModule
  vtkProcessModule::GetProcessModule()->SetMultipleSessionsSupport(
    options->GetMultiServerMode() != 0);

  // Make sure the ProxyManager get created...
  vtkSMProxyManager::GetProxyManager();

  // Now load any plugins located in the PV_PLUGIN_PATH environment variable.
  // These are always loaded (not merely located).
  vtkNew<vtkPVPluginLoader> loader;
  loader->LoadPluginsFromPluginSearchPath();
  loader->LoadPluginsFromPluginConfigFile();

  vtkInitializationHelper::SaveUserSettingsFileDuringFinalization = false;
  // Load settings files on client-processes.
  if (!options->GetDisableRegistry() && type != vtkProcessModule::PROCESS_SERVER &&
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

  // until we replace PARAVIEW_SMTESTDRIVER with something cleaner, we have
  // to print this greeting out when the process is launched from
  // smTestDriver
  if (vtksys::SystemTools::HasEnv("PARAVIEW_SMTESTDRIVER"))
  {
    cout << "Process started" << endl;
  }
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
    bool savingSucceeded = settings->SaveSettingsToFile(userSettingsFilePath.c_str());
    if (!savingSucceeded)
    {
      vtkGenericWarningMacro(<< "Saving settings file to '" << userSettingsFilePath << "' failed");
    }
  }

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
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  const char* app_dir_p = options->GetApplicationPath();
  std::string app_dir = app_dir_p ? app_dir_p : "";
  app_dir = vtksys::SystemTools::GetProgramPath(app_dir.c_str());

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

  std::string filename = vtkInitializationHelper::GetApplicationName() + "-SiteSettings.json";
  std::string siteSettingsFile;
  for (size_t cc = 0; cc < pathsToSearch.size(); cc++)
  {
    std::string path = pathsToSearch[cc];
    siteSettingsFile = path + "/" + filename;
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
