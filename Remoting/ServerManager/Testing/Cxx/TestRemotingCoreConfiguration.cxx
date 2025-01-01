// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCLIOptions.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"

extern int TestRemotingCoreConfiguration(int argc, char* argv[])
{
  vtkNew<vtkCLIOptions> options;
  options->SetName("TestRemotingCoreConfiguration");
  options->SetDescription("Test for 'vtkRemotingCoreConfiguration'");

  vtkProcessModuleConfiguration::GetInstance()->PopulateOptions(
    options, vtkProcessModule::PROCESS_CLIENT);
  vtkRemotingCoreConfiguration::GetInstance()->PopulateOptions(
    options, vtkProcessModule::PROCESS_CLIENT);
  vtkLogF(INFO, "%s", options->GetHelp());

  options->Parse(argc, argv);
  vtkRemotingCoreConfiguration::GetInstance()->Print(cout);
  return EXIT_SUCCESS;
}
