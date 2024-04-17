// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProcessModuleConfiguration.h"

#include "vtkCLIOptions.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtk_cli11.h>
#include <vtksys/SystemInformation.hxx>

vtkStandardNewMacro(vtkProcessModuleConfiguration);
//----------------------------------------------------------------------------
vtkProcessModuleConfiguration::vtkProcessModuleConfiguration()
{
  this->LogStdErrVerbosity = vtkLogger::VERBOSITY_INVALID;
}

//----------------------------------------------------------------------------
vtkProcessModuleConfiguration::~vtkProcessModuleConfiguration() = default;

//----------------------------------------------------------------------------
vtkProcessModuleConfiguration* vtkProcessModuleConfiguration::GetInstance()
{
  static auto Singleton = vtk::TakeSmartPointer(vtkProcessModuleConfiguration::New());
  return Singleton.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkProcessModuleConfiguration::PopulateOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes ptype)
{
  auto app = options->GetCLI11App();

  app
    ->add_option("-v,--verbosity", this->LogStdErrVerbosity,
      "Log verbosity on stderr as an integer in range [-9, 9] "
      "or INFO, WARNING, ERROR, or OFF. Defaults to INFO(0).")
    ->transform(
      [](const std::string& value) {
        auto xformedValue = vtkLogger::ConvertToVerbosity(value.c_str());
        if (xformedValue == vtkLogger::VERBOSITY_INVALID)
        {
          throw CLI::ValidationError("Invalid verbosity specified!");
        }
        return std::to_string(xformedValue);
      },
      "verbosity");

  auto groupLogging = app->add_option_group("Debugging / Logging", "Logging and debugging options");

  groupLogging->add_flag(
    "--enable-bt", this->EnableStackTrace, "Generate stack-trace on crash, if possible.");

  groupLogging
    ->add_option(
      "--cslog", this->CSLogFileName, "Filename to use to generate a ClientServerStream log.")
    ->check([](const std::string&) { return std::string(); }, "filename", "filename");

  groupLogging
    ->add_option(
      "-l,--log",
      [this](const CLI::results_t& results) {
        for (const auto& value : results)
        {
          const auto separator = value.find_last_of(',');
          if (separator != std::string::npos)
          {
            const auto verbosityString = value.substr(separator + 1);
            const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityString.c_str());
            if (verbosity == vtkLogger::VERBOSITY_INVALID)
            {
              vtkLogF(ERROR, "Invalid verbosity specified '%s'", verbosityString.c_str());
              // invalid verbosity specified.
              return false;
            }
            // remove the ",..." part from filename.
            this->LogFiles.emplace_back(value.substr(0, separator), verbosity);
          }
          else
          {
            this->LogFiles.emplace_back(value, vtkLogger::VERBOSITY_INFO);
          }
        }
        return true;
      },
      "Additional log files to generate. Can be specified multiple times. "
      "By default, log verbosity is set to INFO(0) and may be "
      "overridden per file by adding suffix `,verbosity` where verbosity values "
      "are same as those accepted for `--verbosity` argument.")
    ->delimiter('+') // reset delimiter. For log files ',' is used to separate verbosity.
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)
    ->type_name("TEXT:filename[,ENUM:verbosity] ...");

  auto group = app->add_option_group("MPI", "MPI-specific options");
  auto mpi = group->add_flag(
    "--mpi", this->ForceMPIInit, "Initialize MPI on current process, even if not necessary.");
  group
    ->add_flag("--no-mpi", this->ForceNoMPIInit,
      "Skip initializing MPI on current process, if not required.")
    ->excludes(mpi);
  group
    ->add_flag("--use-mpi-ssend", this->UseMPISSend,
      "Use 'MPI_SSend' instead of 'MPI_Send', whenever possible. Useful for debugging race "
      "conditions "
      "in distributed environments.")
    ->envname("PARAVIEW_USE_MPI_SSEND");
  if (ptype == vtkProcessModule::PROCESS_BATCH)
  {
    app->add_flag("-s,--sym,--symmetric", this->SymmetricMPIMode,
      "When specified, the python script is processed symmetrically on all processes.");
  }
#if PARAVIEW_USE_PYTHON
  app->add_option("--venv", this->VirtualEnvironmentPath,
    "Initialize python with a virtual environment at this path.");
#endif // PARAVIEW_USE_PYTHON

  return true;
}

//----------------------------------------------------------------------------
std::string vtkProcessModuleConfiguration::GetCSLogFileName() const
{
  return vtkProcessModuleConfiguration::GetRankAnnotatedFileName(this->CSLogFileName);
}

//----------------------------------------------------------------------------
std::string vtkProcessModuleConfiguration::GetRankAnnotatedFileName(const std::string& fname)
{
  if (fname.empty())
  {
    return {};
  }

  auto controller = vtkMultiProcessController::GetGlobalController();
  return (controller && controller->GetNumberOfProcesses() > 1)
    ? fname + "." + std::to_string(controller->GetLocalProcessId())
    : fname;
}

//----------------------------------------------------------------------------
void vtkProcessModuleConfiguration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ForceMPIInit: " << this->ForceMPIInit << endl;
  os << indent << "ForceNoMPIInit: " << this->ForceNoMPIInit << endl;
  os << indent << "SymmetricMPIMode: " << this->SymmetricMPIMode << endl;
  os << indent << "EnableStackTrace: " << this->EnableStackTrace << endl;
  os << indent << "LogStdErrVerbosity: " << this->LogStdErrVerbosity << endl;
  os << indent << "CSLogFileName: " << this->CSLogFileName.c_str() << endl;
  os << indent << "LogFiles (count=" << this->LogFiles.size() << "):" << endl;
  for (auto& pair : this->LogFiles)
  {
    os << indent.GetNextIndent() << pair.first << ", " << pair.second << endl;
  }
}
