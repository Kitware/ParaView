/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataLabelRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataLabelRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMDataLabelRepresentationProxy);
vtkCxxRevisionMacro(vtkSMDataLabelRepresentationProxy, "Revision: 1.1$");

//-----------------------------------------------------------------------------
vtkSMDataLabelRepresentationProxy::vtkSMDataLabelRepresentationProxy()
{
  this->CollectProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->GeometryIsValid = 0;
  this->AppendProxy = 0;
  
  this->CellCenterFilter = 0;
  this->CellActorProxy = 0;
  this->CellMapperProxy = 0;
  this->CellTextPropertyProxy = 0;
}

//-----------------------------------------------------------------------------
vtkSMDataLabelRepresentationProxy::~vtkSMDataLabelRepresentationProxy()
{
  this->CollectProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->AppendProxy = 0;
  this->TextPropertyProxy = 0;

  this->CellCenterFilter = 0;
  this->CellActorProxy = 0;
  this->CellMapperProxy = 0;
  this->CellTextPropertyProxy = 0;

}

//----------------------------------------------------------------------------
bool vtkSMDataLabelRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  // TODO: Eventually we may want to put the label actors in Render3D
  //renderView->AddPropToRenderer(this->ActorProxy);
  //renderView->AddPropToRenderer(this->CellActorProxy);

  renderView->AddPropToRenderer2D(this->ActorProxy);
  renderView->AddPropToRenderer2D(this->CellActorProxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMDataLabelRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  //renderView->RemovePropFromRenderer(this->ActorProxy);
  //renderView->RemovePropFromRenderer(this->CellActorProxy);
  renderView->RemovePropFromRenderer2D(this->ActorProxy);
  renderView->RemovePropFromRenderer2D(this->CellActorProxy);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMDataLabelRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }
 
  this->AppendProxy = this->GetSubProxy("Append");
  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->MapperProxy = this->GetSubProxy("PointLabelMapper");
  this->ActorProxy = this->GetSubProxy("PointLabelProp2D");
  this->TextPropertyProxy =  this->GetSubProxy("PointLabelProperty");

  if (!this->AppendProxy || !this->CollectProxy || !this->UpdateSuppressorProxy ||
    !this->MapperProxy || !this->ActorProxy || !this->TextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return false;
    }

  this->CellCenterFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("CellCentersFilter"));
  this->CellMapperProxy = this->GetSubProxy("CellLabelMapper");
  this->CellActorProxy = this->GetSubProxy("CellLabelProp2D");
  this->CellTextPropertyProxy =  this->GetSubProxy("CellLabelProperty");

  if (!this->CellCenterFilter || !this->CellMapperProxy
    || !this->CellActorProxy || !this->CellTextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return false;
    }

  this->AppendProxy->SetServers(vtkProcessModule::DATA_SERVER);
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->MapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->TextPropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->CellCenterFilter->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->CellMapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->CellActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->CellTextPropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMDataLabelRepresentationProxy::EndCreateVTKObjects()
{
  // Init UpdateSuppressor so that my pipeline knows what portion to do.
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetUpdateNumberOfPieces"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetPartitionId"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetUpdatePiece"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    this->UpdateSuppressorProxy->GetServers(), stream);

  // There used to be a check to ensure that the data type of input is vtkDataSet.
  // I've taken that out since it would cause excution of the extract selection
  // filter. We can put that back if needed.
  this->Connect(this->GetInputProxy(), this->AppendProxy, 
    "Input", this->OutputPort);

  this->Connect(this->AppendProxy, this->CollectProxy);
  this->SetupPipeline();
  this->SetupDefaults();

  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetupPipeline()
{
  vtkSMProxyProperty* pp;

  vtkClientServerStream stream;

  vtkSMIntVectorProperty *otype = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("OutputDataType"));
  if (otype != NULL)
    {
    otype->SetElement(0, VTK_UNSTRUCTURED_GRID); 
    }
  
  stream << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() << "GetOutputPort" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetInputConnection" 
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);

  // Now send to the render server.
  // This order of sending first to CLIENT|DATA_SERVER and then to render server
  // ensures that the connections are set up correctly even when data server and
  // render server are the same.
  stream  << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID() 
    << "GetOutputPort" 
    << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID()
    << "SetInputConnection"  
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

  this->Connect(this->UpdateSuppressorProxy, this->MapperProxy);

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

  // Set up Cell Centers pipeline
  this->Connect(this->UpdateSuppressorProxy, this->CellCenterFilter);
  this->Connect(this->CellCenterFilter, this->CellMapperProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->CellMapperProxy->GetProperty("LabelTextProperty"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property LabelTextProperty on CellMapperProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->CellTextPropertyProxy);
  this->CellMapperProxy->UpdateVTKObjects();

  pp = vtkSMProxyProperty::SafeDownCast(
    this->CellActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on CellActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->CellMapperProxy);

  this->CellActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetupDefaults()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkSMIntVectorProperty* ivp;

  // Collect filter needs the MPIMToNSocketConnection to communicate between
  // render server and data server nodes.
  stream  << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID()
    << "SetMPIMToNSocketConnection"
    << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

  // Set the server flag on the collect filter to correctly identify each
  // processes.
  stream  << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID()
    << "SetServerToRenderServer"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID()
    << "SetServerToDataServer"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::DATA_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID()
    << "SetServerToClient"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
    return;
    }
  ivp->SetElement(0, 2); // Clone mode.
  this->CollectProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::Update(
  vtkSMViewProxy* view)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  if (!this->GetInputProxy())
    {
    vtkErrorMacro("Input is not set yet!");
    return;
    }

  // check if we should UseCache
  if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
    {
    return;
    }

  this->GeometryIsValid = 1;
  this->UpdateSuppressorProxy->InvokeCommand("ForceUpdate");
  this->Superclass::Update(view);

  // CellCenterFilter is connected after the update suppressor, so it is
  // essential that we call UpdatePipeline() on it which ensures that the
  // PostUpdateData() is called on the filters before the mapper.
  this->CellCenterFilter->UpdatePipeline();

  // This updates the domains for the choosing the correct array name on the
  // mapper.
  this->MapperProxy->GetProperty("Input")->UpdateDependentDomains();
  this->CellMapperProxy->GetProperty("Input")->UpdateDependentDomains();
