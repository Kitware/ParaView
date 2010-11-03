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
#include "vtkPVSession.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVSession::vtkPVSession()
{
}

//----------------------------------------------------------------------------
vtkPVSession::~vtkPVSession()
{
}

//----------------------------------------------------------------------------
vtkPVSession::ServerFlags vtkPVSession::GetProcessRoles()
{
  return CLIENT_AND_SERVERS;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVSession::GetController(vtkPVSession::ServerFlags)
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
