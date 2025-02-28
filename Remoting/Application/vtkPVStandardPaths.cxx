// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVStandardPaths.h"

#include "vtkInitializationHelper.h"
#include "vtkPVLogger.h"
#include "vtkPVVersionQuick.h"
#include "vtkProcessModule.h"

#include <algorithm>
#include <cctype>

namespace os
{
#if defined(_WIN32)
//----------------------------------------------------------------------------
std::string GetWindowsUserSettingsDirectory()
{
  std::string organizationName(vtkInitializationHelper::GetOrganizationName());
  std::string appData;
  if (vtksys::SystemTools::GetEnv("APPDATA", appData))
  {
    vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(), << "found APPDATA env: " << appData);
    std::string separator("\\");
    if (appData[appData.size() - 1] != separator[0])
    {
      appData.append(separator);
    }
    appData += organizationName + separator;
  }

  return appData;
}

//----------------------------------------------------------------------------
std::vector<std::string> GetWindowsSystemDirectories()
{
  std::vector<std::string> paths;
  std::string appData;
  // NOTE: Seems to be known as "ProgramData" now:
  // https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid
  if (vtksys::SystemTools::GetEnv("COMMON_APPDATA", appData))
  {
    vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(), << "found COMMON_APPDATA env: " << appData);
    paths.push_back(appData);
  }

  return paths;
}

#else
//----------------------------------------------------------------------------
std::string GetUnixUserSettingsDirectory()
{
  std::string organizationName(vtkInitializationHelper::GetOrganizationName());
  std::string directoryPath;
  std::string separator("/");

  // Emulating QSettings behavior.
  std::string xdgConfigHome;
  if (vtksys::SystemTools::GetEnv("XDG_CONFIG_HOME", xdgConfigHome))
  {
    vtkVLog(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), << "found XDG_CONFIG_HOME env: " << xdgConfigHome);
    directoryPath = xdgConfigHome;
    if (directoryPath[directoryPath.size() - 1] != separator[0])
    {
      directoryPath += separator;
    }
  }
  else
  {
    std::string home;
    if (!vtksys::SystemTools::GetEnv("HOME", home))
    {
      return std::string();
    }
    vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(), << "found HOME env: " << home);
    directoryPath = home;
    if (directoryPath[directoryPath.size() - 1] != separator[0])
    {
      directoryPath += separator;
    }
    directoryPath += ".config/";
  }
  directoryPath += organizationName + separator;
  return directoryPath;
}

//----------------------------------------------------------------------------
std::vector<std::string> GetUnixSystemDirectories()
{
  std::vector<std::string> paths;
  // see https://specifications.freedesktop.org/basedir-spec/latest/
  std::string dataDirs;
  if (vtksys::SystemTools::GetEnv("XDG_DATA_DIRS", dataDirs))
  {
    vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(), << "found XDG_DATA_DIRS env: " << dataDirs);
    std::string envDir(dataDirs);
    std::vector<std::string> dirs = vtksys::SystemTools::SplitString(envDir, ':');
    for (const std::string& directory : dirs)
    {
      paths.push_back(directory);
    }
  }
  paths.push_back("/usr/local/share");
  paths.push_back("/usr/share");
  return paths;
}
#endif
};

namespace vtkPVStandardPaths
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
std::vector<std::string> GetSystemDirectories()
{
#if defined(_WIN32)
  return os::GetWindowsSystemDirectories();
#else
  return os::GetUnixSystemDirectories();
#endif
}

//----------------------------------------------------------------------------
std::vector<std::string> GetInstallDirectories()
{
  std::vector<std::string> installDirectories;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkLogF(WARNING, "Cannot retrieve InstallDirectories without a vtkProcessModule initialized.");
    return installDirectories;
  }

  const auto& app_dir = pm->GetSelfDir();

  std::string app_name = vtkInitializationHelper::GetApplicationName();
  std::transform(app_name.begin(), app_name.end(), app_name.begin(),
    [](unsigned char c) { return std::tolower(c); });

  // If the application path ends with lib/<app_name>-X.X, shared
  // forwarding of the executable was used. Remove that part of the
  // path to get back to the installation root.
  auto appPathEnds = app_dir.find("/lib/" + app_name + "-" PARAVIEW_VERSION);
  std::string rootDirectory = app_dir;
  if (appPathEnds != std::string::npos)
  {
    rootDirectory = app_dir.substr(0, appPathEnds);
  }

  // next to executable
  installDirectories.push_back(rootDirectory);

  // Remove the trailing /bin if it is there.
  if (rootDirectory.size() >= 4 && rootDirectory.substr(rootDirectory.size() - 4) == "/bin")
  {
    rootDirectory = rootDirectory.substr(0, rootDirectory.size() - 4);
    installDirectories.push_back(rootDirectory);
  }

  installDirectories.push_back(rootDirectory + "/share/" + app_name + "-" PARAVIEW_VERSION);
  installDirectories.push_back(rootDirectory + "/lib");
#if defined(__APPLE__)
  // paths for app
  installDirectories.push_back(rootDirectory + "/../../..");
  installDirectories.push_back(rootDirectory + "/../../../../lib");

  // path when doing a unix style install.
  installDirectories.push_back(rootDirectory + "/../lib/" + app_name + "-" PARAVIEW_VERSION);

  installDirectories.push_back(rootDirectory + "/../Support");
  installDirectories.push_back(rootDirectory + "/../../../Support");
#endif
  // On windows configuration files are in the parent directory
  installDirectories.push_back(rootDirectory + "/..");

  return installDirectories;
}

//----------------------------------------------------------------------------
std::string GetUserSettingsDirectory()
{
#if defined(_WIN32)
  return os::GetWindowsUserSettingsDirectory();
#else
  return os::GetUnixUserSettingsDirectory();
#endif
}

//----------------------------------------------------------------------------
std::string GetUserSettingsFilePath()
{
  std::string path = GetUserSettingsDirectory();
  path.append(vtkInitializationHelper::GetApplicationName());
  path.append("-UserSettings.json");

  return path;
}

VTK_ABI_NAMESPACE_END
};
