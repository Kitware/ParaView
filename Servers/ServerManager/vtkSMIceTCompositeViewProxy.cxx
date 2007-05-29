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
#include "vtkSMProxyManager.h"
#include "vtkSMSimpleParallelStrategy.h"
#include "vtkCollectionIterator.h"
#include "vtkCollection.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMDataRepresentationProxy.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIceTCompositeViewProxy);
vtkCxxRevisionMacro(vtkSMIceTCompositeViewProxy, "1.6");
vtkCxxSetObjectMacro(vtkSMIceTCompositeViewProxy, SharedMultiViewManager, vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMIceTCompositeViewProxy, SharedParallelRenderManager, vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMIceTCompositeViewProxy, SharedRenderWindow, vtkSMProxy);
//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::vtkSMIceTCompositeViewProxy()
{
  this->MultiViewManager = 0;
  this->ParallelRenderManager = 0;
  this->KdTree = 0;
  this->KdTreeManager = 0;

  this->ImageReductionFactor = 1;
  this->CompositeThreshold = 20.0;

  this->SharedParallelRenderManager = 0;
  this->SharedMultiViewManager = 0;
  this->SharedRenderWindow = 0;

  this->DisableOrderedCompositing  = 0;

  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->TileMullions[0] = this->TileMullions[1] = 0;

  this->ActiveStrategyVector = new vtkSMRepresentationStrategyVector();
}

//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::~vtkSMIceTCompositeViewProxy()
{
  this->SetSharedMultiViewManager(0);
  this->SetSharedParallelRenderManager(0);
  this->SetSharedRenderWindow(0);

  delete this->ActiveStrategyVector;
  this->ActiveStrategyVector=0;
}

