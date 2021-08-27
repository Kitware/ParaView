/*=========================================================================

   Program: ParaView
   Module:  pqCoreConfiguration.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqCoreConfiguration.h"

#include "vtkCLIOptions.h"
#include "vtk_cli11.h"

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

  auto groupTesting = app->add_option_group("Testing", "Testing specific options");

  groupTesting
    ->add_option("--baseline-directory", this->BaselineDirectory,
      "Baseline directory where test recorder will store baseline images.")
    ->envname("PARAVIEW_TEST_BASELINE_DIR");
  groupTesting
    ->add_option("--test-directory", this->TestDirectory,
      "Temporary directory used to output test results and other temporary files.")
    ->envname("PARAVIEW_TEST_DIR");
  groupTesting
    ->add_option(
      "--data-directory", this->DataDirectory, "Directory containing data files for tests.")
    ->envname("PARAVIEW_DATA_ROOT");
  groupTesting
    ->add_option(
      "--test-script",
      [this](const CLI::results_t& args) {
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
      "Test scripts to execute in order specified on the command line.")
    ->type_name("TEXT:filename ...")
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

  groupTesting
    ->add_option(
      "--test-baseline",
      [this](const CLI::results_t& args) {
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
      "Test baseline for tests script provided.")
    ->type_name("TEXT:filename ...")
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

  groupTesting
    ->add_option(
      "--test-threshold",
      [this](const CLI::results_t& args) {
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
          this->TestScripts[static_cast<int>(cc)].Threshold = std::atoi(args[cc].c_str());
        }
        return true;
      },
      "Test image comparison threshold for test scripts provided.")
    ->type_name("INT ...")
    ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

  groupTesting->add_flag(
    "--exit", this->ExitAppWhenTestsDone, "Exit application when tests are finished.");

  groupTesting->add_flag(
    "--test-master", this->TestMaster, "Test collaboration 'master' configuration.");
  groupTesting->add_flag(
    "--test-slave", this->TestSlave, "Test collaboration 'slave' configuration.");

  CLI::deprecate_option(groupTesting, "--test-master");
  CLI::deprecate_option(groupTesting, "--test-slave");

  auto groupStartup =
    app->add_option_group("Startup", "Options controls actions on application launch");
  groupStartup->add_option("--state", this->StateFileName,
    "State file (.pvsm or .py) to load when the application starts.");
  groupStartup
    ->add_option(
      "--script", this->PythonScript, "Python script to execute when the application starts.")
    ->excludes("--state");
  groupStartup
    ->add_option("data,--data", this->DataFileNames,
      "Load the specified data file(s) when the client starts. To choose a file series, "
      "replace the numeral with a '.', for example,  my0.vtk, my1.vtk...myN.vtk becomes my..vtk, "
      "etc.")
    ->excludes("--state")
    ->excludes("--script");

  groupStartup
    ->add_option("--live", this->CatalystLivePort,
      "Connect to Catalyst Live session at the specified port number.")
    ->default_val(-1);

  return true;
}
