/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractTriggerProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractTriggerProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMExtractTriggerProxy);
//----------------------------------------------------------------------------
vtkSMExtractTriggerProxy::vtkSMExtractTriggerProxy()
{
}

//----------------------------------------------------------------------------
vtkSMExtractTriggerProxy::~vtkSMExtractTriggerProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMExtractTriggerProxy::IsActivated(vtkSMExtractsController* controller)
{
  const int timestep = controller->GetTimeStep();

  int start_timestep = 0;
  if (vtkSMPropertyHelper(this, "UseStartTimeStep").GetAsInt() == 1)
  {
    start_timestep = vtkSMPropertyHelper(this, "StartTimeStep").GetAsInt();
    if (start_timestep > timestep)
    {
      return false;
    }
  }

  if (vtkSMPropertyHelper(this, "UseEndTimeStep").GetAsInt() == 1 &&
    timestep > vtkSMPropertyHelper(this, "EndTimeStep").GetAsInt())
  {
    return false;
  }

  const int frequency = vtkSMPropertyHelper(this, "Frequency").GetAsInt();
  if ((timestep - start_timestep) % frequency != 0)
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMExtractTriggerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
