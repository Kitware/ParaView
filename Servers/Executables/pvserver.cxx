/*=========================================================================

Program:   ParaView
Module:    pvserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h" // For VTK_USE_MPI and VTK_USE_PATENTED
#include "vtkPVConfig.h"

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include "vtkMultiProcessController.h"
#include "vtkOutputWindow.h"

#include "vtkTimerLog.h"

#ifdef _WIN32
#  include "vtkDynamicLoader.h"
#endif

#include "vtkPVOptions.h"
#include "vtkPVCreateProcessModule.h"
#include "vtkProcessModule.h"

/*
 * Make sure all the kits register their classes with vtkInstantiator.
 * Since ParaView uses Tcl wrapping, all of VTK is already compiled in
 * anyway.  The instantiators will add no more code for the linker to
 * collect.
 */
#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#ifdef VTK_USE_RENDERING
#include "vtkRenderingInstantiator.h"
#endif

#ifdef VTK_USE_PATENTED
#include "vtkPatentedInstantiator.h"
#endif

#ifdef VTK_USE_HYBRID
#include "vtkHybridInstantiator.h"
#endif

#ifdef VTK_USE_PARALLEL
#include "vtkParallelInstantiator.h"
#endif

#include "vtkPVCommonInstantiator.h"
#include "vtkPVFiltersInstantiator.h"
#include "vtkSMInstantiator.h"
#include "vtkClientServerInterpreter.h"
static void ParaViewInitializeInterpreter(vtkProcessModule* pm);

#ifdef PPARAVIEW_BUILD_WITH_ADAPTOR
#include "vtkPVAdaptor.h"
#endif

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
int main(int argc, char* argv[])
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
  int retVal = 0;
  int startVal = 0;
  
#ifdef PARAVIEW_ENABLE_FPE
  u_fpu_setup();
#endif //PARAVIEW_ENABLE_FPE

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
#endif

  // Don't prompt the user with startup errors on unix.
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkOutputWindow::GetInstance()->PromptUserOn();
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif

  int display_help = 0;
  vtkPVOptions* options = vtkPVOptions::New();
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
    options->Delete();
#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif
    return 1;
    }

  // Create the process module for initializing the processes.
  // Only the root server processes args.
  
  vtkProcessModule* pm = vtkPVCreateProcessModule::CreateProcessModule(options);
  
  pm->Initialize();
  
#ifdef PPARAVIEW_BUILD_WITH_ADAPTOR
  vtkPVAdaptorCreatePrototypes();
#endif
  
  ParaViewInitializeInterpreter(pm);

  // Start the application's event loop.  This will enable
  // vtkOutputWindow's user prompting for any further errors now that
  // startup is completed.
  int new_argc = 0;
  char** new_argv = 0;
  options->GetRemainingArguments(&new_argc, &new_argv);

  startVal = pm->Start(new_argc, new_argv);

  // Clean up for exit.
  pm->Finalize();
  pm->Delete();
  pm = NULL;

  // free some memory
  vtkTimerLog::CleanupLog();

#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif
  options->Delete();
  
#ifdef PPARAVIEW_BUILD_WITH_ADAPTOR
  vtkPVAdaptorDispose();
#endif
  
  return (retVal?retVal:startVal);
}

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGenericFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
#ifdef VTK_USE_PATENTED
extern "C" void vtkPatentedCS_Initialize(vtkClientServerInterpreter*);
#endif
extern "C" void vtkPVCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);

#ifdef PARAVIEW_LINK_XDMF
extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);
#endif
#ifdef PARAVIEW_BUILD_DEVELOPMENT
extern "C" void vtkPVDevelopmentCS_Initialize(vtkClientServerInterpreter *);
#endif

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkCommonCS_Initialize(pm->GetInterpreter());
  vtkFilteringCS_Initialize(pm->GetInterpreter());
  vtkGenericFilteringCS_Initialize(pm->GetInterpreter());
  vtkImagingCS_Initialize(pm->GetInterpreter());
  vtkGraphicsCS_Initialize(pm->GetInterpreter());
  vtkIOCS_Initialize(pm->GetInterpreter());
  vtkRenderingCS_Initialize(pm->GetInterpreter());
  vtkHybridCS_Initialize(pm->GetInterpreter());
  vtkParallelCS_Initialize(pm->GetInterpreter());
#ifdef VTK_USE_PATENTED
  vtkPatentedCS_Initialize(pm->GetInterpreter());
#endif
  vtkPVCommonCS_Initialize(pm->GetInterpreter());
  vtkPVFiltersCS_Initialize(pm->GetInterpreter());

#ifdef PARAVIEW_LINK_XDMF
  vtkXdmfCS_Initialize(pm->GetInterpreter());
#endif
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  vtkPVDevelopmentCS_Initialize(pm->GetInterpreter());
#endif
}
