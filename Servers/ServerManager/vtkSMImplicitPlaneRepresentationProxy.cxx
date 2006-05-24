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
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMImplicitPlaneRepresentationProxy);
vtkCxxRevisionMacro(vtkSMImplicitPlaneRepresentationProxy, "1.1");

//---------------------------------------------------------------------------
vtkSMImplicitPlaneRepresentationProxy::vtkSMImplicitPlaneRepresentationProxy()
{
}

//---------------------------------------------------------------------------
vtkSMImplicitPlaneRepresentationProxy::~vtkSMImplicitPlaneRepresentationProxy()
{
}

//---------------------------------------------------------------------------
void vtkSMImplicitPlaneRepresentationProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  float opacity = 1.0;
  if (pm->GetNumberOfPartitions(this->ConnectionID) == 1)
    { 
    opacity = .25;
    }
  
  vtkClientServerStream stream;
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    
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
}

//---------------------------------------------------------------------------
void vtkSMImplicitPlaneRepresentationProxy::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}







