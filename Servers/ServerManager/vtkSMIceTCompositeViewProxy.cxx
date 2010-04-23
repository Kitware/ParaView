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
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkIceTConstants.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"
#include "vtkSMDataRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMUnstructuredDataParallelStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSMUniformGridParallelStrategy.h"
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIceTCompositeViewProxy);

vtkInformationKeyMacro(vtkSMIceTCompositeViewProxy, KD_TREE, ObjectBase);
//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::vtkSMIceTCompositeViewProxy()
{
  this->MultiViewManager = 0;
  this->ParallelRenderManager = 0;
  this->KdTree = 0;
  this->KdTreeManager = 0;

  this->ImageReductionFactor = 1;

  this->DisableOrderedCompositing  = 0;

  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->TileMullions[0] = this->TileMullions[1] = 0;
  this->EnableTiles = false;

  this->LastCompositingDecision = false;
  this->LastOrderedCompositingDecision = false;

  this->ActiveStrategyVector = new vtkSMRepresentationStrategyVector();

  this->Information->Set(KD_TREE(), 0);

  this->ViewSize[0] = this->ViewSize[1] = 400;

  this->RenderersID =0;
  this->UsingIceTRenderers = true;
}

//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::~vtkSMIceTCompositeViewProxy()
{
  if (this->MultiViewManager && this->RenderersID)
    {
    // Remove renderers from the MultiViewManager.
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
      << this->MultiViewManager->GetID()
      << "RemoveAllRenderers" 
      << this->RenderersID
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER_ROOT, stream);
    this->RenderersID = 0;
    }

  delete this->ActiveStrategyVector;
  this->ActiveStrategyVector=0;
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::InitializeForMultiView(vtkSMViewProxy* view)
{
  vtkSMIceTCompositeViewProxy* otherView =
    vtkSMIceTCompositeViewProxy::SafeDownCast(view);
  if (!otherView)
    {
    vtkErrorMacro("Other view must be a vtkSMIceTCompositeViewProxy.");
    return;
    }

  if (this->ObjectsCreated)
    {
    vtkErrorMacro("InitializeForMultiView must be called before CreateVTKObjects.");
    return;
    }

  if (!otherView->GetObjectsCreated())
    {
    vtkErrorMacro(
      "InitializeForMultiView was called before the other view was intialized.");
    return;
    }

  this->SharedParallelRenderManagerID = 
    otherView->ParallelRenderManager->GetID();

  this->SharedMultiViewManagerID = otherView->MultiViewManager? 
    otherView->MultiViewManager->GetID() : vtkClientServerID(0);

  this->SharedRenderWindowID = otherView->RenderWindowProxy->GetID();

  this->Superclass::InitializeForMultiView(view);
}