//----------------------------------------------------------------------------
bool vtkSMIceTCompositeViewProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->MultiViewManager = this->SharedMultiViewManager?
    this->SharedMultiViewManager : this->GetSubProxy("MultiViewManager");
  this->ParallelRenderManager = this->SharedParallelRenderManager?
    this->SharedParallelRenderManager: this->GetSubProxy("ParallelRenderManager");

  this->KdTree = this->GetSubProxy("KdTree");
  this->KdTreeManager = this->GetSubProxy("KdTreeManager");

  if (!this->KdTreeManager)
    {
    vtkErrorMacro("KdTreeManager must be defined.");
    return false;
    }

  if (!this->KdTree)
    {
    vtkErrorMacro("KdTree must be defined.");
    return false;
    }

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
  this->KdTree->SetServers(vtkProcessModule::RENDER_SERVER);
  this->KdTreeManager->SetServers(vtkProcessModule::RENDER_SERVER);

  if (this->SharedRenderWindow && !this->RenderWindowProxy->GetObjectsCreated())
    {
    // FIXME: can't simply replace the ptr, since we want the exposed properties
    // to go the correct render window.

    // This code simply replaces the render window to use the shared render
    // window (which is the case w/o client-server. However subclasses may want to
    // fix this to create a new render window instance on the client.
    this->RenderWindowProxy = this->SharedRenderWindow;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMIntVectorProperty* ivp;

  // Anti-aliasing generally screws up compositing.  Turn it off.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
    (pm->GetNumberOfPartitions(this->ConnectionID) > 1))
    {
    vtkClientServerStream stream2;
    stream2 << vtkClientServerStream::Invoke
            << this->RenderWindowProxy->GetID() 
            << "SetMultiSamples" << 0
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,vtkProcessModule::RENDER_SERVER,stream2);
    }

  // * Initialize the MultiViewManager, if one exists.
  
  if (this->MultiViewManager)
    {
    // Set the render windows on the two manager. 
    this->Connect(this->RenderWindowProxy, this->MultiViewManager, "RenderWindow");

    // Make the multiview manager aware of our renderers.
    stream  << vtkClientServerStream::Invoke
      << this->MultiViewManager->GetID()
      << "AddRenderer" << (int)this->GetSelfID().ID
      << this->RendererProxy->GetID()
      << vtkClientServerStream::End;

    stream  << vtkClientServerStream::Invoke
      << this->MultiViewManager->GetID()
      << "AddRenderer" << (int)this->GetSelfID().ID
      << this->Renderer2DProxy->GetID()
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
            << this->ParallelRenderManager->GetID()
            << "SetController" << vtkClientServerStream::LastResult
            << vtkClientServerStream::End;
    stream  << vtkClientServerStream::Invoke
            << this->ParallelRenderManager->GetID()
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


  // Initialize the ordered compositing stuff.
  this->Connect(this->KdTree, this->KdTreeManager, "KdTree");

  // This call may fail if the server-side renderer is not vtkIceTRenderer.
  // Hence subclasses must be careful about that.
  stream  << vtkClientServerStream::Invoke
          << this->RendererProxy->GetID()
          << "SetSortingKdTree"
          << this->KdTree->GetID()
          << vtkClientServerStream::End;

  // Set the controllers for the kdtree.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->KdTree->GetID() 
          << "SetController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << vtkClientServerStream::LastResult 
          << "GetNumberOfProcesses"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->KdTree->GetID() 
          << "SetNumberOfRegionsOrMore"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << vtkClientServerStream::LastResult 
          << "GetNumberOfProcesses"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->KdTreeManager->GetID()
          << "SetNumberOfPieces"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;

  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
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
            << this->MultiViewManager->GetID()
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
            << this->MultiViewManager->GetID()
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
            << this->MultiViewManager->GetID()
            << "SetWindowSize" << x << y
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      this->MultiViewManager->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMIceTCompositeViewProxy::NewStrategyInternal(
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

  // Update ordered compositing tree.
  this->UpdateOrderedCompositingPipeline();

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
void vtkSMIceTCompositeViewProxy::UpdateOrderedCompositingPipeline()
{
  this->ActiveStrategyVector->clear();
  
  bool ordered_compositing_needed = false;

  // Collect active strategies from all the representations.
  // Check is any of the representations says that it requires ordered
  // compositing for rendering.
  vtkCollectionIterator* iter = this->Representations->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDataRepresentationProxy* repr = 
      vtkSMDataRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr || !repr->GetVisibility())
      {
      continue;
      }
    repr->GetActiveStrategies(*this->ActiveStrategyVector); 
    ordered_compositing_needed |= repr->GetOrderedCompositingNeeded();
    }
  iter->Delete();

  // If ordered compositing is disabled or no representation requires ordered
  // compositing or if we are doing a local render we don't need ordered
  // compositing. Hence we tell all the strategies so.
  if (this->DisableOrderedCompositing || !ordered_compositing_needed 
    || !this->LastCompositingDecision)
    {
    // Nothing much to do, just update all strategies so that they disable the
    // distributor.
    this->SetOrderedCompositingDecision(false);
    this->ActiveStrategyVector->clear();
    return;
    }

  // We've decided that we need ordered compositing.

  // Update the data inputs to the KdTreeManager and ask it to update.
  vtkSMProxyProperty* ppProducers = vtkSMProxyProperty::SafeDownCast(
    this->KdTreeManager->GetProperty("Producers"));
  ppProducers->RemoveAllProxies();

  vtkSMProxyProperty* ppStructuredProducer = vtkSMProxyProperty::SafeDownCast(
    this->KdTreeManager->GetProperty("StructuredProducer"));
  ppStructuredProducer->RemoveAllProxies();
  ppStructuredProducer->AddProxy(0);

  vtkSMRepresentationStrategyVector::iterator strategyIter;
  for (strategyIter = this->ActiveStrategyVector->begin();
    strategyIter != this->ActiveStrategyVector->end(); ++strategyIter)
    {
    if (strcmp(strategyIter->GetPointer()->GetXMLName(), 
        "UniformGridParallelStrategy") == 0)
      {
      ppStructuredProducer->RemoveAllProxies();
      ppStructuredProducer->AddProxy(strategyIter->GetPointer()->GetOutput());
      strategyIter->GetPointer()->Update();
      }
    else
      {
      vtkSMSimpleParallelStrategy* pstrategy = 
        vtkSMSimpleParallelStrategy::SafeDownCast(strategyIter->GetPointer());
      if (pstrategy && pstrategy->GetDistributedSource())
        {
        ppProducers->AddProxy(pstrategy->GetDistributedSource());
        pstrategy->UpdateDistributedData();
        }
      }
    }
  this->KdTreeManager->UpdateVTKObjects();
  this->KdTreeManager->InvokeCommand("Update");

  this->SetOrderedCompositingDecision(true);
  this->ActiveStrategyVector->clear();
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetOrderedCompositingDecision(bool decision)
{
  // Iterate over all active strategies and pass the decision to them.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ViewHelper->GetProperty("UseOrderedCompositing"));
  ivp->SetElement(0, decision? 1 : 0);
  this->ViewHelper->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageReductionFactor: " << this->ImageReductionFactor << endl;
}


