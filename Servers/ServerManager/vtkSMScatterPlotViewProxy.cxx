/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScatterPlotViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScatterPlotViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include <vtksys/ios/sstream>
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkSMScatterPlotViewProxy);
vtkCxxRevisionMacro(vtkSMScatterPlotViewProxy, "1.4");
//----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy::vtkSMScatterPlotViewProxy()
{
  this->RenderView = 0;
  this->LegendScaleActor = 0;
  this->CubeAxesActor = 0;
}

//----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy::~vtkSMScatterPlotViewProxy()
{
  /*
  if(!this->CubeAxesActor)
    {
    return;
    }
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera" << 0
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  */
  if (this->RenderView && this->LegendScaleActor)
    {
    this->RenderView->RemovePropFromRenderer2D(this->LegendScaleActor);
    }
  if (this->RenderView && this->CubeAxesActor)
    {
    this->RenderView->RemovePropFromRenderer(this->CubeAxesActor);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderView: " << this->RenderView << endl;
  os << indent << "LegendScaleActor: " << this->LegendScaleActor << endl;
  os << indent << "CubeAxesActor: " << this->CubeAxesActor << endl;
}


//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::AddRepresentation(vtkSMRepresentationProxy* repr)
{
  if (this->RenderView)
    {
    this->RenderView->AddRepresentation(repr);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* repr)
{
  if (this->RenderView)
    {
    this->RenderView->RemoveRepresentation(repr);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::RemoveAllRepresentations()
{
  if (this->RenderView)
    {
    this->RenderView->RemoveAllRepresentations();
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  if (this->RenderView)
    {
    this->RenderView->SetViewUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::SetCacheTime(double time)
{
  this->Superclass::SetCacheTime(time);
  if (this->RenderView)
    {
    this->RenderView->SetCacheTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::SetUseCache(int use)
{
  this->Superclass::SetUseCache(use);
  if (this->RenderView)
    {
    this->RenderView->SetUseCache(use);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::SetViewPosition(int x, int y)
{
  this->Superclass::SetViewPosition(x, y);
  if (this->RenderView)
    {
    this->RenderView->SetViewPosition(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::SetGUISize(int x, int y)
{
  this->Superclass::SetGUISize(x, y);
  if (this->RenderView)
    {
    this->RenderView->SetGUISize(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::StillRender()
{
  if (this->RenderView)
    {
    this->RenderView->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::InteractiveRender()
{
  if (this->RenderView)
    {
    this->RenderView->InteractiveRender();
    }
}


//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMScatterPlotViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    sproxy->UpdatePipeline(this->GetViewUpdateTime());
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
                                                 "ScatterPlotRepresentation");
                                                 //                                           "CubeAxesRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
        pxm->NewProxy("representations", "ScatterPlotRepresentation"));
    //pxm->NewProxy("representations", "CubeAxesRepresentation"));
    /*
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      repr->GetProperty("Representation"));
    ivp->SetElement(0, 0);
    */
    return repr;
    }

  // Currently only images can be shown 
  vtkErrorMacro("This view only supports surface representation.");
  return 0;
}

//----------------------------------------------------------------------------
bool vtkSMScatterPlotViewProxy::BeginCreateVTKObjects()
{
  /*
  bool res = this->Superclass::BeginCreateVTKObjects();

  if(!res)
    {
    return false;
    }
  */
  this->RenderView = vtkSMRenderViewProxy::SafeDownCast(
    this->GetSubProxy("RenderView"));
  if (!this->RenderView)
    {
    vtkErrorMacro("Missing \"RenderView\" subproxy.");
    return false;
    }
  
  this->CubeAxesActor = this->GetSubProxy("CubeAxesActor");
  if (!this->CubeAxesActor)
    {
    vtkErrorMacro("Missing \"CubeAxesActor\" subproxy.");
    return false;
    }

  this->CubeAxesActor->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->LegendScaleActor = this->GetSubProxy("LegendScaleActor");
  if (!this->LegendScaleActor)
    {
    vtkErrorMacro("Missing \"LegendScaleActor\" subproxy.");
    return false;
    }

  this->LegendScaleActor->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderView->GetProperty("CameraParallelProjection"));
  ivp->SetElement(0, 1);
  
  vtkSMIntVectorProperty* lvp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("LegendScaleAxesVisibility"));
  lvp->SetElement(0, 1);

  vtkSMIntVectorProperty* rvp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("RightAxisVisibility"));
  rvp->SetElement(0, 0);

  vtkSMIntVectorProperty* tvp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("TopAxisVisibility"));
  tvp->SetElement(0, 0);
  
  bool res =  this->Superclass::BeginCreateVTKObjects();

  return res;
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  this->RenderView->AddPropToRenderer(this->CubeAxesActor);
  this->RenderView->AddPropToRenderer2D(this->LegendScaleActor);


  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->RenderView->GetRendererProxy()->GetID()
          << "GetActiveCamera"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);

}


//----------------------------------------------------------------------------
const char* vtkSMScatterPlotViewProxy::GetSuggestedViewType(vtkIdType connectionID)
{
  vtkSMViewProxy* rootView =
    vtkSMViewProxy::SafeDownCast(this->GetSubProxy("RenderView"));
  if (rootView)
    {
    vtksys_ios::ostringstream stream;
    stream << "ScatterPlot" << rootView->GetSuggestedViewType(connectionID);
    this->SuggestedViewType = stream.str();
    return this->SuggestedViewType.c_str();
    }

  return this->Superclass::GetSuggestedViewType(connectionID);
  //return "ScatterPlotView";
}
