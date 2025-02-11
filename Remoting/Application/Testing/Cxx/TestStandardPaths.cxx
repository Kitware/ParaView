// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInitializationHelper.h"
#include "vtkPVStandardPaths.h"
#include "vtkProcessModule.h"

#include <string>
#include <vector>

// Some of those static values match environment var, defined in the CMakeLists
namespace results
{
#if defined(_WIN32)
static std::vector<std::string> systemDirs = { "C:\\test\\syspath" };
static std::string userDir = "D:\\test\\appdata\\ParaViewTesting\\";
// #elif defined (__APPLE__)
#else
static std::vector<std::string> systemDirs = { "/test/syspath", "/test/othersyspath",
  "/usr/local/share", "/usr/share" };
static std::string userDir = "/test/config/ParaViewTesting/";
#endif

static bool compareVectors(
  const std::vector<std::string>& expected, const std::vector<std::string>& retrieved)
{
  if (expected.size() != retrieved.size())
  {
    cerr << "Different number of paths! Has " << retrieved.size() << ":\n";
    for (const std::string& path : retrieved)
    {
      cerr << path << "\n";
    }
    return false;
  }

  for (size_t idx = 0; idx < expected.size(); idx++)
  {
    if (expected[idx] != retrieved[idx])
    {
      cerr << "Paths do not match. At index " << idx << " Expect " << expected[idx] << " but has "
           << retrieved[idx] << "\n";
      return false;
    }
  }

  return true;
}
};

extern int TestStandardPaths(int argc, char* argv[])
{
  std::vector<std::string> retrievenSystemDirs = vtkPVStandardPaths::GetSystemDirectories();

  if (!results::compareVectors(results::systemDirs, retrievenSystemDirs))
  {
    cerr << "System Directories failed\n";
    return EXIT_FAILURE;
  }

  // UserDir uses organization name.
  vtkInitializationHelper::SetApplicationName("TestStandardPaths");
  vtkInitializationHelper::SetOrganizationName("ParaViewTesting");
  std::string userDir = vtkPVStandardPaths::GetUserSettingsDirectory();
  if (userDir != results::userDir)
  {
    cerr << "Wrong user directory found. Has " << userDir << " but expects " << results::userDir
         << "\n";
    return EXIT_FAILURE;
  }

  // InstallDirs requires an initialized process module.
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);
  std::vector<std::string> installDirs = vtkPVStandardPaths::GetInstallDirectories();

  vtkInitializationHelper::Finalize();

  return installDirs.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
}
