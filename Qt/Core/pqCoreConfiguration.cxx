// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCoreConfiguration.h"

#include "vtkCLIOptions.h"
#include "vtk_cli11.h"

namespace
{
bool EndsWith(const std::string& name, const std::string& suffix)
{
  return (name.size() >= suffix.size() && name.substr(name.size() - suffix.size()) == suffix);
}
}

//-----------------------------------------------------------------------------
pqCoreConfiguration::pqCoreConfiguration() = default;

//-----------------------------------------------------------------------------
pqCoreConfiguration::~pqCoreConfiguration() = default;

//-----------------------------------------------------------------------------
pqCoreConfiguration* pqCoreConfiguration::instance()
{
  static pqCoreConfiguration config;
  return &config;
}

//-----------------------------------------------------------------------------
bool pqCoreConfiguration::populateOptions(vtkCLIOptions* options)
{
  auto app = options->GetCLI11App();

  // Clear DataFileNames to support parsing multiple times
  app->preparse_callback([this](std::size_t) { this->DataFileNames.clear(); });

  auto groupTesting = app->add_option_group("Testing", qPrintable(tr("Testing specific options")));

  groupTesting
    ->add_option("--baseline-directory", this->BaselineDirectory,
      qPrintable(tr("Baseline directory where test recorder will store baseline images.")))
    ->envname("PARAVIEW_TEST_BASELINE_DIR");
  groupTesting
    ->add_option("--test-directory", this->TestDirectory,
      qPrintable(tr("Temporary directory used to output test results and other temporary files.")))
    ->envname("PARAVIEW_TEST_DIR");
  groupTesting
    ->add_option("--data-directory", this->DataDirectory,
      qPrintable(tr("Directory containing data files for tests.")))
    ->envname("PARAVIEW_DATA_ROOT");
  groupTesting
    ->add_option(
      "--test-script",
      [this](const CLI::results_t& args)
      {
        if (this->TestScripts.empty())
        {
          this->TestScripts.resize(static_cast<int>(args.size()));
        }
        if (this->TestScripts.size() != static_cast<int>(args.size()))
        {
          return false;
        }
        for (size_t cc = 0; cc < args.size(); ++cc)
        {
          this->TestScripts[static_cast<int>(cc)].FileName = args[cc];
        }
        return true;
      },
      qPrintable(tr("Test scripts to execute in order specified on the command line.")))
    ->type_name("TEXT:filename ...")
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

  groupTesting
    ->add_option(
      "--test-baseline",
      [this](const CLI::results_t& args)
      {
        if (this->TestScripts.empty())
        {
          this->TestScripts.resize(static_cast<int>(args.size()));
        }
        if (this->TestScripts.size() != static_cast<int>(args.size()))
        {
          return false;
        }
        for (size_t cc = 0; cc < args.size(); ++cc)
        {
          this->TestScripts[static_cast<int>(cc)].Baseline = args[cc];
        }
        return true;
      },
      qPrintable(tr("Test baseline for tests script provided.")))
    ->type_name("TEXT:filename ...")
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

  groupTesting
    ->add_option(
      "--test-threshold",
      [this](const CLI::results_t& args)
      {
        if (this->TestScripts.empty())
        {
          this->TestScripts.resize(static_cast<int>(args.size()));
        }
        if (this->TestScripts.size() != static_cast<int>(args.size()))
        {
          return false;
        }
        for (size_t cc = 0; cc < args.size(); ++cc)
        {
          this->TestScripts[static_cast<int>(cc)].Threshold = std::atof(args[cc].c_str());
        }
        return true;
      },
      qPrintable(tr("Test image comparison threshold for test scripts provided.")))
    ->type_name("FLOAT ...")
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

  groupTesting->add_flag("--exit", this->ExitAppWhenTestsDone,
    qPrintable(tr("Exit application when tests are finished.")));

  groupTesting->add_flag("--test-master", this->TestMaster,
    qPrintable(tr("Test collaboration 'master' configuration.")));
  groupTesting->add_flag(
    "--test-slave", this->TestSlave, qPrintable(tr("Test collaboration 'slave' configuration.")));

  CLI::deprecate_option(groupTesting, "--test-master");
  CLI::deprecate_option(groupTesting, "--test-slave");

  auto groupStartup = app->add_option_group(
    "Startup", qPrintable(tr("Options controls actions on application launch")));
  groupStartup->add_option("--state", this->StateFileName,
    qPrintable(tr("State file (.pvsm or .py) to load when the application starts.")));
  groupStartup
    ->add_option("--script", this->PythonScript,
      qPrintable(tr("Python script to execute when the application starts.")))
    ->excludes("--state");
  groupStartup
    ->add_option("--data", this->DataFileNames,
      qPrintable(tr(
        "Load the specified data file(s) when the client starts. To choose a file series, "
        "replace the numeral with a '.', for example,  my0.vtk, my1.vtk...myN.vtk becomes my..vtk, "
        "etc.")))
    ->excludes("--state")
    ->excludes("--script");

  groupStartup
    ->add_option("filenames", this->PositionalFileNames,
      qPrintable(tr(
        "Positional arguments may be used to pass either data (--data), state (--state), "
        "or script (--script) files. `.pvsm` files are treated as state files, `.py` are treated "
        "as scripts and all others are treated as data files.")))
    ->excludes("--state")
    ->excludes("--script")
    ->excludes("--data")
    ->each(
      [this](const std::string& value)
      {
        if (::EndsWith(value, ".pvsm"))
        {
          this->StateFileName = value;
        }
        else if (::EndsWith(value, ".py"))
        {
          this->PythonScript = value;
        }
        else
        {
          this->DataFileNames.push_back(value);
        }
      });

  groupStartup
    ->add_option("--live", this->CatalystLivePort,
      qPrintable(tr("Connect to Catalyst Live session at the specified port number.")))
    ->default_val(-1);

  return true;
}
