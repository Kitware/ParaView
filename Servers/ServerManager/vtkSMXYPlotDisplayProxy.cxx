/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMXYPlotDisplayProxy.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMXYPlotDisplayProxy);
vtkCxxRevisionMacro(vtkSMXYPlotDisplayProxy, "1.1.2.2");
//-----------------------------------------------------------------------------
vtkSMXYPlotDisplayProxy::vtkSMXYPlotDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->XYPlotActorProxy = 0;
  this->PropertyProxy =0;
}

//-----------------------------------------------------------------------------
vtkSMXYPlotDisplayProxy::~vtkSMXYPlotDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->XYPlotActorProxy = 0;
  this->PropertyProxy =0;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->CollectProxy = this->GetSubProxy("CollectProxy");
  this->XYPlotActorProxy = this->GetSubProxy("XYPlotActor");
  this->PropertyProxy = this->GetSubProxy("Property");

  if (!this->UpdateSuppressorProxy || !this->CollectProxy || !this->XYPlotActor
    || !this->PropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined!");
    return;
    }

  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->XYPlotActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->PropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  
  this->Superclass::CreateVTKObjects(numObjects);

  this->SetupPipeline();
  this->SetupDefaults();
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::SetupPipeline()
{
  vtkSMInputProperty* ipp;

  ipp = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  if (!ipp)
    {
    vtkErrorMacro("Failed to find property Input on UpdateSuppressor.");
    return;
    }
  ipp->RemoveAllProxies();
  ipp->AddProxy(this->GeometryFilterProxy);

  svp  = vtkSMStringVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("OutputType"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property OutputType on UpdateSuppressorProxy.");
    return;
    }
  svp->SetElement(0,"vtkPolyData");
  this->UpdateSuppressorProxy->UpdateVTKObjects();

  // We hook up the XY plot's input here itself.
  ipp = vtkSMInputProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("Input"));
  if (!ipp)
    {
    vtkErrorMacro("Failed to find property Input on XYPlotActorProxy.");
    return;
    }
  ipp->RemoveAllProxies();
  ipp->AddProxy(this->UpdateSuppressorProxy);
  this->XYPlotActorProxy->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("Property"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Property on XYPlotActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->PropertyProxy);
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::SetupDefaults()
{
  vtkPVProcessModule* pm =
    vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  vtkClientServerStream stream;
  
  int i, num;
  num = this->CollectProxy->GetNumberOfIDs();
  // This stuff is quite similar to vtkSMCompositePartDisplay::SetupCollectionFilter.
  // If only I could avoid repetition.
  for (i=0; i < num; i++)
    {
    // We always duplicate beacuse all processes render the plot.
    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i)
      << "SetMoveModeToClone"
      << vtkClientServerStream::End;
    pm->SendStream( this->CollectProxy->GetServers(), stream);

    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i)
      << " SetMPIMToNSocketConnection"
      << pm->GetMPIMToNSocketConnectionID()
      << vtkClientServerStream::End;
    // create, SetPassThrough, and set the mToN connection
    // object on all servers and client
    pm->SendStream(
      vtkProcessModule::RENDER_SERVER | vtkProcessModule::DATA_SERVER, stream);

    // always set client mode.
    stream << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i)
      << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT, stream);

    // if running in client mode
    // then set the server to be servermode
    if(pm->GetClientMode())
      {
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }
    
    // if running in render server mode
    if (pm->GetOptions()->GetRenderServerMode())
      {
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
      }  

    if(pm->GetClientMode())
      {
      // We need this because the socket controller has no way of distinguishing
      // between processes.
      //law int fixme;  // This is called twice!  Fix it.
      stream << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetServerToClient"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      }

    // Handle collection setup with client server.
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream( 
      vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
    }

  // Not we set the properties for the XYPlotActor.
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("Position"));
  if (dvp)
    {
    dvp->SetElement(0, 0.05);
    dvp->SetElement(1, 0.05);
    }
  else
    {
    vtkErrorMacro("Failed to find property Position on XYPlotActorProxy.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("Position2"));
  if (dvp)
    {
    dvp->SetElement(0, 0.8);
    dvp->SetElement(1, 0.3);
    }
  else
    {
    vtkErrorMacro("Failed to find property Position2 on XYPlotActorProxy.");
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("NumberOfXLabels"));
  if (ivp)
    {
    ivp->SetElement(0, 5);
    }
  else
    {
    vtkErrorMacro("Failed to find property NumberOfXLabels.");
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("NumberOfYLabels"));
  if (ivp)
    {
    ivp->SetElement(0, 5);
    }
  else
    {
    vtkErrorMacro("Failed to find property NumberOfYLabels on XYPlotActorProxy.");
    }

  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("XTitle"));
  if (svp)
    {
    // This is stupid and has to change! (Line division label is meaningless.
    //int fixme;
    svp->SetElement(0, "Line Divisions");
    }
  else
    {
    vtkErrorMacro("Failed to find property XTitle.");
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("PlotPoints"));
  if (ivp)
    {
    ivp->SetElement(0, 1);
    }
  else
    {
    vtkErrorMacro("Failed to find property PlotPoints.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("LegendPosition"));
  if (dvp)
    {
    dvp->SetElement(0, 0.4);
    dvp->SetElement(0, 0.6);
    }
  else
    {
    vtkErrorMacro("Failed to find property LegendPosition.");
    }

   dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("LegendPosition2"));
  if (dvp)
    {
    dvp->SetElement(0, 0.5);
    dvp->SetElement(0, 0.25);
    }
  else
    {
    vtkErrorMacro("Failed to find property LegendPosition2.");
    }   

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Color"));
  if (dvp)
    {
    dvp->SetElement(0, 1);
    dvp->SetElement(1, 0.8);
    dvp->SetElement(2, 0.8);
    }
  else
    {
    vtkErrorMacro("Failed to find property Color on PropertyProxy.");
    }
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("PointSize"));
  if (dvp)
    {
    dvp->SetElement(0, 2);
    }
  else
    {
    vtkErrorMacro("Failed to find property PointSize on PropertyProxy.");
    }
 
  this->PropertyProxy->UpdateVTKObjects();
  this->XYPlotActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->AddProxy(this->XYPlotActorProxy);
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->RemoveProxy(this->XYPlotActorProxy); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UpdateSuppressorProxy: " << this->UpdateSuppressorProxy
    << endl;
  os << indent << "CollectProxy: " << this->CollectProxy
    << endl;
  os << indent << "XYPlotActorProxy: " << this->XYPlotActorProxy
    << endl;
}
