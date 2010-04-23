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
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkToolkits.h" // For 

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkProcessModule* vtkPVCreateProcessModule::CreateProcessModule(vtkPVOptions* op)
{
  vtkProcessModule *pm;
  pm = vtkProcessModule::New();
  pm->SetOptions(op);
  vtkProcessModule::SetProcessModule(pm);
  return pm;
}

//----------------------------------------------------------------------------
void vtkPVCreateProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
