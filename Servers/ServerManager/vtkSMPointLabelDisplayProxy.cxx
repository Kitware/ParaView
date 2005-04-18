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
#include "vtkUnstructuredGrid.h"
#include "vtkMPIMoveData.h"
#include "vtkPVDataInformation.h"
vtkStandardNewMacro(vtkSMPointLabelDisplayProxy);
vtkCxxRevisionMacro(vtkSMPointLabelDisplayProxy, "Revision: 1.1$");

//-----------------------------------------------------------------------------
vtkSMPointLabelDisplayProxy::vtkSMPointLabelDisplayProxy()
{
  this->CollectProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->GeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
vtkSMPointLabelDisplayProxy::~vtkSMPointLabelDisplayProxy()
{
  this->CollectProxy = 0;
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
  vtkPVDataInformation *di=input->GetDataInformation();
  if(!di->DataSetTypeIsA("vtkDataSet") || di->GetBaseDataClassName())
    {
    return;
    }

  this->InvalidateGeometry();
  this->CreateVTKObjects(1);

  this->SetupPipeline(); // Have to this earlier
  this->SetupDefaults(); 
  
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on UpdateSuppressorProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(input);


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
 
  this->CollectProxy = this->GetSubProxy("Collect");
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->MapperProxy = this->GetSubProxy("Mapper");
  this->ActorProxy = this->GetSubProxy("Actor");
  this->TextPropertyProxy =  this->GetSubProxy("Property");

  if (!this->CollectProxy || !this->UpdateSuppressorProxy || !this->MapperProxy
    || !this->ActorProxy || !this->TextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return;
    }
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
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
  
  for (unsigned int i=0; i < this->UpdateSuppressorProxy->GetNumberOfIDs();i++)
    {
    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "GetUnstructuredGridOutput"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(
      this->UpdateSuppressorProxy->GetServers(), stream);
    }
  
  svp =  vtkSMStringVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("OutputType"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property OutputType on "
      "UpdateSuppressorProxy.");
    return;
    }
  svp->SetElement(0, "vtkUnstructuredGrid");

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
  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    // A rather complex mess to set the correct server variable 
    // on all of the remote duplication filters.
    if(pm->GetClientMode())
      {
      // We need this because the socket controller has no way of distinguishing
      // between processes.
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToClient"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      }
    // pm->ClientMode is only set when there is a server.
    if(pm->GetClientMode())
      {
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }
    // if running in render server mode
    if(pm->GetOptions()->GetRenderServerMode())
      {
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
      }  

    // Handle collection setup with client server.
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) 
      << "SetClientDataServerSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);

    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID()
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

    }
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
    return;
    }
  ivp->SetElement(0, 2); // Clone mode.
  this->CollectProxy->UpdateVTKObjects();

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
  /*
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));

  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->AddProxy(this->ActorProxy);
  */
  rm->AddPropToRenderer2D(this->ActorProxy);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::RemoveFromRenderModule(
  vtkSMRenderModuleProxy* rm)
{
  /*
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->RemoveProxy(this->ActorProxy);
  */
  rm->RemovePropFromRenderer2D(this->ActorProxy);
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
  if (this->UpdateSuppressorProxy)
    {
    vtkSMProperty *p = this->UpdateSuppressorProxy->GetProperty("RemoveAllCaches");
    p->Modified();
    this->UpdateSuppressorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::MarkConsumersAsModified()
{
  this->Superclass::MarkConsumersAsModified();
  this->InvalidateGeometry();
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkSMPointLabelDisplayProxy::GetCollectedData()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkMPIMoveData* dp = vtkMPIMoveData::SafeDownCast(
    pm->GetObjectFromID(this->CollectProxy->GetID(0)));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetUnstructuredGridOutput();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GeometryIsValid: " << this->GeometryIsValid << endl;
}
