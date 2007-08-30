/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiProcessRenderView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiProcessRenderView.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationStrategy.h"

vtkCxxRevisionMacro(vtkSMMultiProcessRenderView, "1.4");
//----------------------------------------------------------------------------
vtkSMMultiProcessRenderView::vtkSMMultiProcessRenderView()
{
  this->RemoteRenderThreshold = 20.0;
  this->LastCompositingDecision = false;
  this->RemoteRenderAvailable = false;
}

//----------------------------------------------------------------------------
vtkSMMultiProcessRenderView::~vtkSMMultiProcessRenderView()
{
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMMultiProcessRenderView::NewStrategyInternal(
  int dataType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;

  if (dataType == VTK_POLY_DATA)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "PolyDataParallelStrategy"));
    }
  else if (dataType == VTK_UNIFORM_GRID)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "UniformGridParallelStrategy"));
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "UnstructuredGridParallelStrategy"));
    }
  else
    {
    vtkWarningMacro("This view does not provide a suitable strategy for "
      << dataType);
    }

  return strategy;
}

//----------------------------------------------------------------------------
bool vtkSMMultiProcessRenderView::GetCompositingDecision(
  unsigned long totalMemory, int vtkNotUsed(stillRender))
{
  if (!this->RemoteRenderAvailable)
    {
    // Cannot remote render due to setup issues.
    return false;
    }

  if (static_cast<float>(totalMemory)/1000.0 < this->RemoteRenderThreshold)
    {
    return false; // Local render.
    }

  return true;

}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::BeginStillRender()
{
  // When BeginStillRender() is called, we are assured that
  // UpdateAllRepresentations() has been called ensuring that all representation
  // pipelines are up-to-date. Since the representation strategies gurantee that
  // the full-res pipelines are always updated, we don't have to worry about
  // whether LOD is enabled when the UpdateAllRepresentations() was called.
 
  // Find out whether we are going to render with or without compositing.
  // We use the full res data size for this decision.
  this->LastCompositingDecision = 
    this->GetCompositingDecision(this->GetVisibileFullResDataSize(), 1);

  // If the collection decision has changed our representation pipelines may be
  // out of date. Hence, we tell the superclass to update representations once
  // again prior to performing the render.
  // TODO: call this method only if the collection decision really changed.
  this->SetForceRepresentationUpdate(true);

  this->SetUseCompositing(this->LastCompositingDecision);

  this->Superclass::BeginStillRender();
}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::BeginInteractiveRender()
{
  // When BeginInteractiveRender() is called we are assured that
  // UpdateAllRepresentations() has been called.

  // Give the superclass a chance to decide if it wants to use LOD or not.
  this->Superclass::BeginInteractiveRender();

  // Update all representations prior to using their datasizes to determine if
  // compositing should be used. This is necessary since if LOD decision changed
  // from false to true, then, the LOD pipelines will be invalid and we need to
  // use the LOD data information for the collection decision.
  if (this->GetForceRepresentationUpdate())
    {
    this->SetForceRepresentationUpdate(false);
    this->UpdateAllRepresentations();
    }

  this->LastCompositingDecision = 
    this->GetCompositingDecision(this->GetVisibleDisplayedDataSize(), 0);

  // If the collection decision has changed our representation pipelines may be
  // out of date. Hence, we tell the superclass to update representations once
  // again prior to performing the render.
  // TODO: call this method only if the collection decision really changed.
  this->SetForceRepresentationUpdate(true);

  this->SetUseCompositing(this->LastCompositingDecision);
}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::SetUseCompositing(bool usecompositing)
{
  // Update the view information so that all representations/strategies will be
  // made aware of the new UseCompositing state.
  this->Information->Set(USE_COMPOSITING(), usecompositing? 1: 0);
}

//-----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // Check if it's possible to access display on the server side.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkPVServerInformation* serverInfo = pm->GetServerInformation(this->ConnectionID);
  if (serverInfo && !serverInfo->GetRemoteRendering())
    {
    this->RemoteRenderAvailable = false;
    }
  else
    {
    vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
    pm->GatherInformation(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
    this->RemoteRenderAvailable = (di->GetCanOpenDisplay() == 1);
    di->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoteRenderThreshold: " 
    << this->RemoteRenderThreshold << endl;
  os << indent << "RemoteRenderAvailable: " 
    << this->RemoteRenderAvailable << endl;
}


