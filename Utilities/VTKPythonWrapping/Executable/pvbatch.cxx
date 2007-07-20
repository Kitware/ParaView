/*=========================================================================

Program:   ParaView
Module:    pvbatch.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h" // For VTK_USE_MPI
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVMain.h"
#include "vtkProcessModule.h"
#include "vtkPVPythonOptions.h"
#include "vtkPVProcessModulePythonHelper.h"

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

#include "vtkRenderingInstantiator.h"
#include "vtkVolumeRenderingInstantiator.h"
#include "vtkHybridInstantiator.h"
#include "vtkParallelInstantiator.h"

#include "vtkPVServerCommonInstantiator.h"
#include "vtkPVFiltersInstantiator.h"
#include "vtkPVServerManagerInstantiator.h"
#include "vtkClientServerInterpreter.h"
static void ParaViewInitializeInterpreter(vtkProcessModule* pm);

#include <vtksys/SystemTools.hxx>
#include <vtkstd/vector>
#include <vtkstd/string>

#define vtkPVStrDup(x) \
  strcpy(new char[ strlen(x) + 1], x)

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // MPI initialization changes CWD which messes with the executable path,
  // disrupting the paraview module location code. Hence, we change the
  // executable path to absolute path prior to MPI intialization.
  vtkstd::string fullExePath = vtksys::SystemTools::CollapseFullPath(argv[0]);
  vtkstd::vector<char*> vArg;
  vArg.push_back(vtkPVStrDup(fullExePath.c_str()));
  for (int cc=1; cc <argc;++cc)
    {
    vArg.push_back(vtkPVStrDup(argv[cc]));
    }

  vtkPVMain::SetInitializeMPI(1); // don't use MPI even when available.
  vtkPVMain::Initialize(&argc, &argv); 
  vtkPVMain* pvmain = vtkPVMain::New();
  vtkPVPythonOptions* options = vtkPVPythonOptions::New();
  options->SetProcessType(vtkPVOptions::PVBATCH);
  vtkPVProcessModulePythonHelper* helper = vtkPVProcessModulePythonHelper::New();
  helper->SetDisableConsole(true);
  int ret = pvmain->Initialize(options, helper, ParaViewInitializeInterpreter, argc,  &*vArg.begin());
  if (!ret)
    {
    // Tell process module that we support Multiple connections.
    // This must be set before starting the event loop.
    vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOff();
    helper->Run(options);
    }
  helper->Delete();
  pvmain->Delete();
  options->Delete();
  vtkPVMain::Finalize();

  vtkstd::vector<char*>::iterator it;
  for ( it = vArg.begin(); it != vArg.end(); ++ it )
    {
    delete [] *it;
    }

  return ret;
}

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGenericFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkInfovisCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkVolumeRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkWidgetsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVServerCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);

extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkCommonCS_Initialize(pm->GetInterpreter());
  vtkFilteringCS_Initialize(pm->GetInterpreter());
  vtkGenericFilteringCS_Initialize(pm->GetInterpreter());
  vtkImagingCS_Initialize(pm->GetInterpreter());
  vtkInfovisCS_Initialize(pm->GetInterpreter());
  vtkGraphicsCS_Initialize(pm->GetInterpreter());
  vtkIOCS_Initialize(pm->GetInterpreter());
  vtkRenderingCS_Initialize(pm->GetInterpreter());
  vtkVolumeRenderingCS_Initialize(pm->GetInterpreter());
  vtkHybridCS_Initialize(pm->GetInterpreter());
  vtkWidgetsCS_Initialize(pm->GetInterpreter());
  vtkParallelCS_Initialize(pm->GetInterpreter());
  vtkPVServerCommonCS_Initialize(pm->GetInterpreter());
  vtkPVFiltersCS_Initialize(pm->GetInterpreter());
  vtkXdmfCS_Initialize(pm->GetInterpreter());
}
