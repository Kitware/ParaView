/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientSideDataProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientSideDataProxy.h"

#include "vtkClientServerStream.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMClientSideDataProxy);
vtkCxxRevisionMacro(vtkSMClientSideDataProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMClientSideDataProxy::vtkSMClientSideDataProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->GeometryIsValid = 0;
  this->PolyOrUGrid = 0;
}

//-----------------------------------------------------------------------------
vtkSMClientSideDataProxy::~vtkSMClientSideDataProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMClientSideDataProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->CollectProxy = this->GetSubProxy("Collect");

  if (!this->UpdateSuppressorProxy || !this->CollectProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined!");
    return;
    }

  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------
  
void vtkSMClientSideDataProxy::AddInput(vtkSMSourceProxy* input, const char*, 
                                       int )
{
  this->InvalidateGeometry();
  this->CreateVTKObjects(1);

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on CollectProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(input);
  this->CollectProxy->UpdateVTKObjects();

  vtkPVDataInformation *DataInformation = input->GetDataInformation();
  if (DataInformation->DataSetTypeIsA("vtkUnstructuredGrid"))
    {
    this->PolyOrUGrid = 1;
    }

  this->SetupPipeline();
  this->SetupDefaults();
}

//-----------------------------------------------------------------------------
void vtkSMClientSideDataProxy::SetupPipeline()
{
  vtkSMStringVectorProperty* svp;

  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke;
    if (this->PolyOrUGrid)
      stream << this->CollectProxy->GetID(i) << "GetUnstructuredGridOutput";   
    else
      stream << this->CollectProxy->GetID(i) << "GetPolyDataOutput";
    stream << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << this->UpdateSuppressorProxy->GetID(i) << "SetInput"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID,
      this->UpdateSuppressorProxy->GetServers(), stream);
    }

  svp  = vtkSMStringVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("OutputType"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property OutputType on UpdateSuppressorProxy.");
    return;
    }
  if (this->PolyOrUGrid)
    svp->SetElement(0,"vtkUnstructuredGrid");
  else
    svp->SetElement(0,"vtkPolyData");
  this->UpdateSuppressorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMClientSideDataProxy::SetupDefaults()
{
  vtkProcessModule* pm =vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  
  int i, num;
  num = this->CollectProxy->GetNumberOfIDs();
  // We always duplicate beacuse all processes render the plot.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  ivp->SetElement(0, 2); // Clone mode.
  this->CollectProxy->UpdateVTKObjects();

  // This stuff is quite similar to vtkSMCompositePartDisplay::SetupCollectionFilter.
  // If only I could avoid repetition.
  for (i=0; i < num; i++)
    {

    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i)
      << "SetMPIMToNSocketConnection"
      << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
      << vtkClientServerStream::End;
    // create, SetPassThrough, and set the mToN connection
    // object on all servers and client
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER | vtkProcessModule::DATA_SERVER, stream);

    // always set client mode.
    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i)
      << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);

    // if running in client mode
    // then set the server to be servermode
    if(pm->GetClientMode())
      {
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::DATA_SERVER, stream);
      }

    // if running in render server mode
    if (pm->GetOptions()->GetRenderServerMode())
      {
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::RENDER_SERVER, stream);
      }  

    if(pm->GetClientMode())
      {
      // We need this because the socket controller has no way of distinguishing
      // between processes.
      //law int fixme;  // This is called twice!  Fix it.
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToClient"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::CLIENT, stream);
      }

    // Handle collection setup with client server.
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << pm->GetConnectionClientServerID(this->ConnectionID)
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientSideDataProxy::Update()
{
  if(this->GeometryIsValid || !this->UpdateSuppressorProxy)
    {
    return;
    }
  vtkSMProperty* p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
  p->Modified();
  this->GeometryIsValid = true;
  this->UpdateVTKObjects();
  this->UpdateSuppressorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkSMClientSideDataProxy::GetCollectedData()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  
  vtkMPIMoveData* dp = vtkMPIMoveData::SafeDownCast(
      pm->GetObjectFromID(this->CollectProxy->GetID(0)));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutput();
}

//-----------------------------------------------------------------------------
void vtkSMClientSideDataProxy::InvalidateGeometry()
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
void vtkSMClientSideDataProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);
  this->InvalidateGeometry();
}

//-----------------------------------------------------------------------------
void vtkSMClientSideDataProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UpdateSuppressorProxy: " << this->UpdateSuppressorProxy
    << endl;
  os << indent << "CollectProxy: " << this->CollectProxy
    << endl;
}

