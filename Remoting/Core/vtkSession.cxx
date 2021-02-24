/*=========================================================================

  Program:   ParaView
  Module:    vtkSession.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSession.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

//----------------------------------------------------------------------------
vtkSession::vtkSession() = default;

//----------------------------------------------------------------------------
vtkSession::~vtkSession() = default;

//----------------------------------------------------------------------------
void vtkSession::Activate()
{
  if (vtkProcessModule* pm = vtkProcessModule::GetProcessModule())
  {
    pm->PushActiveSession(this);
  }
}

//----------------------------------------------------------------------------
void vtkSession::DeActivate()
{
  if (vtkProcessModule* pm = vtkProcessModule::GetProcessModule())
  {
    pm->PopActiveSession(this);
  }
}

//----------------------------------------------------------------------------
void vtkSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
