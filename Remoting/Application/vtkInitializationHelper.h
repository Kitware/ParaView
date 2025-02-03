// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInitializationHelper
 * @brief   Helpers class to initialize ParaView Clients.
 *
 * It initializes the server manager so that it can be used.
 *
 * This class is in fact a collection of static methods to
 * initialize different part of the application:
 * parse command line argument, load settings, initialize a vtkProcessModule.
 *
 * A main `Initialize` method calls subsequent methods
 * in the appropriate order for ParaView.
 *
 * Also cleanup is available through `Finalize`.
 */

#ifndef vtkInitializationHelper_h
#define vtkInitializationHelper_h

#include "vtkObject.h"
#include "vtkParaViewDeprecation.h"       // for deprecation macros
#include "vtkRemotingApplicationModule.h" // needed for exports
#include <string>                         // needed for std::string

#if PARAVIEW_USE_PYTHON
#include "vtkPythonInterpreter.h"
#endif

class vtkCLIOptions;
class vtkRemotingCoreConfiguration;
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
   * Internally calls, in this order:
   *  - InitializeProcessModule
   *  - InitializeGlobalOptions
   *  - InitializeSettings
   *  - InitializeOtherOptions
   *  - InitializeOthers
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
   * Initialize the vtkProcessModule used in ParaView
   *
   * This method should be called before any other initialization method
   * Unless that method calls it.
   *
   * This method should be called only once
   *
   * Returns `true` on success, `false` otherwise.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization.
   */
  static bool InitializeProcessModule(int argc, char** argv, int type);

  /**
   * Initialize only the global options (see vtkCLIOptions) of ParaView engine but
   * do not check for options that cause early exit of the program.
   *
   * Returns `true` on success, `false` otherwise.
   * When `false`, use `GetExitCode` to obtain the exit code.
   *
   * If options is nullptr, then this method internally creates and uses
   * an internal vtkCLIOptions instance. In that case, extra / unknown arguments
   * are simply ignored.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization, in combination with InitializeOtherOptions.
   */
  static bool InitializeGlobalOptions(int argc, char** argv, int processType,
    vtkCLIOptions* options = nullptr, bool enableStandardArgs = true);

  /**
   * Initialize all options of ParaView engine but the global options, then
   * check for options that cause early exit of the program.
   *
   * Returns `true` on success, `false` otherwise.
   * When `false`, use `GetExitCode` to obtain the exit code. Note, for
   * options that cause early exit of the program like `--help`, `--version` etc,
   * this method returns `false` with exit-code set to 0.
   *
   * If options is nullptr, then this method internally creates and uses
   * an internal vtkCLIOptions instance. In that case, extra / unknown arguments
   * are simply ignored.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization, in combination with InitializeGlobalOptions.
   */
  static bool InitializeOtherOptions(int argc, char** argv, int processType,
    vtkCLIOptions* options = nullptr, bool enableStandardArgs = true);

  /**
   * Initialize the process module and options of ParaView engine.
   *
   * Returns `true` on success, `false` otherwise.
   * When `false`, use `GetExitCode` to obtain the exit code. Note, for
   * options that cause early exit of the program like `--help`, `--version` etc,
   * this method returns `false` with exit-code set to 0.
   *
   * If options is nullptr, then this method internally creates and uses
   * an internal vtkCLIOptions instance. In that case, extra / unknown arguments
   * are simply ignored.
   *
   * This method call InitializeProcessModule, then initialize all options.
   */
  static bool InitializeOptions(int argc, char** argv, int processType,
    vtkCLIOptions* options = nullptr, bool enableStandardArgs = true);

  /**
   * Initialize the setting by reading the settings file,
   * unless coreConfig DisableRegistry is set to true.
   * Assume that InitializeProcessModule has been called.
   *
   * If defaultCoreConfig is set to true, this will use
   * the setting to set default values on the vtkRemotingCoreConfiguration
   *
   * This method always returns `true`.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization.
   */
  static bool InitializeSettings(int type, bool defaultCoreConfig);

  /**
   * Initialize everything that is not initialized by specific methods,
   * see Initialize method for more info.
   *
   * Assume that InitializeProcessModule has been called.
   * Returns `true` on success, `false` otherwise.
   *
   * This method is used by Initialize but can be used when separating options initialization
   * from the rest of the initialization.
   */
  static bool InitializeOthers();

  /**
   * Initialize everything that needs to be initialized in the paraview engine after the options.
   * Returns `true` on success, `false` otherwise.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use InitializeSettings and InitializeOthers instead")
  static bool InitializeMiscellaneous(int type);

  /**
   * Initialize Python virtual environment from --venv command-line argument if any was provided.
   *
   * Make sure that the vtkPythonInterpreter has been initialized before calling this function.
   */
  static void InitializePythonVirtualEnvironment();

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
  PARAVIEW_DEPRECATED_IN_5_13_0("Use vtkPVStandardPaths::GetUserSettingsDirectory instead")
  static std::string GetUserSettingsDirectory();

  /**
   * Get file path for the user settings file.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use vtkPVStandardPaths::GetUserSettingsFilePath instead")
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

  /**
   * Parse options and optionally check the code config for options that
   * cause early exit of the program like `--help` or `--version`.
   */
  static bool ParseOptions(int argc, char** argv, vtkCLIOptions* options,
    vtkRemotingCoreConfiguration* coreConfig, bool checkForExit);

  static bool LoadSettingsFilesDuringInitialization;
  static bool SaveUserSettingsFileDuringFinalization;
  static std::string OrganizationName;
  static std::string ApplicationName;
  static int ExitCode;
};

#endif
