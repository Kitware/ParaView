/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCaveRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCaveRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"

vtkStandardNewMacro(vtkSMCaveRenderViewProxy);
vtkCxxRevisionMacro(vtkSMCaveRenderViewProxy, "1.1");

//-----------------------------------------------------------------------------
vtkSMCaveRenderViewProxy::vtkSMCaveRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMCaveRenderViewProxy::~vtkSMCaveRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::ConfigureRenderManagerFromServerInformation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(
    this->ConnectionID);
    
  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("NumberOfDisplays"));
  if (ivp)
    {
    ivp->SetElements1(numMachines);
    }
  // the property must be applied to the vtkobject before 
  // the property Displays is set.
  this->ParallelRenderManager->UpdateProperty("NumberOfDisplays");

  double* displays = new double[ numMachines*10];
  for (idx = 0; idx < numMachines; idx++)
    {
    displays[idx*10+0] = idx;
    displays[idx*10+1] = serverInfo->GetLowerLeft(idx)[0];
    displays[idx*10+2] = serverInfo->GetLowerLeft(idx)[1];
    displays[idx*10+3] = serverInfo->GetLowerLeft(idx)[2];
    displays[idx*10+4] = serverInfo->GetLowerRight(idx)[0];
    displays[idx*10+5] = serverInfo->GetLowerRight(idx)[1];
    displays[idx*10+6] = serverInfo->GetLowerRight(idx)[2];
    displays[idx*10+7] = serverInfo->GetUpperLeft(idx)[0];
    displays[idx*10+8] = serverInfo->GetUpperLeft(idx)[1];
    displays[idx*10+9] = serverInfo->GetUpperLeft(idx)[2];
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("Displays"));
  if (dvp)
    {
    dvp->SetElements(displays, numMachines*10);
    }
  this->ParallelRenderManager->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::EndCreateVTKObjects()
{     
  this->Superclass::EndCreateVTKObjects();
  this->ConfigureRenderManagerFromServerInformation();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

