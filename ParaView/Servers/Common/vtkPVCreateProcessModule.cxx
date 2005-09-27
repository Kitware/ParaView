/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCreateProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCreateProcessModule.h"

#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVMPIProcessModule.h"
#include "vtkToolkits.h" // For 

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPVCreateProcessModule, "1.1");

//----------------------------------------------------------------------------
vtkProcessModule* vtkPVCreateProcessModule::CreateProcessModule(vtkPVOptions* op)
{
  vtkProcessModule *pm;
  if (op->GetClientMode() || op->GetServerMode() ||
    op->GetRenderServerMode()) 
    {
    pm = vtkPVClientServerModule::New();
    }
  else
    {
#ifdef VTK_USE_MPI
    pm = vtkPVMPIProcessModule::New();
#else 
    pm = vtkPVProcessModule::New();
#endif
    }

  pm->SetOptions(op);
  vtkProcessModule::SetProcessModule(pm);
  return pm;
}
