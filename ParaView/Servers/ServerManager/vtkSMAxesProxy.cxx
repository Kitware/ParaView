/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAxesProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAxesProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMInputProperty.h"

vtkStandardNewMacro(vtkSMAxesProxy);
vtkCxxRevisionMacro(vtkSMAxesProxy, "1.6");
//---------------------------------------------------------------------------
vtkSMAxesProxy::vtkSMAxesProxy()
{
}

//---------------------------------------------------------------------------
vtkSMAxesProxy::~vtkSMAxesProxy()
{
}


//---------------------------------------------------------------------------
void vtkSMAxesProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Superclass::CreateVTKObjects(numObjects);
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  int cc;

  vtkClientServerStream str;
  for (cc=0; cc< numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    str << vtkClientServerStream::Invoke << id 
      << "SymmetricOn" << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke << id
      << "ComputeNormalsOff" << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, this->Servers,str,0);
    }

  // Setup the pipeline.
  vtkSMProxy* mapper = this->GetSubProxy("Mapper");
  vtkSMProxy* actor = this->GetSubProxy("Prop");

  if (!mapper)
    {
    vtkErrorMacro("Subproxy Mapper must be defined.");
    return;
    }
  
  if (!actor)
    {
    vtkErrorMacro("Subproxy Actor must be defined.");
    return;
    }
 
  for (cc=0; cc< numObjects; cc++)
    {
    str << vtkClientServerStream::Invoke << this->GetID(cc) 
      << "GetOutput" << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke << mapper->GetID(cc)
      << "SetInput" 
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, this->Servers,str,0);
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    actor->GetProperty("Mapper"));
  pp->RemoveAllProxies();
  pp->AddProxy(mapper);

  this->UpdateVTKObjects();

}

//---------------------------------------------------------------------------
void vtkSMAxesProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
