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
vtkCxxRevisionMacro(vtkSMScatterPlotViewProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy::vtkSMScatterPlotViewProxy()
{
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
}

//----------------------------------------------------------------------------
bool vtkSMScatterPlotViewProxy::BeginCreateVTKObjects()
{
  bool res = this->Superclass::BeginCreateVTKObjects();

  if(!res)
    {
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
    this->GetProperty("CameraParallelProjection"));
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
  
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->Renderer2DProxy->GetID()
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

  return res;
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  this->AddPropToRenderer2D(this->CubeAxesActor);
  this->AddPropToRenderer2D(this->LegendScaleActor);
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
const char* vtkSMScatterPlotViewProxy::GetSuggestedViewType(vtkIdType connectionID)
{
  /*
  vtkSMViewProxy* rootView =
    vtkSMViewProxy::SafeDownCast(this->GetSubProxy("RenderView"));
  if (rootView)
    {
    vtksys_ios::ostringstream stream;
    stream << "2D" << rootView->GetSuggestedViewType(connectionID);
    this->SuggestedViewType = stream.str();
    return this->SuggestedViewType.c_str();
    }

  return this->Superclass::GetSuggestedViewType(connectionID);
  */
  return "ScatterPlotView";
}
