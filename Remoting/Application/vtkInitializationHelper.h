// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInitializationHelper
 * @brief   help class for python modules
 *
 * This class is used by the python modules when they are loaded from
 * python (as opposed to pvpython). It simply initializes the server
 * manager so that it can be used.
 */

#ifndef vtkInitializationHelper_h
#define vtkInitializationHelper_h

#include "vtkObject.h"
#include "vtkRemotingApplicationModule.h" // needed for exports
#include <string>                         // needed for std::string

class vtkCLIOptions;
class vtkStringList;

class VTKREMOTINGAPPLICATION_EXPORT vtkInitializationHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkInitializationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes ParaView engine. Returns `true` on success, `false` otherwise.
   * When `false`, use `GetExitCode` to obtain the exit code. Note, for requests
   * like `--help`, `--version` etc, this method returns `false` with exit-code
   * set to 0.
   *
   * If vtkCLIOptions is nullptr, then this method internally creates and uses
   * an internal vtkCLIOptions instance. In that case, extra / unknown arguments
   * are simply ignored.
   *
   * Internally calls InitializeOptions and InitializeMiscellaneous.
   */
  static bool Initialize(int argc, char** argv, int processType, vtkCLIOptions* options = nullptr,
    bool enableStandardArgs = true);

  /**
   * Overload primary intended for Python wrapping.
   */
  static bool Initialize(vtkStringList* argv, int processType);

  /**
   * An overload that does not take argc/argv for convenience.
   */
  static bool Initialize(const char* executable, int type);

  /**
   * Initialize only the options of ParaView engine.
   *
   * Returns `true` on success, `false` otherwise.
   * When `false`, use `GetExitCode` to obtain the exit code. Note, for requests
   * like `--help`, `--version` etc, this method returns `false` with exit-code
   * set to 0.
   *
   * If vtkCLIOptions is nullptr, then this method internally creates and uses
   * an internal vtkCLIOptions instance. In that case, extra / unknown arguments
   * are simply ignored.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization.
   */
  static bool InitializeOptions(int argc, char** argv, int processType,
    vtkCLIOptions* options = nullptr, bool enableStandardArgs = true);

  /**
   * Initialize everything that needs to be initialized in the paraview engine after the options.
   * Returns `true` on success, `false` otherwise.
   *
   * Make sure to call InitializeOptions before calling this method.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization.
   */
  static bool InitializeMiscellaneous(int type);

  /**
   * Finalizes the server manager. Do not use the server manager
   * after calling this.
   */
  static void Finalize();

  /**
   * Returns the exit code after `Initialize`.
   */
  static int GetExitCode() { return vtkInitializationHelper::ExitCode; }

  ///@{
  /**
   * Initialization for standalone executables linking against a PV
   * library. This is needed to insure that linker does not remove object
   * factories' auto init during static linking. It also cleans up after
   * protobuf.
   */
  static void StandaloneInitialize();
  static void StandaloneFinalize();
  ///@}

  ///@{
  /**
   * During initialization, vtkInitializationHelper reads "settings" files for
   * configuring vtkSMSettings. To disable this processing of the settings file
   * for an application (e.g. in Catalyst), turn this off. On by default.
   */
  static void SetLoadSettingsFilesDuringInitialization(bool);
  static bool GetLoadSettingsFilesDuringInitialization();
  ///@}

  ///@{
  /**
   * Sets the organization producing this application. This is
   * "ParaView" by default, but can be different for branded applications.
   */
  static void SetOrganizationName(const std::string& organizationName);
  static const std::string& GetOrganizationName();
  ///@}

  ///@{
  /**
   * Sets the name of the application. This is "ParaView" by default, but
   * can be different for branded applications.
   */
  static void SetApplicationName(const std::string& appName);
  static const std::string& GetApplicationName();
  ///@}

  /**
   * Get directory for user settings file. The last character is always the
   * file path separator appropriate for the system.
   */
  static std::string GetUserSettingsDirectory();

  /**
   * Get file path for the user settings file.
   */
  static std::string GetUserSettingsFilePath();

protected:
  vtkInitializationHelper() = default;
  ~vtkInitializationHelper() override = default;

  /**
   * Load user and site settings
   */
  static void LoadSettings();

private:
  vtkInitializationHelper(const vtkInitializationHelper&) = delete;
  void operator=(const vtkInitializationHelper&) = delete;

  static bool LoadSettingsFilesDuringInitialization;
  static bool SaveUserSettingsFileDuringFinalization;
  static std::string OrganizationName;
  static std::string ApplicationName;
  static int ExitCode;
};

#endif
