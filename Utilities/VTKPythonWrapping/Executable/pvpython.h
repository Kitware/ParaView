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
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVPythonOptions.h"
#include "vtkSMSession.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

#include "vtkPVPythonInterpretor.h"
#include <vtkstd/vector>

namespace ParaViewPython {

  //---------------------------------------------------------------------------

  char* clone(const char* str)
    {
    char *newStr = new char[ strlen(str) + 1 ];
    strcpy(newStr, str);
    return newStr;
    }

  //---------------------------------------------------------------------------

  void ProcessArgsForPython( vtkstd::vector<char*> & pythonArgs,
                             const char *script,
                             int argc, char* argv[] )
    {
    pythonArgs.clear();
    pythonArgs.push_back(clone(argv[0]));
    if(script)
      {
      pythonArgs.push_back(clone(script));
      }
    else if (argc > 1)
      {
      pythonArgs.push_back(clone("-"));
      }
    for (int cc=1; cc < argc; cc++)
      {
      pythonArgs.push_back(clone(argv[cc]));
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
    static vtkSmartPointer<vtkPVPythonOptions> options =
      vtkSmartPointer<vtkPVPythonOptions>::New();
    vtkInitializationHelper::Initialize( argc, argv, processType, options );
    if (options->GetTellVersion() || options->GetHelpSelected())
      {
      vtkInitializationHelper::Finalize();
      return 1;
      }

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

    int ret_val = 0;
    if (pm->GetSymmetricMPIMode() == false &&
      pm->GetPartitionId() > 0)
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
      vtkstd::vector<char*> pythonArgs;
      ProcessArgsForPython(pythonArgs, options->GetPythonScriptName(),
        remaining_argc, remaining_argv);

      // Start interpretor
      vtkPVPythonInterpretor* interpretor = vtkPVPythonInterpretor::New();
      ret_val = interpretor->PyMain(pythonArgs.size(), &*pythonArgs.begin());
      interpretor->Delete();

      // Free python args
      vtkstd::vector<char*>::iterator it = pythonArgs.begin();
      while(it != pythonArgs.end())
        {
        delete [] *it;
        ++it;
        }
      }
    // Exit application
    vtkInitializationHelper::Finalize();
    return ret_val;
    }
}
