/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTCompositeViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTCompositeViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMIceTCompositeViewProxy);
vtkCxxRevisionMacro(vtkSMIceTCompositeViewProxy, "1.1");
vtkCxxSetObjectMacro(vtkSMIceTCompositeViewProxy, SharedMultiViewManager, vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMIceTCompositeViewProxy, SharedParallelRenderManager, vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMIceTCompositeViewProxy, SharedRenderWindow, vtkSMProxy);
//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::vtkSMIceTCompositeViewProxy()
{
  this->MultiViewManager = 0;
  this->ParallelRenderManager = 0;
  this->ImageReductionFactor = 1;
  this->CompositeThreshold = 20.0;

  this->SharedParallelRenderManager = 0;
  this->SharedMultiViewManager = 0;
  this->SharedRenderWindow = 0;

  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->TileMullions[0] = this->TileMullions[1] = 0;
}

//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::~vtkSMIceTCompositeViewProxy()
{
  this->SetSharedMultiViewManager(0);
  this->SetSharedParallelRenderManager(0);
  this->SetSharedRenderWindow(0);
}

//----------------------------------------------------------------------------
bool vtkSMIceTCompositeViewProxy::BeginCreateVTKObjects(int numObjects)
{
  if (!this->Superclass::BeginCreateVTKObjects(numObjects))
    {
    return false;
    }

  this->MultiViewManager = this->SharedMultiViewManager?
    this->SharedMultiViewManager : this->GetSubProxy("MultiViewManager");
  this->ParallelRenderManager = this->SharedParallelRenderManager?
    this->SharedParallelRenderManager: this->GetSubProxy("ParallelRenderManager");

  if (!this->ParallelRenderManager)
    {
    vtkErrorMacro("ParallelRenderManager must be defined.");
    return false;
    }

  if (this->MultiViewManager)
    {
    // As far as vtkSMIceTCompositeViewProxy is concerned 
    // CLIENT == RENDER_SERVER_ROOT. However, in client-server configurations, the
    // multiview manager is created on the client and the render server root.
    // Hence we set the flags accordingly.
    this->MultiViewManager->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER_ROOT);
    }

  this->ParallelRenderManager->SetServers(vtkProcessModule::RENDER_SERVER);

  if (this->SharedRenderWindow && !this->RenderWindowProxy->GetObjectsCreated())
    {
    // This code simply replaces the render window to use the shared render
    // window (which is the case w/o client-server. However subclasses may want to
    // fix this to create a new render window instance on the client.
    this->RenderWindowProxy = this->SharedRenderWindow;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::EndCreateVTKObjects(int numObjects)
{
  this->Superclass::EndCreateVTKObjects(numObjects);
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMIntVectorProperty* ivp;

  // Anti-aliasing generally screws up compositing.  Turn it off.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
    (pm->GetNumberOfPartitions(this->ConnectionID) > 1))
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->RenderWindowProxy->GetID(0) 
           << "SetMultiSamples" << 0
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    }

  // * Initialize the MultiViewManager, if one exists.
  
  if (this->MultiViewManager)
    {
    // Set the render windows on the two manager. 
    this->Connect(this->RenderWindowProxy, this->MultiViewManager, "RenderWindow");

    // Make the multiview manager aware of our renderers.
    stream  << vtkClientServerStream::Invoke
      << this->MultiViewManager->GetID(0)
      << "AddRenderer" << (int)this->GetSelfID().ID
      << this->RendererProxy->GetID(0)
      << vtkClientServerStream::End;

    stream  << vtkClientServerStream::Invoke
      << this->MultiViewManager->GetID(0)
      << "AddRenderer" << (int)this->GetSelfID().ID
      << this->Renderer2DProxy->GetID(0)
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT, stream);
    this->MultiViewManager->UpdateVTKObjects();
    }


  // * Initialize the ParallelRenderManager.
  this->Connect(this->RenderWindowProxy, 
    this->ParallelRenderManager, "RenderWindow");

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("TileDimensions"));
  if (ivp)
    {
    ivp->SetElements(this->TileDimensions);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("TileMullions"));
  if (ivp)
    {
    ivp->SetElements(this->TileMullions);
    }
  this->ParallelRenderManager->UpdateVTKObjects();

  if (!this->SharedParallelRenderManager)
    {
    // If SharedParallelRenderManager is set, then we assume that the controller
    // on the parallel render manager has been initialized by the one who own
    // the shared parallel render manager. Hence, we only set the controller if
    // the parallel render manager is not shared.
    stream  << vtkClientServerStream::Invoke
            << pm->GetProcessModuleID()
            << "GetController"
            << vtkClientServerStream::End;
    stream  << vtkClientServerStream::Invoke
            << this->ParallelRenderManager->GetID(0)
            << "SetController" << vtkClientServerStream::LastResult
            << vtkClientServerStream::End;
    stream  << vtkClientServerStream::Invoke
            << this->ParallelRenderManager->GetID(0)
            << "InitializeRMIs" 
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      this->ParallelRenderManager->GetServers(), stream);
    }

  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    // Non-mesa, X offscreen rendering requires access to the display
    vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
    pm->GatherInformation(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
    if (di->GetCanOpenDisplay())
      {
      this->ParallelRenderManager->InvokeCommand("InitializeOffScreen");
      }
    di->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetGUISize(int x, int y)
{
  this->Superclass::SetGUISize(x, y);

  if (this->MultiViewManager)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke 
            << this->MultiViewManager->GetID(0)
            << "SetGUISize" << x << y
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      this->MultiViewManager->GetServers(), stream);
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderWindowProxy->GetProperty("RenderWindowSize"));
  if (ivp)
    {
    ivp->SetElements2(x, y);
    this->RenderWindowProxy->UpdateProperty("RenderWindowSize");
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetViewPosition(int x, int y)
{
  this->Superclass::SetViewPosition(x, y);
  
  if (this->MultiViewManager)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke 
            << this->MultiViewManager->GetID(0)
            << "SetWindowPosition" << x << y
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      this->MultiViewManager->GetServers(), stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetViewSize(int x, int y)
{
  if (this->MultiViewManager)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID(0)
            << "SetWindowSize" << x << y
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      this->MultiViewManager->GetServers(), stream);
    }
}

