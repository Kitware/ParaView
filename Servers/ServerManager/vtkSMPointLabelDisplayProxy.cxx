/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPointLabelDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPointLabelDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVOptions.h"
#include "vtkSMRenderModuleProxy.h"

vtkStandardNewMacro(vtkSMPointLabelDisplayProxy);
vtkCxxRevisionMacro(vtkSMPointLabelDisplayProxy, "Revision: 1.1$");

//-----------------------------------------------------------------------------
vtkSMPointLabelDisplayProxy::vtkSMPointLabelDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->GeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
vtkSMPointLabelDisplayProxy::~vtkSMPointLabelDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::AddInput(vtkSMSourceProxy* input,
  const char*, int , int)
{
  this->SetInput(input);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetInput(vtkSMSourceProxy* input)
{
  this->InvalidateGeometry();
  this->CreateVTKObjects(1);

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on UpdateSuppressorProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(input);

  this->SetupPipeline();
  this->SetupDefaults();
}


//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  if (numObjects != 1)
    {
    vtkErrorMacro("Can handle on 1 input!");
    numObjects = 1;
    }
  
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->MapperProxy = this->GetSubProxy("Mapper");
  this->ActorProxy = this->GetSubProxy("Actor");
  this->TextPropertyProxy =  this->GetSubProxy("Property");

  if (!this->UpdateSuppressorProxy || !this->MapperProxy
    || !this->ActorProxy || !this->TextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return;
    }
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->MapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->TextPropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetupPipeline()
{
  vtkSMInputProperty* ip;
  vtkSMStringVectorProperty* svp;
  vtkSMProxyProperty* pp;

  vtkClientServerStream stream;

  svp =  vtkSMStringVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("OutputType"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property OutputType on "
      "UpdateSuppressorProxy.");
    return;
    }
// I don't set the output type...so the output will me same as the input.
//  svp->SetElement(0, "vtkUnstructuredGrid");
//  svp->SetElement(0, "vtkPolyData");

  ip = vtkSMInputProperty::SafeDownCast(
    this->MapperProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on MapperProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->UpdateSuppressorProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->MapperProxy->GetProperty("LabelTextProperty"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property LabelTextProperty.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->TextPropertyProxy);
  this->MapperProxy->UpdateVTKObjects();

  pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on ActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->MapperProxy);

  this->ActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetupDefaults()
{
  vtkPVProcessModule* pm =
    vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  vtkClientServerStream stream;
  vtkSMIntVectorProperty* ivp;

  unsigned int i;
  for (i=0; i < this->UpdateSuppressorProxy->GetNumberOfIDs(); i++)
    {
    // Tell the update suppressor to produce the correct partition.
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(this->UpdateSuppressorProxy->GetServers(), stream);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->TextPropertyProxy->GetProperty("FontSize"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
    return;
    }
  ivp->SetElement(0, 24);
  this->TextPropertyProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));

  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->AddProxy(this->ActorProxy);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::RemoveFromRenderModule(
  vtkSMRenderModuleProxy* rm)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->RemoveProxy(this->ActorProxy);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::Update()
{
  if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
    {
    return;
    }
  vtkSMProperty* p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
  p->Modified();
  this->UpdateSuppressorProxy->UpdateVTKObjects(); 
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::MarkConsumersAsModified()
{
  this->Superclass::MarkConsumersAsModified();
  this->InvalidateGeometry();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GeometryIsValid: " << this->GeometryIsValid << endl;
}