//----------------------------------------------------------------------------
bool vtkSMIceTCompositeViewProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->MultiViewManager = this->GetSubProxy("MultiViewManager");
  this->ParallelRenderManager = this->GetSubProxy("ParallelRenderManager");

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

  if (!this->SharedRenderWindowID.IsNull() && 
    !this->RenderWindowProxy->GetObjectsCreated())
    {
    // Share the render window.
    this->RenderWindowProxy->InitializeAndCopyFromID(
      this->SharedRenderWindowID);
    }

  if (!this->SharedParallelRenderManagerID.IsNull() &&
    !this->ParallelRenderManager->GetObjectsCreated())
    {
    this->ParallelRenderManager->InitializeAndCopyFromID(
      this->SharedParallelRenderManagerID);
    }

  if (!this->SharedMultiViewManagerID.IsNull() &&
    !this->MultiViewManager->GetObjectsCreated())
    {
    this->MultiViewManager->InitializeAndCopyFromID(
      this->SharedMultiViewManagerID);
    }

  this->Information->Set(KD_TREE(), this->KdTree);

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
    this->RenderersID = static_cast<int>(this->GetSelfID().ID);
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID()
            << "AddRenderer"
            << this->RenderersID
            << this->RendererProxy->GetID()
            << vtkClientServerStream::End;

    stream  << vtkClientServerStream::Invoke
      << this->MultiViewManager->GetID()
      << "AddRenderer" << (int)this->GetSelfID().ID
      << this->Renderer2DProxy->GetID()
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT, stream);
    this->MultiViewManager->UpdateVTKObjects();

    // HACK: If MultiViewManager exists, it implies that the view module is used on a
    // MPI process, disable interaction.
    this->Interactor->SetPVRenderView(0);
    this->Interactor->Disable();
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

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("EnableTiles"));
  if (ivp)
    {
    ivp->SetElement(0, this->EnableTiles? 1 : 0);
    }
  this->ParallelRenderManager->UpdateVTKObjects();

  if (this->SharedParallelRenderManagerID.IsNull())
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

  // Initialize the ordered compositing stuff.
  this->Connect(this->KdTree, this->KdTreeManager, "KdTree");

  if (this->UsingIceTRenderers)
    {
    // This call may fail if the server-side renderer is not vtkIceTRenderer.
    // Hence subclasses must be careful about that.
    stream  << vtkClientServerStream::Invoke
            << this->RendererProxy->GetID()
            << "SetSortingKdTree"
            << this->KdTree->GetID()
            << vtkClientServerStream::End;
    }

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

  // NOTE: vtkSMIceTDesktopRenderViewProxy must not call this method.
  // GUI Size is the render window size on all processes.
  // For this to work correctly, the GUISize set on all view proxies must be
  // same.
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
  // NOTE: vtkSMIceTDesktopRenderViewProxy must not call this method.
  this->UpdateViewport();
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::SetViewSize(int x, int y)
{
  // NOTE: vtkSMIceTDesktopRenderViewProxy must not call this method.
  this->ViewSize[0] = x;
  this->ViewSize[1] = y;
  this->UpdateViewport();
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::UpdateViewport()
{
  double viewport[4];
  viewport[0] = this->ViewPosition[0]/(double)this->GUISize[0];
  viewport[1] = this->ViewPosition[1]/(double)this->GUISize[1];
  viewport[2] = (this->ViewPosition[0] + this->ViewSize[0])/
    (double) this->GUISize[0];
  viewport[3] = (this->ViewPosition[1] + this->ViewSize[1])/
    (double) this->GUISize[1];
 
  // Set the view port on all renders belonging to this view on all processes.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->RendererProxy->GetID()
          << "SetViewport"
          << viewport[0] << viewport[1] << viewport[2] << viewport[3]
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->Renderer2DProxy->GetID()
          << "SetViewport"
          << viewport[0] << viewport[1] << viewport[2] << viewport[3]
          << vtkClientServerStream::End;

  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::BeginStillRender()
{
  if (this->MultiViewManager)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID()
            << "SetActiveViewID"
            << this->RenderersID
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT,
      stream);
    }
  // Let the superclass decide if we are using compositing at all.
  this->Superclass::BeginStillRender();

  // Turn off image reduction factor, since we don't use any image reduction
  // will doing still renders.
  this->SetImageReductionFactorInternal(1);

  // Update ordered compositing tree.
  this->UpdateOrderedCompositingPipeline();
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::BeginInteractiveRender()
{
  if (this->MultiViewManager)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID()
            << "SetActiveViewID"
            << this->RenderersID
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER_ROOT,
      stream);
    }

  // Give the superclass a chance to decide if it wants to use LOD or not.
  // Let the superclass decide if we are using compositing at all.
  this->Superclass::BeginInteractiveRender();

  if (this->LastCompositingDecision)
    {
    // Set the user-specified image reduction factor to use for compositing.
    this->SetImageReductionFactorInternal(this->ImageReductionFactor);
    }
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

  this->Superclass::SetUseCompositing(usecompositing);
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

  vtkSMProxyProperty* ppProducers = vtkSMProxyProperty::SafeDownCast(
    this->KdTreeManager->GetProperty("Producers"));

  vtkSMProxyProperty* ppStructuredProducer = vtkSMProxyProperty::SafeDownCast(
    this->KdTreeManager->GetProperty("StructuredProducer"));

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

    // Clean references to strategy outputs kept by the KdTreeManager.
    ppProducers->RemoveAllProxies();
    if (ppStructuredProducer->GetNumberOfProxies() > 0 &&
      ppStructuredProducer->GetProxy(0))
      {
      ppStructuredProducer->RemoveAllProxies();
      ppStructuredProducer->AddProxy(0);
      }
    this->KdTreeManager->UpdateVTKObjects();
    return;
    }

  // We've decided that we need ordered compositing.

  // Update the data inputs to the KdTreeManager and ask it to update.
  ppProducers->RemoveAllProxies();
  ppStructuredProducer->RemoveAllProxies();
  ppStructuredProducer->AddProxy(0);

  vtkSMRepresentationStrategyVector::iterator strategyIter;
  for (strategyIter = this->ActiveStrategyVector->begin();
    strategyIter != this->ActiveStrategyVector->end(); ++strategyIter)
    {
    if (strcmp(strategyIter->GetPointer()->GetXMLName(), 
        "UniformGridParallelStrategy") == 0)
      {
      vtkSMUniformGridParallelStrategy* ugps =
        vtkSMUniformGridParallelStrategy::SafeDownCast(strategyIter->GetPointer());
      ppStructuredProducer->RemoveAllProxies();
      ppStructuredProducer->AddProxy(
        ugps->vtkSMSimpleStrategy::GetOutput());
      
      // cout << strategyIter->GetPointer()->UpdateRequired() << endl; 
      // Essential to update the representation, so that the
      // KdTree generator gets the updated data to use when generating the
      // KdTree.
      // It is safe to call Update() on representations or strategies after the
      // compositing/remote render decision has been made.
      strategyIter->GetPointer()->Update();
      }
    else
      {
      vtkSMUnstructuredDataParallelStrategy* pstrategy = 
        vtkSMUnstructuredDataParallelStrategy::SafeDownCast(
          strategyIter->GetPointer());
      if (pstrategy && pstrategy->GetDistributedSource())
        {
        ppProducers->AddProxy(pstrategy->GetDistributedSource());
        pstrategy->UpdateDistributedData();
        // We mark the distributed data invalid since we'd be rebuilding the
        // KdTree, and the distributor must be re-executed after the KdTree has
        // changed. This method is smart enough to only mark the distributed
        // data invalid, not the entire pipeline which saves ton of pipeline
        // rexecutions.
        // Although we are making the distributed data invalid on every render.
        // The OrderedCompositingDistrubitor is smart enough to re-distribute
        // data only if something changed, hence distribution does not actually
        // take place on every render unless something changed.
        pstrategy->InvalidateDistributedData();
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
  this->Information->Set(USE_ORDERED_COMPOSITING(), decision? 1 : 0);

  if (this->LastOrderedCompositingDecision == decision)
    {
    return;
    }
  this->LastOrderedCompositingDecision = decision;

  if (this->UsingIceTRenderers)
    {
    // Cannot do this with the server manager because this method only
    // exists on the render server, not the client.
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke << this->RendererProxy->GetID()
            << "SetComposeOperation"
            << (decision? vtkIceTConstants::ComposeOperationOver :
              vtkIceTConstants::ComposeOperationClosest)
            << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::RemoveRepresentationInternal(
  vtkSMRepresentationProxy* repr)
{
  // Clean inputs going to the KdTreeManager so that we don't keep references to
  // algorithms in the strategies used by the representation being removed.
  vtkSMProxyProperty* ppProducers = vtkSMProxyProperty::SafeDownCast(
    this->KdTreeManager->GetProperty("Producers"));
  ppProducers->RemoveAllProxies();

  vtkSMProxyProperty* ppStructuredProducer = vtkSMProxyProperty::SafeDownCast(
    this->KdTreeManager->GetProperty("StructuredProducer"));
  ppStructuredProducer->RemoveAllProxies();
  ppStructuredProducer->AddProxy(0);
  this->KdTreeManager->UpdateVTKObjects();

  this->Superclass::RemoveRepresentationInternal(repr);
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMIceTCompositeViewProxy::CaptureWindow(int magnification)
{
  // NOTE: vtkSMIceTDesktopRenderViewProxy must not call this method.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  if (this->MultiViewManager)
    {
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID()
            << "SetActiveViewID"
            << this->RenderersID
            << vtkClientServerStream::End;
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID()
            << "StartMagnificationFix"
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->MultiViewManager->GetServers(), stream);
    }

  vtkImageData* capture = this->Superclass::CaptureWindow(magnification);

  if (this->MultiViewManager)
    {
    stream  << vtkClientServerStream::Invoke
            << this->MultiViewManager->GetID()
            << "EndMagnificationFix"
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->MultiViewManager->GetServers(), stream);
    }
  return capture;
}


//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageReductionFactor: " << this->ImageReductionFactor << endl;
  os << indent << "DisableOrderedCompositing: " 
    << this->DisableOrderedCompositing << endl;
}


