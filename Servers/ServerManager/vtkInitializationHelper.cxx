/*=========================================================================

Program:   ParaView
Module:    vtkInitializationHelper.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkPVConfig.h"
/*
 * Make sure all the kits register their classes with vtkInstantiator.
 * Since ParaView uses Tcl wrapping, all of VTK is already compiled in
 * anyway.  The instantiators will add no more code for the linker to
 * collect.
 */

#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h" // MANDATORY gives rutime problems otherwise
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

#include "vtkDummyProcessModuleHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVMain.h"
#include "vtkPVOptions.h"
#include "vtkSMApplication.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"

#include <vtkstd/string>


static void vtkInitializationHelperInit(vtkProcessModule* pm);

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.
extern "C" void vtkParaviewMinInit_Initialize(vtkClientServerInterpreter*);
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

vtkProcessModuleGUIHelper* vtkInitializationHelper::Helper = 0;
vtkPVMain* vtkInitializationHelper::PVMain = 0;
vtkPVOptions* vtkInitializationHelper::Options = 0;
vtkSMApplication* vtkInitializationHelper::Application = 0;

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(const char* executable)
{
  vtkInitializationHelper::Initialize(executable, NULL);
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(const char* executable,
  vtkPVOptions* options)
{
  if (!executable)
    {
    vtkGenericWarningMacro("Executable name has to be defined.");
    return;
    }

  // Pass the program name to make option parser happier
  char* argv = new char[strlen(executable)+1];
  strcpy(argv, executable);

  vtkSmartPointer<vtkPVOptions> newoptions = options;
  if (!options)
    {
    newoptions = vtkSmartPointer<vtkPVOptions>::New();
    }
  vtkInitializationHelper::Initialize(1, &argv, newoptions);
  delete[] argv;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(int argc, char**argv, vtkPVOptions* options)
{
  if (vtkInitializationHelper::PVMain)
    {
    vtkGenericWarningMacro("Python module already initialize. Skipping.");
    return;
    }

  if (!options)
    {
    vtkGenericWarningMacro("vtkPVOptions must be specified.");
    return;
    }

  // don't change process type if the caller has already initialized it.
  if (options->GetProcessType() == vtkPVOptions::ALLPROCESS)
    {
    options->SetProcessType(vtkPVOptions::PVCLIENT);
    }

  if (options->GetProcessType() == vtkPVOptions::PVCLIENT)
    {
    // in client mode, we don't provide access to MPI.
    vtkPVMain::SetUseMPI(0); // don't use MPI even when available.
    }
  vtkInitializationHelper::PVMain = vtkPVMain::New();
  vtkInitializationHelper::Options = options;
  vtkInitializationHelper::Options->Register(0); // keep reference.


  // This process module helper does nothing. ProcessModuleHelpers are to be
  // deprecated, then don't serve much anymore.
  vtkInitializationHelper::Helper = vtkDummyProcessModuleHelper::New();

  // First initialization
  PVMain->Initialize(
    vtkInitializationHelper::Options, 
    vtkInitializationHelper::Helper,
    vtkInitializationHelperInit, 
    argc, argv);

  vtkInitializationHelper::Application = vtkSMApplication::New();
  vtkInitializationHelper::Application->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOn();
  // Initialize everything else
  vtkInitializationHelper::PVMain->Run(Options);
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Finalize()
{
  vtkSMObject::SetProxyManager(0);
  if (PVMain)
    {
    PVMain->Delete();
    PVMain = 0;
    }
  if (Application)
    {
    Application->Delete();
    Application = 0;
    }
  if (Helper)
    {
    Helper->Delete();
    Helper = 0;
    }
  if (Options)
    {
    Options->Delete();
    Options = 0;
    }
  vtkProcessModule::SetProcessModule(0);
}

//-----------------------------------------------------------------------------
/*
 * PARAVIEW_MINIMIUM if enabled, initializes only required set of classes.
 * The mechanism is specified in the Servers/ServerManager/CMakeList.txt
 * Otherwise the entire vtk+paraview class list will be included.
 *
 * @param pm IN used to pass the interpreter for every *_Initialize function.
 */
void vtkInitializationHelperInit(vtkProcessModule* pm)
{

#ifdef PARAVIEW_MINIMAL_BUILD
  vtkParaviewMinInit_Initialize(pm->GetInterpreter());
#else
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
#endif
}

//----------------------------------------------------------------------------
/*
 * The primary interface used to Initialize all the kits.
 *
 * @param pm IN process module is passed to vtkInitilizationHelperInit function
 */
void vtkInitializationHelper::InitializeInterpretor(vtkProcessModule* pm)
{
  ::vtkInitializationHelperInit(pm);
}
