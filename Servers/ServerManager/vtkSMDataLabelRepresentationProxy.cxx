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

//#include "vtkSMSimpleParallelStrategy.h"

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
  
  this->CellCenterFilter = 0;
  this->CellActorProxy = 0;
  this->CellMapperProxy = 0;
  this->CellTextPropertyProxy = 0;

  this->SetSelectionSupported(false);
}

//-----------------------------------------------------------------------------
vtkSMDataLabelRepresentationProxy::~vtkSMDataLabelRepresentationProxy()
{
  this->CollectProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;

  this->CellCenterFilter = 0;
  this->CellActorProxy = 0;
  this->CellMapperProxy = 0;
  this->CellTextPropertyProxy = 0;

}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::AddInput(unsigned int inputPort,
                                           vtkSMSourceProxy* input,
                                           unsigned int outputPort,
                                           const char* method)
{
  this->Superclass::AddInput(inputPort, input, outputPort, method);
  this->SetInputInternal(input, outputPort);
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetInputInternal(
  vtkSMSourceProxy* input, unsigned int outputPort)
{
  vtkPVDataInformation *di=input->GetDataInformation();
  if(!di->DataSetTypeIsA("vtkDataSet") || di->GetCompositeDataClassName())
    {
    return;
    }

  this->InvalidateGeometryInternal(0);
  this->CreateVTKObjects();

  this->Connect(input, this->CollectProxy, "Input", outputPort);
  this->SetupPipeline();  

  //vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
  //  this->CollectProxy->GetProperty("Input"));
  //if (!ip)
  //  {
  //  vtkErrorMacro("Failed to find property Input on UpdateSuppressorProxy.");
  //  return;
  //  }
  //ip->RemoveAllProxies();
  //ip->AddInputConnection(input, outputPort);
  //this->CollectProxy->UpdateProperty("Input");
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
 
  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->MapperProxy = this->GetSubProxy("Mapper");
  this->ActorProxy = this->GetSubProxy("Prop2D");
  this->TextPropertyProxy =  this->GetSubProxy("Property");

  if (!this->CollectProxy || !this->UpdateSuppressorProxy || !this->MapperProxy
    || !this->ActorProxy || !this->TextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return false;
    }

  this->CellCenterFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("CellCentersFilter"));
  this->CellMapperProxy = this->GetSubProxy("CellMapper");
  this->CellActorProxy = this->GetSubProxy("CellProp2D");
  this->CellTextPropertyProxy =  this->GetSubProxy("CellProperty");

  if (!this->CellCenterFilter || !this->CellMapperProxy
    || !this->CellActorProxy || !this->CellTextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return false;
    }

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
  //this->SetupPipeline();
  this->SetupDefaults();

  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetupPipeline()
{
  vtkSMInputProperty* ip;
  vtkSMProxyProperty* pp;

  vtkClientServerStream stream;

  vtkSMIntVectorProperty *otype = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("OutputDataType"));
  if (otype != NULL)
    {
    otype->SetElement(0,4); //vtkUnstructuredGrid
    }
  
  stream << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() << "GetOutputPort" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetInputConnection" << 0
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
    << "GetOutputPort" << 0
    << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID()
    << "SetInputConnection" << 0 
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

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

  ////// ----------------- Copied from parallel strategy END -------------------

  // Now we need to set up some default parameters on these filters.

  // Collect filter needs the socket controller use to communicate between
  // data-server root and the client.
  stream  << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() 
    << "GetSocketController"
    << pm->GetConnectionClientServerID(this->ConnectionID)
    << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID()
    << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT_AND_SERVERS, stream);

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

  // Set the MultiProcessController on the distributor.
  //stream  << vtkClientServerStream::Invoke
  //  << pm->GetProcessModuleID() 
  //  << "GetController"
  //  << vtkClientServerStream::End;
  //stream  << vtkClientServerStream::Invoke
  //  << this->CollectProxy->GetID()
  //  << "SetController"
  //  << vtkClientServerStream::LastResult
  //  << vtkClientServerStream::End;
  //pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

  ////// ----------------- Copied from parallel strategy END -------------------

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
    return;
    }
  ivp->SetElement(0, 2); // Clone mode.
  this->CollectProxy->UpdateVTKObjects();

 //  Tell the update suppressor to produce the correct partition.
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

  //stream << vtkClientServerStream::Invoke
  //  << this->CollectProxy->GetID() << "SetProcessModuleConnection"
  //  << pm->GetConnectionClientServerID(this->GetConnectionID())
  //  << vtkClientServerStream::End;
  //pm->SendStream(this->ConnectionID, 
  //  this->CollectProxy->GetServers(), stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->TextPropertyProxy->GetProperty("FontSize"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
    return;
    }
  ivp->SetElement(0, 18);
  this->TextPropertyProxy->UpdateVTKObjects();

  this->SetCellLabelVisibility(0);

  // Set default Cell Text property
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CellTextPropertyProxy->GetProperty("FontSize"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property FontSize on CellTextPropertyProxy.");
    return;
    }
  ivp->SetElement(0, 24);

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->CellTextPropertyProxy->GetProperty("Color"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Color on CellTextPropertyProxy.");
    return;
    }
  dvp->SetElements3(0.0, 1.0, 0.0);

  this->CellTextPropertyProxy->UpdateVTKObjects();

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

  // check if we should UseCache

  if (this->ViewInformation)
  {
  if (this->ViewInformation->Has(vtkSMViewProxy::USE_CACHE()))
    {
    if(this->ViewInformation->Get(vtkSMViewProxy::USE_CACHE())>0)
      {
      if (this->ViewInformation->Has(vtkSMViewProxy::CACHE_TIME()))
        {
        vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
          this->UpdateSuppressorProxy->GetProperty("CacheUpdate"));
        dvp->SetElement(0, this->ViewInformation->Get(vtkSMViewProxy::CACHE_TIME()));
        this->UpdateSuppressorProxy->UpdateProperty("CacheUpdate", 1);
        return;
        }
      }
    }
  }

  if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
    {
    return;
    }

  this->GeometryIsValid = 1;
  this->UpdateSuppressorProxy->InvokeCommand("ForceUpdate");
  this->Superclass::Update(view);
