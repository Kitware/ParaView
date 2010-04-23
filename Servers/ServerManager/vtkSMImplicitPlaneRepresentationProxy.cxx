/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImplicitPlaneRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImplicitPlaneRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMImplicitPlaneRepresentationProxy);

//---------------------------------------------------------------------------
vtkSMImplicitPlaneRepresentationProxy::vtkSMImplicitPlaneRepresentationProxy()
{
}

//---------------------------------------------------------------------------
vtkSMImplicitPlaneRepresentationProxy::~vtkSMImplicitPlaneRepresentationProxy()
{
}

//---------------------------------------------------------------------------
void vtkSMImplicitPlaneRepresentationProxy::CreateVTKObjects()
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  float opacity = 1.0;
  if (pm->GetNumberOfPartitions(this->ConnectionID) == 1)
    { 
    opacity = .25;
    }
  
  vtkClientServerID id = this->GetID();
    
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << id
         << "OutlineTranslationOff"
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream, 1);
  stream << vtkClientServerStream::Invoke << id
         << "GetPlaneProperty"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke 
         << vtkClientServerStream::LastResult 
         << "SetOpacity" 
         << opacity 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << id
         << "GetSelectedPlaneProperty" 
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke 
         << vtkClientServerStream::LastResult 
         << "SetOpacity" 
         << opacity 
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream, 1);
}

//---------------------------------------------------------------------------
void vtkSMImplicitPlaneRepresentationProxy::SendRepresentation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID id = this->GetID();

  vtkImplicitPlaneRepresentation* rep = 
    vtkImplicitPlaneRepresentation::SafeDownCast(pm->GetObjectFromID(id));

  int repState = rep->GetRepresentationState();
  // Don't bother to server if representation is the same.
  if (repState == this->RepresentationState)
    {
    return;
    }
  this->RepresentationState = repState;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << id
         << "SetRepresentationState"
         << repState
         << vtkClientServerStream::End;
  pm->SendStream(
    this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream, 1);
}

//---------------------------------------------------------------------------
void vtkSMImplicitPlaneRepresentationProxy::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}







