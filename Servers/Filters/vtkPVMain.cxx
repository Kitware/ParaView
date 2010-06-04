/*=========================================================================

  Module:    vtkPVMain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h" // For VTK_USE_MPI 
#include "vtkPVConfig.h"
#include "vtkPVFiltersConfig.h"

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#ifdef PARAVIEW_BUILD_WITH_ADAPTOR
#include "vtkPVAdaptor.h"
#endif

#include "vtkFloatingPointExceptions.h"

#include "vtkPVMain.h"

#ifdef _WIN32
#include "vtkDynamicLoader.h"
#endif
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkTimerLog.h"

#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

#include <clocale>

vtkStandardNewMacro(vtkPVMain);

int vtkPVMain::UseMPI = 1;
int vtkPVMain::FinalizeMPI = 0;

//----------------------------------------------------------------------------
vtkPVMain::vtkPVMain()
{
  this->ProcessModule = 0;
}

//----------------------------------------------------------------------------
void vtkPVMain::SetUseMPI(int s)
{
  vtkPVMain::UseMPI = s;
}

//----------------------------------------------------------------------------
int vtkPVMain::GetUseMPI()
{
  return vtkPVMain::UseMPI;
}

//----------------------------------------------------------------------------
vtkPVMain::~vtkPVMain()
{
  // Clean up for exit.
  if(this->ProcessModule)
    {
    this->ProcessModule->Finalize();
    this->ProcessModule->Delete();
    this->ProcessModule = NULL;
    // free some memory
    }
  vtkTimerLog::CleanupLog();
}

void vtkPVMain::Initialize(int* argc, char** argv[])
{
  setlocale(LC_NUMERIC,"C");

#ifdef VTK_USE_MPI
  if(vtkPVMain::UseMPI)
    {
    int flag = 0;
    MPI_Initialized(&flag);
    if(flag == 0)
      {
      // MPICH changes the current working directory after MPI_Init. We fix that
      // by changing the CWD back to the original one after MPI_Init.
      vtkstd::string cwd = vtksys::SystemTools::GetCurrentWorkingDirectory(true);
      
      // This is here to avoid false leak messages from vtkDebugLeaks when
      // using mpich. It appears that the root process which spawns all the
      // main processes waits in MPI_Init() and calls exit() when
      // the others are done, causing apparent memory leaks for any objects
      // created before MPI_Init().
      MPI_Init(argc, argv);
      
      // restore CWD to what it was before the MPI intialization.
      vtksys::SystemTools::ChangeDirectory(cwd.c_str());

      vtkPVMain::FinalizeMPI = 1;
      }
    }
#else
  (void)argc;
  (void)argv;
#endif
#ifdef PARAVIEW_BUILD_WITH_ADAPTOR
  vtkPVAdaptorInitialize();
#endif

#ifdef VTK_USE_X
  // Hack to support -display parameter.  vtkPVOptions requires parameters to be
  // specified as -option=value, but it is generally expected that X window
  // programs allow you to set the display as -display host:port (i.e. without
  // the = between the option and value).  Unless someone wants to change
  // vtkPVOptions to work with or without the = (which may or may not be a good
  // idea), then this is the easiest way around the problem.
  for (int i = 1; i < *argc-1; i++)
    {
    if (strcmp((*argv)[i], "-display") == 0)
      {
      char *displayenv = (char *)malloc(strlen((*argv)[i+1]) + 10);
      sprintf(displayenv, "DISPLAY=%s", (*argv)[i+1]);
      putenv(displayenv);
      *argc -= 2;
      for (int j = i; j < *argc; j++)
        {
        (*argv)[j] = (*argv)[j+2];
        }
      (*argv)[*argc] = NULL;
      break;
      }
    }
#endif
  
  //sleep(15);
}

void vtkPVMain::Finalize()
{
#ifdef VTK_USE_MPI
  if (vtkPVMain::FinalizeMPI)
    {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    }
#endif
#ifdef PARAVIEW_BUILD_WITH_ADAPTOR
  vtkPVAdaptorDispose();
#endif
}

//----------------------------------------------------------------------------
int vtkPVMain::Initialize(vtkPVOptions* options,
                          vtkProcessModuleGUIHelper* helper,
                          INITIALIZE_INTERPRETER_FUNCTION initInterp, 
                          int argc, char* argv[])
{
  // Avoid Ghost windows on windows XP
#ifdef _WIN32
  typedef void (* VOID_FUN)();
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary("user32.dll");
  if(lib)
    {
    VOID_FUN func = (VOID_FUN)
      vtkDynamicLoader::GetSymbolAddress(lib, "DisableProcessWindowsGhosting");
    if(func)
      {
      (*func)();
      }
    }  
#endif
#ifdef PARAVIEW_ENABLE_FPE
  vtkFloatingPointExceptions::Enable();
#endif //PARAVIEW_ENABLE_FPE

  // Don't prompt the user with startup errors on unix.
#if defined(_WIN32) && !defined(__CYGWIN__)
  if(getenv("DASHBOARD_TEST_FROM_CTEST") ||
    getenv("DART_TEST_FROM_DART"))
    {
    vtkOutputWindow::GetInstance()->PromptUserOff();
    }
  else
    {
    vtkOutputWindow::GetInstance()->PromptUserOn();
    }
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif

  int display_help = 0;
  bool ret_failure = false;
  vtksys_ios::ostringstream sscerr;
  if (argv && !options->Parse(argc, argv) )
    {
    if ( options->GetUnknownArgument() )
      {
      sscerr << "Got unknown argument: " << options->GetUnknownArgument() << endl;
      }
    if ( options->GetErrorMessage() )
      {
      sscerr << "Error: " << options->GetErrorMessage() << endl;
      }
    display_help = 1;
    }
  if ( display_help || options->GetHelpSelected() )
    {
    sscerr << options->GetHelp() << endl;
    vtkOutputWindow::GetInstance()->DisplayText( sscerr.str().c_str() );
    ret_failure = true;
    }

  if (options->GetTellVersion() ) 
    {
    int MajorVersion = PARAVIEW_VERSION_MAJOR;
    int MinorVersion = PARAVIEW_VERSION_MINOR;
    char name[128];
    sprintf(name, "ParaView%d.%d\n", MajorVersion, MinorVersion);
    vtkOutputWindow::GetInstance()->DisplayText(name);
    ret_failure = true;
    }

  // Create the process module for initializing the processes.
  // Only the root server processes args.
  
  this->ProcessModule = vtkProcessModule::New();
  this->ProcessModule->SetOptions(options);
  vtkProcessModule::SetProcessModule(this->ProcessModule);
  // PM can use MPI only if MPI was initialized.
  this->ProcessModule->SetUseMPI(vtkPVMain::UseMPI);
  if(helper)
    {
    helper->SetProcessModule(this->ProcessModule);
    this->ProcessModule->SetGUIHelper(helper);
    }

  this->ProcessModule->Initialize();

  (*initInterp)(this->ProcessModule);

  return ret_failure? 1 : 0;
}

//-----------------------------------------------------------------------------
int vtkPVMain::Run(vtkPVOptions* options)
{
  if (!this->ProcessModule)
    {
    vtkErrorMacro("ProcessModule must be set before calling Run().");
    return 1;
    }

  // Start the application's event loop.  This will enable
  // vtkOutputWindow's user prompting for any further errors now that
  // startup is completed.
  int new_argc = 0;
  char** new_argv = 0;
  options->GetRemainingArguments(&new_argc, &new_argv);

  return this->ProcessModule->Start(new_argc, new_argv);
}

//-----------------------------------------------------------------------------
void vtkPVMain::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

