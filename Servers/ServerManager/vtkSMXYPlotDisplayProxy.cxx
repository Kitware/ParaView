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
#include "vtkSMProxyProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkCommand.h"
#include "vtkXYPlotWidget.h"
#include "vtkXYPlotActor.h"
#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkCoordinate.h"
#include "vtkSMSourceProxy.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkMPIMoveData.h"
#include "vtkPolyData.h"
#include "vtkSMIntVectorProperty.h"

class vtkSMXYPlotDisplayProxyObserver : public vtkCommand
{
public:
  static vtkSMXYPlotDisplayProxyObserver* New() 
    { return new vtkSMXYPlotDisplayProxyObserver; }
  virtual void Execute (vtkObject* obj, unsigned long event, void* calldata)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(obj, event, calldata);
      }
    }
  void SetTarget(vtkSMXYPlotDisplayProxy* t)
    {
    this->Target = t;
    }
protected:
  vtkSMXYPlotDisplayProxy* Target;
  vtkSMXYPlotDisplayProxyObserver() { this->Target = 0; }
  ~vtkSMXYPlotDisplayProxyObserver() { this->SetTarget(0); }
};


vtkStandardNewMacro(vtkSMXYPlotDisplayProxy);
vtkCxxRevisionMacro(vtkSMXYPlotDisplayProxy, "1.1.2.4");
//-----------------------------------------------------------------------------
vtkSMXYPlotDisplayProxy::vtkSMXYPlotDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->XYPlotActorProxy = 0;
  this->PropertyProxy =0;
  this->Observer = vtkSMXYPlotDisplayProxyObserver::New();
  this->Observer->SetTarget(this);
  this->XYPlotWidget = vtkXYPlotWidget::New();
  this->RenderModuleProxy = 0;
  this->Visibility = 0;
  this->GeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
vtkSMXYPlotDisplayProxy::~vtkSMXYPlotDisplayProxy()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->XYPlotWidget->Delete();
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->XYPlotWidget->SetXYPlotActor(0);
  this->XYPlotActorProxy = 0;
  this->PropertyProxy =0;
  this->RenderModuleProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->CollectProxy = this->GetSubProxy("Collect");
  this->XYPlotActorProxy = this->GetSubProxy("XYPlotActor");
  this->PropertyProxy = this->GetSubProxy("Property");

  if (!this->UpdateSuppressorProxy || !this->CollectProxy || !this->XYPlotActorProxy
    || !this->PropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined!");
    return;
    }

  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->XYPlotActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->PropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  
  this->Superclass::CreateVTKObjects(numObjects);

}

//-----------------------------------------------------------------------------
  
