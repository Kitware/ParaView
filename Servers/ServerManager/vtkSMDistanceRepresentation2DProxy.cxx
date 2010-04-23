/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDistanceRepresentation2DProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDistanceRepresentation2DProxy.h"

#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMDistanceRepresentation2DProxy);
//----------------------------------------------------------------------------
vtkSMDistanceRepresentation2DProxy::vtkSMDistanceRepresentation2DProxy()
{
}

//----------------------------------------------------------------------------
vtkSMDistanceRepresentation2DProxy::~vtkSMDistanceRepresentation2DProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMDistanceRepresentation2DProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "InstantiateHandleRepresentation"
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->GetConnectionID(),
    this->GetServers(),
    stream);
}

//----------------------------------------------------------------------------
void vtkSMDistanceRepresentation2DProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


