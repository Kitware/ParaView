/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeKeeperProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMTimeKeeper.h"

vtkStandardNewMacro(vtkSMTimeKeeperProxy);
//----------------------------------------------------------------------------
vtkSMTimeKeeperProxy::vtkSMTimeKeeperProxy()
{
}

//----------------------------------------------------------------------------
vtkSMTimeKeeperProxy::~vtkSMTimeKeeperProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (this->ObjectsCreated)
    {
    vtkSMTimeKeeper* tk = vtkSMTimeKeeper::SafeDownCast(
      this->GetClientSideObject());
    if (tk)
      {
      tk->SetTimestepValuesProperty(this->GetProperty("TimestepValues"));
      tk->SetTimeRangeProperty(this->GetProperty("TimeRange"));
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