//----------------------------------------------------------------------------
bool vtkSMIceTCompositeViewProxy::GetCompositingDecision(
  unsigned long totalMemory, int vtkNotUsed(stillRender))
{
  if (static_cast<float>(totalMemory)/1000.0 < this->CompositeThreshold)
    {
    return false; // Local render.
    }

  return true;

}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::BeginStillRender()
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

  // Turn off image reduction factor, since we don't use any image reduction
  // will doing still renders.
  this->SetImageReductionFactorInternal(1);
  this->SetUseCompositing(this->LastCompositingDecision);

  this->Superclass::BeginStillRender();
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::BeginInteractiveRender()
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

  if (this->LastCompositingDecision)
    {
    // Set the user-specified image reduction factor to use for compositing.
    this->SetImageReductionFactorInternal(this->ImageReductionFactor);
    }

  this->SetUseCompositing(this->LastCompositingDecision);
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetImageReductionFactorInternal(int factor)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("ImageReductionFactor"));
  if (ivp)
    {
    ivp->SetElement(0, factor);
    this->ParallelRenderManager->UpdateProperty("ImageReductionFactor");
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetUseCompositing(bool usecompositing)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("UseCompositing"));
  if (ivp)
    {
    ivp->SetElement(0, usecompositing? 1 : 0);
    this->ParallelRenderManager->UpdateProperty("UseCompositing");
    } 

  // We need to make all representation strategies aware of our compositing
  // decision. To do that, we set it on the ViewHelper. All strategies use
  // ViewHelper to get LOD or Compositing decisions made by the view.
  if (this->ViewHelper)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->ViewHelper->GetProperty("UseCompositing"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property CompositingFlag on ViewHelper.");
      return;
      }
    ivp->SetElement(0, (usecompositing? 1 : 0));
    this->ViewHelper->UpdateProperty("CompositingFlag");
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageReductionFactor: " << this->ImageReductionFactor << endl;
}


