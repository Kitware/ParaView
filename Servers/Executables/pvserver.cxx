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
#include "vtkPVMain.h" // For VTK_USE_MPI
#include "vtkToolkits.h" // For VTK_USE_MPI
#include "vtkPVServerOptions.h"
#include "vtkPVConfig.h" // Required to get build options for paraview

// Include all the Instantiators:
#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#ifdef VTK_USE_RENDERING
#include "vtkRenderingInstantiator.h"
#endif

#ifdef VTK_USE_VOLUMERENDERING
#include "vtkVolumeRenderingInstantiator.h"
#endif

#ifdef VTK_USE_HYBRID
#include "vtkHybridInstantiator.h"
#endif

#ifdef VTK_USE_PARALLEL
#include "vtkParallelInstantiator.h"
#endif

#include "vtkPVServerCommonInstantiator.h"
#include "vtkPVFiltersInstantiator.h"
#include "vtkSMInstantiator.h"
#include "vtkClientServerInterpreter.h"

#include "vtkPVProcessModule.h"

// forward declare the initialize function
static void ParaViewInitializeInterpreter(vtkProcessModule* pm);

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVMain::Initialize(&argc, &argv);
  // First create the correct options for this process
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  // set the type of process
  options->SetProcessType(vtkPVOptions::PVSERVER);
  // Create a pvmain
  vtkPVMain* pvmain = vtkPVMain::New();
  // run the paraview main
  int ret = pvmain->Initialize(options, 0, ParaViewInitializeInterpreter, 
    argc, argv);
  if (ret)
    {
    ret = pvmain->Run(options);
    }
  // clean up and return
  pvmain->Delete();
  options->Delete();
  vtkPVMain::Finalize();
  return ret;
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
