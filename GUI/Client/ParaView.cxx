/*=========================================================================

Program:   ParaView
Module:    ParaView.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include "vtkKWRemoteExecute.h"
#include "vtkMultiProcessController.h"
#include "vtkOutputWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVConfig.h"
#include "vtkPVMPIProcessModule.h"

#include "vtkObject.h"
#include "vtkTclUtil.h"
#include "vtkTimerLog.h"

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
#include "vtkParaViewInstantiator.h"
#include "vtkClientServerInterpreter.h"
static void ParaViewInitializeInterpreter(vtkPVProcessModule* pm);

#ifdef PARAVIEW_ENABLE_FPE
#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif  //_GNU_SOURCE
#include <fenv.h>
#endif //__linux__
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
  // This only works on linux, see man fesetenv
  // It also need to be link to -lm
  fesetenv (FE_NOMASK_ENV);
#endif  //__linux__
}
#endif //PARAVIEW_ENABLE_FPE

//----------------------------------------------------------------------------
int MyMain(int argc, char *argv[])
{
  int retVal = 0;
  int startVal = 0;
  int myId = 0;
  vtkPVProcessModule *pm;
  vtkPVApplication *app;
  
#ifdef PARAVIEW_ENABLE_FPE
  u_fpu_setup();
#endif //PARAVIEW_ENABLE_FPE

#ifdef VTK_USE_MPI
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
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

  // The server is a special case.  We do not initialize Tk for process 0.
  // I would rather have application find this command line option, but
  // I cannot create an application before I initialize Tcl.
  // I could clean this up if I separate the initialization of Tk and Tcl.
  // I do not do this because it would affect other applications.
  int serverMode = 0;
  int renderServerMode = 0;
  int idx;
  for (idx = 0; idx < argc; ++idx)
    {
    if (strcmp(argv[idx],"--server") == 0 || strcmp(argv[idx],"-v") == 0)
      {
      serverMode = 1;
      }
    if (strcmp(argv[idx],"--render-server") == 0 || strcmp(argv[idx],"-rs") == 0)
      {
      renderServerMode = 1;
      }
    }

  // Initialize Tcl/Tk.
  Tcl_Interp *interp;
  if (renderServerMode || serverMode || myId > 0)
    { // DO not initialize Tk.
    vtkKWApplication::SetWidgetVisibility(0);
    }

  ostrstream err;
  interp = vtkPVApplication::InitializeTcl(argc, argv, &err);
  err << ends;
  if (!interp)
    {
#ifdef _WIN32
    ::MessageBox(0, err.str(), 
                 "ParaView error: InitializeTcl failed", MB_ICONERROR|MB_OK);
#else
    cerr << "ParaView error: InitializeTcl failed" << endl 
         << err.str() << endl;
#endif
    err.rdbuf()->freeze(0);
#ifdef VTK_USE_MPI
    MPI_Finalize();
#endif
    return 1;
    }
  err.rdbuf()->freeze(0);

  // Create the application to parse the command line arguments.
  app = vtkPVApplication::New();

  if (myId == 0)
    {
    if (app->ParseCommandLineArguments(argc, argv))
      {
      retVal = 1;
      app->SetStartGUI(0);
      }
    // Get the application settings from the registery
    // It has to be called now, after ParseCommandLineArguments, which can 
    // change the registery level (also, it can not be called in the application
    // constructor or even the KWApplication constructor since we need the
    // application name to be set)
    
    app->GetApplicationSettingsFromRegistery();
    
    // The client chooses a render module.
    if (app->GetRenderModuleName() == NULL)
      { // The render module has not been set by the user.  Choose a default.
      if (app->GetUseTiledDisplay())
        {
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
        app->SetRenderModuleName("IceTRenderModule");
#else
        app->SetRenderModuleName("MultiDisplayRenderModule");
#endif
        }
      else if (app->GetClientMode())
        { // Client server, no tiled display.
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
        app->SetRenderModuleName("DeskTopRenderModule");
#else
        app->SetRenderModuleName("MPIRenderModule");
#endif        
        }
      else
        { // Single process, or one MPI program
#ifdef VTK_USE_MPI
        app->SetRenderModuleName("MPIRenderModule");
#else
        app->SetRenderModuleName("LODRenderModule");
#endif
        }
      }
    }
  
  // Create the process module for initializing the processes.
  // Only the root server processes args.
  if (app->GetClientMode() || serverMode || renderServerMode) 
    {
    vtkPVClientServerModule *processModule = vtkPVClientServerModule::New();

    for (idx = 0; idx < argc; ++idx)
      {
      const char* arg = "--connect-id";
      if (strncmp(argv[idx], arg, strlen(arg)) == 0)
        {
        // Strip string to equals sign.
        const char* newarg=0;
        int len = (int)(strlen(argv[idx]));
        for (int i=0; i<len; i++)
          {
          if (argv[idx][i] == '=')
            {
            newarg = &(argv[idx][i+1]);
            }
          }
        if (newarg)
          {
          processModule->SetConnectID(atoi(newarg));
          break;
          }
        }
      }

    pm = processModule;
    }
  else
    {
#ifdef VTK_USE_MPI
    vtkPVMPIProcessModule *processModule = vtkPVMPIProcessModule::New();
#else 
    vtkPVProcessModule *processModule = vtkPVProcessModule::New();
#endif
    pm = processModule;
    }
  
  app->SetProcessModule(pm);
  pm->InitializeInterpreter();
  bool needLog = false;
  if(getenv("VTK_CLIENT_SERVER_LOG"))
    {
    needLog = true;
    if(app->GetClientMode())
      {
      needLog = false;
      pm->GetInterpreter()->SetLogFile("paraviewClient.log");
      }
    if(serverMode)
      {
      needLog = false;
      pm->GetInterpreter()->SetLogFile("paraviewServer.log");
      }
    if(renderServerMode)
      {
      needLog = false;
      pm->GetInterpreter()->SetLogFile("paraviewRenderServer.log");
      }
    } 
  if(needLog)
    {
    pm->GetInterpreter()->SetLogFile("paraview.log");
    }
  
  ParaViewInitializeInterpreter(pm);
  
  // Start the application's event loop.  This will enable
  // vtkOutputWindow's user prompting for any further errors now that
  // startup is completed.
  startVal = pm->Start(argc, argv);
  
  // Clean up for exit.
  pm->FinalizeInterpreter();
  pm->Delete();
  pm = NULL;

  // free some memory
  vtkTimerLog::CleanupLog();
  app->Delete();
  Tcl_DeleteInterp(interp);
  Tcl_Finalize();

#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif

  return (retVal?retVal:startVal);
}

#ifdef _WIN32
#include <windows.h>
#include "vtkDynamicLoader.h"

int __stdcall WinMain(HINSTANCE vtkNotUsed(hInstance), 
                      HINSTANCE vtkNotUsed(hPrevInstance),
                      LPSTR lpCmdLine, int vtkNotUsed(nShowCmd))
{
  int          argc;
  int          retVal;
  char**       argv;
  unsigned int i;
  int          j;
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
  
  // parse a few of the command line arguments
  // a space delimites an argument except when it is inside a quote
  argc = 1;
  int pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    }

  argv = (char**)malloc(sizeof(char*)* (argc+1));

  argv[0] = (char*)malloc(1024);
  ::GetModuleFileName(0, argv[0],1024);

  for(j=1; j<argc; j++)
    {
    argv[j] = (char*)malloc(strlen(lpCmdLine)+10);
    }
  argv[argc] = 0;

  argc = 1;
  pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    }
  argv[argc] = 0;

  // Initialize the processes and start the application.
  retVal = MyMain(argc, argv);

  // Delete arguments
  for(j=0; j<argc; j++)
    {
    free(argv[j]);
    }
  free(argv);

  return retVal;;
}
#else
int main(int argc, char *argv[])
{
  return MyMain(argc, argv);
}
#endif


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
  vtkKWParaViewCS_Initialize(pm->GetInterpreter());

#ifdef PARAVIEW_LINK_XDMF
  vtkXdmfCS_Initialize(pm->GetInterpreter());
#endif
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  vtkPVDevelopmentCS_Initialize(pm->GetInterpreter());
#endif
}
