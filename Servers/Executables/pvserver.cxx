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

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include "vtkPVClientServerModule.h"
#include "vtkOutputWindow.h"
#include "vtkObject.h"
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

#include "vtkPVProgressHandler.h"
#include "vtkPVCommonInstantiator.h"
#include "vtkPVFiltersInstantiator.h"
#include "vtkSMInstantiator.h"
#include "vtkClientServerInterpreter.h"

#include "vtkPVProcessModule.h"
#include "vtkTimerLog.h"
#include "vtkDynamicLoader.h"
#include "kwsys/SystemTools.hxx"
#include "vtkPVConfig.h"
#include "vtkPVDemoPaths.h"

#include "vtkPVOptions.h"


int FindDemoPath(vtkPVClientServerModule*pm, char* argv0)
{
  kwsys_stl::string path;
  kwsys_stl::string error;
  if(!kwsys::SystemTools::FindProgramPath(argv0, path, error))
    {
    cerr << "Error, could not find path to own executable. " << error.c_str() << "\n";
    return 0;
    }
  path = kwsys::SystemTools::GetProgramPath(path.c_str());
  kwsys_stl::string demoPath = path;
  demoPath += "/Demos/Demo1.pvs";
  if(kwsys::SystemTools::FileExists(demoPath.c_str()))
    {
    demoPath = path;
    demoPath += "/Demos";
    pm->SetDemoPath(demoPath.c_str());
    return 1;
    }
  else
    { 
    const char* relPath = "../share/paraview-" PARAVIEW_VERSION "/Demos";
    demoPath = path;
    demoPath += "/";
    demoPath += relPath;
    demoPath += "/Demo1.pvs";
    if(kwsys::SystemTools::FileExists(demoPath.c_str()))
      {
      demoPath = path; 
      demoPath += "/";
      demoPath += relPath;
      pm->SetDemoPath(demoPath.c_str());
      return 1;
      }
    else
      {  
      const char** dir;
      for(dir=VTK_PV_DEMO_PATHS; *dir; ++dir)
        {
        demoPath = *dir;
        demoPath += "/Demo1.pvs";
        if(kwsys::SystemTools::FileExists(demoPath.c_str()))
          {
          pm->SetDemoPath(*dir);
          return 1;
          }
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
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

extern "C" void vtkKWParaViewCS_Initialize(vtkClientServerInterpreter*);

#ifdef PARAVIEW_LINK_XDMF
extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);
#endif
#ifdef PARAVIEW_BUILD_DEVELOPMENT
extern "C" void vtkPVDevelopmentCS_Initialize(vtkClientServerInterpreter *);
#endif

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkPVProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkCommonCS_Initialize(pm->GetInterpreter());
  vtkFilteringCS_Initialize(pm->GetInterpreter());
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

int main(int argc, char *argv[])
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
#ifdef PARAVIEW_ENABLE_FPE
  u_fpu_setup();
#endif //PARAVIEW_ENABLE_FPE
  
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
    return 1;
    }

#ifdef VTK_USE_MPI
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);
#endif

  // Don't prompt the user with startup errors on unix.
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkOutputWindow::GetInstance()->PromptUserOn();
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif

  vtkPVClientServerModule *pm = vtkPVClientServerModule::New();
  pm->SetOptions(options);
  FindDemoPath(pm, argv[0]);

  pm->InitializeInterpreter();
  pm->GetProgressHandler()->SetServerMode(1);
  vtkProcessModule::SetProcessModule(pm);
  bool needLog = false;
  if(getenv("VTK_CLIENT_SERVER_LOG"))
    {
    needLog = true;
    pm->GetInterpreter()->SetLogFile("paraviewServer.log");
    }
  
  ParaViewInitializeInterpreter(pm);
  
  // Start the application's event loop.  This will enable
  // vtkOutputWindow's user prompting for any further errors now that
  // startup is completed.
  retVal = pm->Start(argc, argv);
  
  // Clean up for exit.
  pm->FinalizeInterpreter();
  pm->Delete();
  pm = NULL;

  // free some memory
  vtkTimerLog::CleanupLog();
  options->Delete();

#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif

  return retVal;
}