//  this->InvokeEvent(vtkSMViewProxy::ForceUpdateEvent);
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetUpdateTimeInternal(double time)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("UpdateTime"));
  dvp->SetElement(0, time);
  this->UpdateSuppressorProxy->UpdateProperty("UpdateTime");

  // Go upstream to the reader and mark it modified.
  this->MarkUpstreamModified();
}

//----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this)
    {
    this->GeometryIsValid = 0;
    }

  this->Superclass::MarkDirty(modifiedProxy);
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::InvalidateGeometryInternal(int useCache)
{
  if (!useCache)
    {
    this->GeometryIsValid = 0;
    //if (this->UpdateSuppressorProxy)
    //  {
    //  vtkSMProperty *p = 
    //    this->UpdateSuppressorProxy->GetProperty("RemoveAllCaches");
    //  p->Modified();
    //  this->UpdateSuppressorProxy->UpdateVTKObjects();
    //  }
    }
}

//----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetPointFontSizeCM(int size) 
{
  if (this->TextPropertyProxy)
    {
    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->TextPropertyProxy->GetProperty("FontSize"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
      return;
      }
    ivp->SetElement(0, size);
    this->TextPropertyProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMDataLabelRepresentationProxy::GetPointFontSizeCM() 
{
  if (this->TextPropertyProxy)
    {    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->TextPropertyProxy->GetProperty("FontSize"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
      return 0;
      }
    return ivp->GetElement(0);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetCellFontSizeCM(int size) 
{
  if (this->CellTextPropertyProxy)
  {

    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->CellTextPropertyProxy->GetProperty("FontSize"));
    if (!ivp)
    {
      vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
      return;
    }
    ivp->SetElement(0, size);
    this->CellTextPropertyProxy->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
int vtkSMDataLabelRepresentationProxy::GetCellFontSizeCM() 
{
  if (this->CellTextPropertyProxy)
  {    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->CellTextPropertyProxy->GetProperty("FontSize"));
    if (!ivp)
    {
      vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
      return 0;
    }
    return ivp->GetElement(0);
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool vtkSMDataLabelRepresentationProxy::GetVisibility()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("PointLabelVisibility"));
  if(ivp->GetElement(0))
    {
    return true;
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("CellLabelVisibility"));
  if(ivp->GetElement(0))
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GeometryIsValid: " << this->GeometryIsValid << endl;
}

