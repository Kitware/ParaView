/*=========================================================================

  Module:    vtkPVMain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h" // For VTK_USE_MPI and VTK_USE_PATENTED
#include "vtkPVConfig.h"
#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#ifdef PPARAVIEW_BUILD_WITH_ADAPTOR
#include "vtkPVAdaptor.h"
#endif


#include "vtkPVMain.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkProcessModuleGUIHelper.h"

#include "vtkMultiProcessController.h"
#include "vtkOutputWindow.h"
#include "vtkPVCreateProcessModule.h"
#include "vtkProcessModule.h"
#include "vtkTimerLog.h"
#include "vtkDynamicLoader.h"

vtkStandardNewMacro(vtkPVMain);
vtkCxxRevisionMacro(vtkPVMain, "1.3");



//----------------------------------------------------------------------------
vtkPVMain::vtkPVMain()
{
  this->ProcessModule = 0;
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

void vtkPVMain::Initialize(int argc, char* argv[])
{
#ifdef VTK_USE_MPI
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  int myId = 0;
  MPI_Init(&argc, &argv);
  // Might as well get our process ID here.  I use it to determine
  // Whether to initialize tk.  Once again, splitting Tk and Tcl 
  // initialization would clean things up.
  MPI_Comm_rank(MPI_COMM_WORLD,&myId); 
#else
  (void)argc;
  (void)argv;
#endif
}

void vtkPVMain::Finalize()
{
#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif
}


#ifdef PARAVIEW_ENABLE_FPE
void u_fpu_setup()
{
#ifdef _MSC_VER
  // enable floating point exceptions on MSVC
  short m = 0x372;
  __asm
    {
    fldcw m;
    }
#endif  //_MSC_VER
#ifdef __linux__
  // This only works on linux x86
  unsigned int fpucw= 0x1372;
  __asm__ ("fldcw %0" : : "m" (fpucw));
#endif  //__linux__
}
#endif //PARAVIEW_ENABLE_FPE

//----------------------------------------------------------------------------
int vtkPVMain::Run(vtkPVOptions* options,
                   vtkProcessModuleGUIHelper* helper,
                   INITIALIZE_INTERPRETER_FUNCTION initInterp, 
                   int argc, char* argv[])
{
  int retVal = 0;
  int startVal = 0;
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
  u_fpu_setup();
#endif //PARAVIEW_ENABLE_FPE

  // Don't prompt the user with startup errors on unix.
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkOutputWindow::GetInstance()->PromptUserOn();
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif

  int display_help = 0;
  if ( !options->Parse(argc, argv) )
    {
    cerr << "Problem parsing command line arguments" << endl;
    if ( options->GetUnknownArgument() )
      {
      cerr << "Got unknown argument: " << options->GetUnknownArgument() << endl;
      }
    if ( options->GetErrorMessage() )
      {
      cerr << "Error: " << options->GetErrorMessage() << endl;
      }
    display_help = 1;
    }
  if ( display_help || options->GetHelpSelected() )
    {
    cerr << options->GetHelp() << endl;
    return 1;
    }

  // Create the process module for initializing the processes.
  // Only the root server processes args.
  
  this->ProcessModule = vtkPVCreateProcessModule::CreateProcessModule(options);

  if(helper)
    {
    helper->SetProcessModule(this->ProcessModule);
    this->ProcessModule->SetGUIHelper(helper);
    }

  this->ProcessModule->Initialize();
#ifdef PPARAVIEW_BUILD_WITH_ADAPTOR
  vtkPVAdaptorCreatePrototypes();
#endif
  (*initInterp)(this->ProcessModule);

  // Start the application's event loop.  This will enable
  // vtkOutputWindow's user prompting for any further errors now that
  // startup is completed.
  int new_argc = 0;
  char** new_argv = 0;
  options->GetRemainingArguments(&new_argc, &new_argv);

  startVal = this->ProcessModule->Start(new_argc, new_argv);
  return (retVal?retVal:startVal);
}


void vtkPVMain::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

