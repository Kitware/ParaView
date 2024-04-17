// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkProcessModuleConfiguration
 * @brief runtime configuration for vtkProcessModule
 *
 * vtkProcessModuleConfiguration is a singleton that maintains runtime
 * configuration options that affect how vtkProcessModule is initialized.
 */

#ifndef vtkProcessModuleConfiguration_h
#define vtkProcessModuleConfiguration_h

#include "vtkLogger.h" // for vtkLogger::Verbosity
#include "vtkObject.h"
#include "vtkProcessModule.h"      // for vtkProcessModule::ProcessTypes
#include "vtkRemotingCoreModule.h" //needed for exports

#include <string> // for std::string
#include <vector> // for std::vector

class vtkCLIOptions;

class VTKREMOTINGCORE_EXPORT vtkProcessModuleConfiguration : public vtkObject
{
public:
  vtkTypeMacro(vtkProcessModuleConfiguration, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton.
   */
  static vtkProcessModuleConfiguration* GetInstance();

  /**
   * Get whether to call `MPI_Init()` on this process irrespective of
   * whether MPI is needed on this process.
   */
  vtkGetMacro(ForceMPIInit, bool);

  /**
   * Get whether to not call `MPI_Init()` on this process irrespective of
   * whether MPI is needed on this process.
   */
  vtkGetMacro(ForceNoMPIInit, bool);

  /**
   * Flag indicating whether to use synchronous send (SSend) for MPI
   * communication.
   */
  vtkGetMacro(UseMPISSend, bool);

  /**
   * Get whether to use symmetric MPI mode. In this mode is only supported
   * in "batch". In that case, all processes, including the satellites, execute
   * the Python script locally.
   */
  vtkGetMacro(SymmetricMPIMode, bool);

  /**
   * Ideally, this method should not be public. However, we are keeping it
   * public to avoid changing to much code at this time.
   */
  vtkSetMacro(SymmetricMPIMode, bool);

  /**
   * Get the virtual environment path to set up for Python.
   */
  vtkGetMacro(VirtualEnvironmentPath, std::string);

  /**
   * Get the verbosity level to use for reporting log messages on `stderr`.
   * In other words, all messages at the chosen level and higher are posted to
   * `stderr`. Default is `vtkLogger::VERBOSITY_INVALID`.
   */
  vtkGetMacro(LogStdErrVerbosity, vtkLogger::Verbosity);

  /**
   * Get the filename to use to generate client-server-stream logs, if any.
   * If running in a distributed mode, the filename is automatically changed to
   * include current rank index.
   */
  std::string GetCSLogFileName() const;

  /**
   * Get whether to generate a stack trace after a crash. This is currently
   * only supported on POSIX systems.
   */
  vtkGetMacro(EnableStackTrace, bool);

  /**
   * Returns a vector of pairs for log files requested.
   */
  const std::vector<std::pair<std::string, vtkLogger::Verbosity>>& GetLogFiles() const
  {
    return this->LogFiles;
  }

  /**
   * Populate command line options.
   * `processType` indicates which type of ParaView process the options are
   * being setup for. That may affect available options.
   */
  bool PopulateOptions(vtkCLIOptions* options, vtkProcessModule::ProcessTypes processType);

  /**
   * A helper function to add a rank number to the filename, if needed. This is
   * useful to create a separate log file per rank, for example, when executing
   * in parallel. If number of ranks is 1, the filename is unchanged.
   */
  static std::string GetRankAnnotatedFileName(const std::string& fname);

protected:
  vtkProcessModuleConfiguration();
  ~vtkProcessModuleConfiguration() override;

private:
  vtkProcessModuleConfiguration(const vtkProcessModuleConfiguration&) = delete;
  void operator=(const vtkProcessModuleConfiguration&) = delete;

  bool ForceMPIInit = false;
  bool ForceNoMPIInit = false;
  bool UseMPISSend = false;
  bool SymmetricMPIMode = false;
  std::string VirtualEnvironmentPath;
  bool EnableStackTrace = false;
  vtkLogger::Verbosity LogStdErrVerbosity = vtkLogger::VERBOSITY_INVALID;
  std::string CSLogFileName;
  std::vector<std::pair<std::string, vtkLogger::Verbosity>> LogFiles;
  static vtkProcessModuleConfiguration* New();
};

#endif
