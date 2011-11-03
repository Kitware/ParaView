/*=========================================================================

  Program:   ParaView
  Module:    vtkSIUniformGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIUniformGridVolumeRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIUniformGridVolumeRepresentationProxy);
//----------------------------------------------------------------------------
vtkSIUniformGridVolumeRepresentationProxy::vtkSIUniformGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSIUniformGridVolumeRepresentationProxy::~vtkSIUniformGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSIUniformGridVolumeRepresentationProxy::CreateVTKObjects(
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
void vtkSIUniformGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