//  this->InvokeEvent(vtkSMViewProxy::ForceUpdateEvent);

  //vtkProcessModule* pm  = vtkProcessModule::GetProcessModule();
  //vtkAlgorithm* dp = vtkAlgorithm::SafeDownCast(
  //  pm->GetObjectFromID(this->CollectProxy->GetID()));

  //vtkTable* data = vtkTable::SafeDownCast(dp->GetOutputDataObject(0));
  //vtkstd::string text = "";
  //if (data->GetNumberOfRows() > 0 && data->GetNumberOfColumns() > 0)
  //{
  //  text = data->GetValue(0, 0).ToString();
  //}

  //// Now get the text from the Input and set it on the text widget display.
  //vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
  //  this->TextWidgetProxy->GetProperty("Text"));
  //svp->SetElement(0, text.c_str());
  //this->TextWidgetProxy->UpdateProperty("Text");
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetUpdateTime(double time)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created!");
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("UpdateTime"));
  dvp->SetElement(0, time);
  // UpdateTime is immediate update, so no need to update.

  // Go upstream to the reader and mark it modified.
  this->MarkUpstreamModified();
}

//----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this)
    {
    this->GeometryIsValid = 0;
    }

  this->Superclass::MarkModified(modifiedProxy);
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::InvalidateGeometryInternal(int useCache)
{
  if (!useCache)
    {
    this->GeometryIsValid = 0;
    if (this->UpdateSuppressorProxy)
      {
      vtkSMProperty *p = 
        this->UpdateSuppressorProxy->GetProperty("RemoveAllCaches");
      p->Modified();
      this->UpdateSuppressorProxy->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkSMDataLabelRepresentationProxy::GetCollectedData()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkMPIMoveData* dp = vtkMPIMoveData::SafeDownCast(
    pm->GetObjectFromID(this->CollectProxy->GetID()));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetUnstructuredGridOutput();
}

//----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetFontSizeCM(int size) 
{
  if (this->TextPropertyProxy)
    {
    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->TextPropertyProxy->GetProperty("PointFontSize"));
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
int vtkSMDataLabelRepresentationProxy::GetFontSizeCM() 
{
  if (this->TextPropertyProxy)
    {    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->TextPropertyProxy->GetProperty("PointFontSize"));
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
  vtkSMProxy* prop2D = this->GetSubProxy("Prop2D");
  if (prop2D)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop2D->GetProperty("Visibility"));
    if(ivp->GetElement(0))
      {
      return true;
      }
    }

  prop2D = this->GetSubProxy("CellProp2D");
  if (prop2D)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop2D->GetProperty("Visibility"));
    if(ivp->GetElement(0))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetPointLabelVisibility(int arg)
{
  vtkSMProxy* prop2D = this->GetSubProxy("Prop2D");

  if (prop2D)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop2D->GetProperty("Visibility"));
    ivp->SetElement(0, arg);
    prop2D->UpdateProperty("Visibility");
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::SetCellLabelVisibility(int arg)
{
  vtkSMProxy* prop2DCell = this->GetSubProxy("CellProp2D");

  if (prop2DCell)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop2DCell->GetProperty("Visibility"));
    ivp->SetElement(0, arg);
    prop2DCell->UpdateProperty("Visibility");
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataLabelRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GeometryIsValid: " << this->GeometryIsValid << endl;
}

