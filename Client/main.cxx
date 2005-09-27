// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// Simple example demonstrating how to write applications using the
// ParaView server manager.  It creates a static pipeline and allows the
// user to interact with the scene using an interactor.

#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVMain.h"
#include "vtkProcessModule.h"
#include "pqOptions.h"
#include "pqProcessModuleGUIHelper.h"


// Include all the Instantiators:
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
static void InitializeInterpreter(vtkProcessModule* pm);


//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVMain::Initialize(&argc, &argv); // MPI must be initialized before any vtk object
  vtkPVMain* pvmain = vtkPVMain::New();
  pqOptions* options = pqOptions::New();
  options->SetProcessType(vtkPVOptions::PVBATCH);
  pqProcessModuleGUIHelper* helper = pqProcessModuleGUIHelper::New();
  int ret = pvmain->Run(options, helper, InitializeInterpreter, argc, argv);
  helper->Delete();
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
void InitializeInterpreter(vtkProcessModule* pm)
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
