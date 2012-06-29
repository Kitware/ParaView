/*=========================================================================

  Program:   ParaView
  Module:    vtkSIAMRVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIAMRVolumeRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIAMRVolumeRepresentationProxy);
//----------------------------------------------------------------------------
vtkSIAMRVolumeRepresentationProxy::vtkSIAMRVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSIAMRVolumeRepresentationProxy::~vtkSIAMRVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSIAMRVolumeRepresentationProxy::CreateVTKObjects(
  vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }
  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSIAMRVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
