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
#include "vtkToolkits.h" // For VTK_USE_MPI and VTK_USE_PATENTED
#include "vtkPVConfig.h"
#include "vtkPVMain.h"

#include "vtkMultiProcessController.h"
#include "vtkOutputWindow.h"


#include "vtkPVGUIClientOptions.h"
#include "vtkPVProcessModuleBatchHelper.h"
#include "vtkPVProcessModuleGUIHelper.h"
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
#include "vtkGenericFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#ifdef VTK_USE_RENDERING
#include "vtkRenderingInstantiator.h"
#endif

#ifdef VTK_USE_VOLUMERENDERING
#include "vtkVolumeRenderingInstantiator.h"
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

#include <kwsys/SystemTools.hxx>
        
static void ParaViewInitializeInterpreter(vtkProcessModule* pm);

//----------------------------------------------------------------------------
int MyMain(int argc, char *argv[])
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkOutputWindow::GetInstance()->PromptUserOn();
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif
  vtkPVMain::Initialize(&argc, &argv);
  vtkPVGUIClientOptions* options = vtkPVGUIClientOptions::New();
  options->SetProcessType(vtkPVOptions::PARAVIEW);
    // Create a pvmain
  vtkPVMain* pvmain = vtkPVMain::New();
  vtkProcessModuleGUIHelper* helper = vtkPVProcessModuleGUIHelper::New();;
  // run the paraview main
  int ret = pvmain->Run(options, helper, ParaViewInitializeInterpreter, argc, argv);
  helper->Delete();
  options->Delete();
  pvmain->Delete();
  vtkPVMain::Finalize();
  return ret;
}


//----------------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
#include "vtkDynamicLoader.h"

int __stdcall WinMain(HINSTANCE vtkNotUsed(hInstance), 
                      HINSTANCE vtkNotUsed(hPrevInstance),
                      LPSTR lpCmdLine, int vtkNotUsed(nShowCmd))
{
  int argc;
  char **argv;
  kwsys::SystemTools::ConvertWindowsCommandLineToUnixArguments(
    lpCmdLine, &argc, &argv);

  int retVal = MyMain(argc, argv);

  for (int i = 0; i < argc; i++) { delete [] argv[i]; }
  delete [] argv;

  return retVal;
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
extern "C" void vtkGenericFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkVolumeRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
#ifdef VTK_USE_PATENTED
extern "C" void vtkPatentedCS_Initialize(vtkClientServerInterpreter*);
#endif
extern "C" void vtkPVCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);

extern "C" void vtkKWParaViewCS_Initialize(vtkClientServerInterpreter*);

extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);
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
  vtkVolumeRenderingCS_Initialize(pm->GetInterpreter());
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