void vtkSMXYPlotDisplayProxy::AddInput(vtkSMSourceProxy* input, const char*, 
  int , int )
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

  this->SetupPipeline();
  this->SetupDefaults();
  this->SetupWidget();
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::SetupWidget()
{
  if (!this->XYPlotActorProxy || this->XYPlotActorProxy->GetNumberOfIDs() < 1)
    {
    vtkErrorMacro("XYPlotActorProxy not defined!");
    return;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkXYPlotActor* actor = vtkXYPlotActor::SafeDownCast(
    pm->GetObjectFromID(this->XYPlotActorProxy->GetID(0)));

  this->XYPlotWidget->SetXYPlotActor(actor);
  this->XYPlotWidget->AddObserver(vtkCommand::InteractionEvent,
    this->Observer);
  this->XYPlotWidget->AddObserver(vtkCommand::StartInteractionEvent,
    this->Observer);
  this->XYPlotWidget->AddObserver(vtkCommand::EndInteractionEvent,
    this->Observer);
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::SetupPipeline()
{
  vtkSMInputProperty* ipp;
  vtkSMStringVectorProperty* svp;

  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    if (this->CollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "GetPolyDataOutput"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "SetInput"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(
      this->UpdateSuppressorProxy->GetServers(), stream);
    }

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
      vtkProcessModule::CLIENT_AND_SERVERS, stream);

    }

  // Not we set the properties for the XYPlotActor.
  vtkSMDoubleVectorProperty* dvp;
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

  this->RenderModuleProxy = rm;
  this->SetVisibility(this->Visibility);
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
  if (this->XYPlotWidget->GetEnabled())
    {
    this->XYPlotWidget->SetEnabled(0);
    }
  this->XYPlotWidget->SetInteractor(0);
  this->XYPlotWidget->SetCurrentRenderer(0);
  this->RenderModuleProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::Update()
{
  if (this->GeometryIsValid || !this->UpdateSuppressorProxy || 
    !this->RenderModuleProxy)
    {
    return;
    }
  vtkSMProperty* p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
  p->Modified();
  this->UpdateSuppressorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::SetVisibility(int visible)
{
  this->Visibility = visible;
  if (!this->RenderModuleProxy)
    {
    return;
    }

  // Set widget interactor.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(
    pm->GetObjectFromID(this->RenderModuleProxy->GetInteractorProxy()->GetID(0)));
  if (!iren)
    {
    vtkErrorMacro("Failed to get client side Interactor.");
    return;
    }
  this->XYPlotWidget->SetInteractor(iren);
  
  vtkRenderer* ren = vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(this->RenderModuleProxy->GetRenderer2DProxy()->GetID(0)));
  if (!ren)
    {
    vtkErrorMacro("Failed to get client side 2D renderer.");
    return;
    }
  this->XYPlotWidget->SetCurrentRenderer(ren);
  this->XYPlotWidget->SetEnabled(visible);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on XYPlotActorProxy.");
    return;
    }
  ivp->SetElement(0, visible);
  this->XYPlotActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::ExecuteEvent(vtkObject*, unsigned long event, 
  void*)
{
  vtkPVGenericRenderWindowInteractor* iren;
  switch (event)
    {
  case vtkCommand::StartInteractionEvent:
    //TODO: enable Interactive rendering.
    iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->XYPlotWidget->GetInteractor());
    iren->InteractiveRenderEnabledOn();
    break;
    
  case vtkCommand::EndInteractionEvent:
    //TODO: disable interactive rendering.
    iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->XYPlotWidget->GetInteractor());
    iren->InteractiveRenderEnabledOff();
    break;

  case vtkCommand::InteractionEvent:
    // Take the client position values and push on to the server.
    vtkXYPlotActor* actor = this->XYPlotWidget->GetXYPlotActor();
    double *pos1 = actor->GetPositionCoordinate()->GetValue();
    double *pos2 = actor->GetPosition2Coordinate()->GetValue();
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->XYPlotActorProxy->GetProperty("Position"));
    if (dvp)
      {
      dvp->SetElement(0, pos1[0]);
      dvp->SetElement(1, pos1[1]);
      }
    else
      {
      vtkErrorMacro("Failed to find property Position on XYPlotActorProxy.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->XYPlotActorProxy->GetProperty("Position2"));
    if (dvp)
      {
      dvp->SetElement(0, pos2[0]);
      dvp->SetElement(1, pos2[1]);
      }
    else
      {
      vtkErrorMacro("Failed to find property Position2 on XYPlotActorProxy.");
      }
    this->XYPlotActorProxy->UpdateVTKObjects();
    break;
    }
  this->InvokeEvent(event); // just in case the GUI wants to know about interaction.
}
//-----------------------------------------------------------------------------
vtkPolyData* vtkSMXYPlotDisplayProxy::GetCollectedData()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  
  vtkMPIMoveData* dp = vtkMPIMoveData::SafeDownCast(
      pm->GetObjectFromID(this->CollectProxy->GetID(0)));
  if (dp == NULL)
    {
    return NULL;
    }

  return vtkPolyData::SafeDownCast(dp->GetOutput());
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotDisplayProxy::MarkConsumersAsModified()
{
  this->Superclass::MarkConsumersAsModified();
  this->InvalidateGeometry();
}

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
