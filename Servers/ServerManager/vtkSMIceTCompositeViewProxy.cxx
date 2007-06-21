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
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"
#include "vtkSMDataRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMSimpleParallelStrategy.h"
#include "vtkSMSourceProxy.h"

#include "vtkPVConfig.h" // for PARAVIEW_USE_ICE_T
#include "vtkToolkits.h" // for VTK_USE_MPI

#ifdef VTK_USE_MPI 
# ifdef PARAVIEW_USE_ICE_T
#   include "vtkIceTRenderManager.h"
# endif
#endif

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIceTCompositeViewProxy);
vtkCxxRevisionMacro(vtkSMIceTCompositeViewProxy, "1.8");

vtkInformationKeyMacro(vtkSMIceTCompositeViewProxy, KD_TREE, ObjectBase);
//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::vtkSMIceTCompositeViewProxy()
{
  this->MultiViewManager = 0;
  this->ParallelRenderManager = 0;
  this->KdTree = 0;
  this->KdTreeManager = 0;

  this->ImageReductionFactor = 1;
  this->CompositeThreshold = 20.0;

  this->DisableOrderedCompositing  = 0;

  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->TileMullions[0] = this->TileMullions[1] = 0;

  this->LastCompositingDecision = false;
  this->LastOrderedCompositingDecision = false;

  this->ActiveStrategyVector = new vtkSMRepresentationStrategyVector();

  this->Information->Set(KD_TREE(), 0);
}

//----------------------------------------------------------------------------
vtkSMIceTCompositeViewProxy::~vtkSMIceTCompositeViewProxy()
{
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

  otherView->UpdateVTKObjects();

  this->SharedParallelRenderManagerID = 
    otherView->SharedParallelRenderManagerID.IsNull()?
    otherView->ParallelRenderManager->GetID():
    otherView->SharedParallelRenderManagerID; 

  this->SharedMultiViewManagerID = otherView->SharedMultiViewManagerID.IsNull()?
    (otherView->MultiViewManager? otherView->MultiViewManager->GetID() : 0) : 
    otherView->SharedMultiViewManagerID; 

  this->SharedRenderWindowID = otherView->SharedRenderWindowID.IsNull()?
    otherView->RenderWindowProxy->GetID() : otherView->SharedRenderWindowID;

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

//----------------------------------------------------------------------------
void vtkSMIceTCompositeViewProxy::BeginStillRender()
{
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
  // When BeginInteractiveRender() is called we are assured that
  // UpdateAllRepresentations() has been called.

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
      ppStructuredProducer->RemoveAllProxies();
      ppStructuredProducer->AddProxy(strategyIter->GetPointer()->GetOutput());
      
      // Essential to update the representation, so that the
      // KdTree generator gets the updated data to use when generating the
      // KdTree.
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
  this->Information->Set(USE_ORDERED_COMPOSITING(), decision? 1 : 0);

  if (this->LastOrderedCompositingDecision == decision)
    {
    return;
    }
  this->LastOrderedCompositingDecision = decision;


#ifdef VTK_USE_MPI 
# ifdef PARAVIEW_USE_ICE_T
  // Cannot do this with the server manager because this method only
  // exists on the render server, not the client.
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke << this->RendererProxy->GetID()
          << "SetComposeOperation"
          << (decision? vtkIceTRenderManager::ComposeOperationOver :
            vtkIceTRenderManager::ComposeOperationClosest)
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
# endif
#endif
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
void vtkSMIceTCompositeViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageReductionFactor: " << this->ImageReductionFactor << endl;
}


