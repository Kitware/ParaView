/*=========================================================================

Program:   ParaView
Module:    pvpython.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h" // Required to get build options for paraview

extern "C" {
void vtkPVInitializePythonModules();
}

#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVPythonOptions.h"
#include "vtkProcessModule.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMSession.h"

#include <vector>
#include <vtksys/SystemTools.hxx>

#include "ParaView_paraview_plugins.h"

namespace ParaViewPython
{

//---------------------------------------------------------------------------

void ProcessArgsForPython(
  std::vector<char*>& pythonArgs, const char* first_unknown_arg, int argc, char* argv[])
{
  pythonArgs.clear();
  pythonArgs.push_back(vtksys::SystemTools::DuplicateString(argv[0]));
  if (first_unknown_arg && strcmp(first_unknown_arg, "--") == 0)
  {
    // `--` causes ParaView to not process the arguments anymore and simply
    // pass them to the Python interpreter.
  }
  else if (first_unknown_arg && strlen(first_unknown_arg) > 0)
  {
    // here we handle a special case when the filename specified is a zip
    // archive.
    if (vtksys::SystemTools::GetFilenameLastExtension(first_unknown_arg) == ".zip")
    {
      // add the archive to sys.path
      vtkPythonInterpreter::PrependPythonPath(first_unknown_arg);
      pythonArgs.push_back(vtksys::SystemTools::DuplicateString("-m"));

      std::string modulename = vtksys::SystemTools::GetFilenameWithoutLastExtension(
        vtksys::SystemTools::GetFilenameName(first_unknown_arg));
      pythonArgs.push_back(vtksys::SystemTools::DuplicateString(modulename.c_str()));
    }
    else
    {
      pythonArgs.push_back(vtksys::SystemTools::DuplicateString(first_unknown_arg));
    }
  }

  for (int cc = 1; cc < argc; cc++)
  {
    pythonArgs.push_back(vtksys::SystemTools::DuplicateString(argv[cc]));
  }
}

//---------------------------------------------------------------------------
int Run(int processType, int argc, char* argv[])
{
  // Setup options
  // Marking this static avoids the false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any non-static objects
  // created before MPI_Init().
  vtkInitializationHelper::SetApplicationName("ParaView");
  static vtkSmartPointer<vtkPVPythonOptions> options = vtkSmartPointer<vtkPVPythonOptions>::New();
  vtkInitializationHelper::Initialize(argc, argv, processType, options);
  if (options->GetTellVersion() || options->GetHelpSelected() || options->GetPrintMonitors())
  {
    vtkInitializationHelper::Finalize();
    return EXIT_SUCCESS;
  }

  if (processType == vtkProcessModule::PROCESS_BATCH && options->GetUnknownArgument() == nullptr)
  {
    vtkGenericWarningMacro("No script specified. "
                           "Please specify a batch script or use 'pvpython'.");
    vtkInitializationHelper::Finalize();
    return EXIT_FAILURE;
  }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();

  // register static plugins
  ParaView_paraview_plugins_initialize();

  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLs("paraview");

  int ret_val = 0;
  if (pm->GetSymmetricMPIMode() == false && pm->GetPartitionId() > 0)
  {
    vtkIdType sid = vtkSMSession::ConnectToSelf();
    pm->GetGlobalController()->ProcessRMIs();
    pm->UnRegisterSession(sid);
  }
  else
  {
    int remaining_argc;
    char** remaining_argv;
    options->GetRemainingArguments(&remaining_argc, &remaining_argv);

    // Process arguments
    std::vector<char*> pythonArgs;
    ProcessArgsForPython(pythonArgs, options->GetUnknownArgument(), remaining_argc, remaining_argv);
    pythonArgs.push_back(nullptr);

    // if user specified verbosity option on command line, then we make vtkPythonInterpreter post
    // log information as INFO, otherwise we leave it at default which is TRACE.
    vtkPythonInterpreter::SetLogVerbosity(
      options->GetLogStdErrVerbosity() != vtkLogger::VERBOSITY_INVALID
        ? vtkLogger::VERBOSITY_INFO
        : vtkLogger::VERBOSITY_TRACE);

    // Start interpretor
    vtkPythonInterpreter::Initialize();

    ret_val =
      vtkPythonInterpreter::PyMain(static_cast<int>(pythonArgs.size()) - 1, &*pythonArgs.begin());

    // Free python args
    std::vector<char*>::iterator it = pythonArgs.begin();
    while (it != pythonArgs.end())
    {
      delete[] * it;
      ++it;
    }
  }
  // Exit application
  vtkInitializationHelper::Finalize();
  return ret_val;
}
}
