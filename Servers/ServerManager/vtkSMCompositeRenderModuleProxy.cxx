/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeRenderModuleProxy.h"
#include "vtkSMCompositeDisplayProxy.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkCollection.h"
#include "vtkSMProxyProperty.h"
#include "vtkRenderWindow.h"
#include "vtkPVTreeComposite.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkSMCompositeRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMCompositeRenderModuleProxy, "1.1.2.5");
//-----------------------------------------------------------------------------
vtkSMCompositeRenderModuleProxy::vtkSMCompositeRenderModuleProxy()
{
  this->LocalRender = 1;
  this->CompositeThreshold = 20.0;
  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;
  this->ReductionFactor = 2;
  this->SquirtLevel = 0;
  this->CompositeManagerProxy = 0;
  this->SetDisplayXMLName("CompositeDisplay");
}

//-----------------------------------------------------------------------------
vtkSMCompositeRenderModuleProxy::~vtkSMCompositeRenderModuleProxy()
{
  this->CompositeManagerProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated )
    {
    return;
    }
  // Give subclasses a chance to decide on the CompositeManager.
  this->CreateCompositeManager();
  
  this->CompositeManagerProxy = this->GetSubProxy("CompositeManager");
  
  if (!this->CompositeManagerProxy)
    {
    //TODO: remove this before committing.
    vtkWarningMacro("CompositeManagerProxy not defined. ");
    }
  this->Superclass::CreateVTKObjects(numObjects);

  // Give subclasses a chance to initialized the CompositeManager.
  this->InitializeCompositingPipeline();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::InitializeCompositingPipeline()
{
  if (!this->CompositeManagerProxy)
    {
    return;
    }
  
  vtkSMProperty *p;
  vtkSMProxyProperty* pp;
  vtkSMIntVectorProperty* ivp;

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
 
  p = this->CompositeManagerProxy->GetProperty("InitializeRMIs");
  if (!p)
    {
    vtkErrorMacro("Failed to find property InitializeRMIs on CompositeManagerProxy.");
    return;
    }
  p->Modified();
  this->CompositeManagerProxy->UpdateVTKObjects();
  // Some CompositeManagerProxies need that InitializeRMIs is called before RenderWindow
  // is set.
 
  pp = vtkSMProxyProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("RenderWindow"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find proeprty RenderWindow on CompositeManagerProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RenderWindowProxy);

  if (getenv("PV_DISABLE_COMPOSITE_INTERRUPTS"))
    {
    p = this->CompositeManagerProxy->GetProperty("EnableAbort");
    // MultiDisplayManager doesn't have EnableAbort.
    if (p)
      {
      p->Modified();
      }
    }

  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    p = this->CompositeManagerProxy->GetProperty("InitializeOffScreen");
    if (!p)
      {
      vtkErrorMacro("Failed to find property InitializeOffScreen on CompositeManagerProxy.");
      return;
      }
    p->Modified();
    }
 
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("UseCompositing"));
  if (ivp)
    {
    // So that the server window does not popup until needed.
    ivp->SetElement(0, 0); 
    }

  this->CompositeManagerProxy->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
int vtkSMCompositeRenderModuleProxy::GetLocalRenderDecision(
  unsigned long totalMemory, int vtkNotUsed(stillRender))
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (static_cast<float>(totalMemory)/1000.0 < this->GetCompositeThreshold() ||
    ( !pm->GetOptions()->GetClientMode() &&
      pm->GetNumberOfPartitions() < 2) )
    {
    return 1; // Local render.
    }
  return 0;
}

///*******************************
// TODO: For all the render methods to work efficiently,
// I have to manage MemorySizeValid falgs!!!!
// *******************************
//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::StillRender()
{
  vtkObject* object;
  vtkSMCompositeDisplayProxy* pDisp;

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  pm->SendPrepareProgress();

  this->UpdateAllDisplays();

  // Find out whether we are going to render localy.
  // Save this so we know where to get the z buffer (for picking?).
  this->LocalRender = this->GetLocalRenderDecision(
    this->GetTotalVisibleGeometryMemorySize(), 1);

  // Change the collection flags and update.
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMCompositeDisplayProxy::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibilityCM())
      {
      this->SetCollectionDecision(pDisp, this->LocalRender);
      }
    }
  // this->UpdateAllDisplays(); //Since SetCollectionDecision invalidates geometry.
  // We don't need to call this explicitly, since Superclass::StillRender() will 
  // call UpdateAllDisplays.


  //Turn of ImageReductionFactor if the CompositeManager supports it.
  if (this->CompositeManagerProxy)
    {
    this->SetImageReductionFactor(this->CompositeManagerProxy, 1);
    this->SetSquirtLevel(this->CompositeManagerProxy, ((this->SquirtLevel)? 1 : 0) );
    this->SetUseCompositing(this->CompositeManagerProxy, ((this->LocalRender)? 0 : 1));
    }
  
  this->Superclass::StillRender();

  pm->SendCleanupPendingProgress();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::InteractiveRender()
{
  vtkObject* object;
  vtkSMCompositeDisplayProxy* pDisp;

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule()); 
  pm->SendPrepareProgress();

  this->UpdateAllDisplays();
  int useLOD = this->GetUseLODDecision();
  unsigned long totalMemory = 0;
  totalMemory = (useLOD)? this->GetTotalVisibleLODGeometryMemorySize() :
    this->GetTotalVisibleGeometryMemorySize();

  this->LocalRender = this->GetLocalRenderDecision(totalMemory, 0);

  // Change the collection flags and update.
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMCompositeDisplayProxy::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibilityCM())
      {
      // TODO: should the two decision be kept independent.
      // Why con't combine them using Shared properties and simplify
      // our life?
      if (useLOD)
        {
        this->SetLODCollectionDecision(pDisp, this->LocalRender);
        }
      else
        {
        this->SetCollectionDecision(pDisp, this->LocalRender);
        }
      }
    }
  if (this->CompositeManagerProxy)
    {
    // Set Squirt Level (if supported).
    this->SetSquirtLevel(this->CompositeManagerProxy, this->SquirtLevel );
    this->SetUseCompositing(this->CompositeManagerProxy, ((this->LocalRender)? 0 : 1));
    }

  if (!this->LocalRender)
    {
    this->GetRenderWindow()->SetDesiredUpdateRate(5.0);
    this->ComputeReductionFactor();
    }

  this->Superclass::InteractiveRender();
 
  pm->SendCleanupPendingProgress();
  
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::ComputeReductionFactor()
{
  vtkRenderWindow* renWin = this->GetRenderWindow();
  float renderTime = 1.0 / renWin->GetDesiredUpdateRate();
  int *windowSize = renWin->GetSize();
  int area, reducedArea;
  float reductionFactor;
  float timePerPixel;
  float getBuffersTime, setBuffersTime, transmitTime;
  float newReductionFactor;
  float maxReductionFactor;

  newReductionFactor = 1;
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  if (this->ReductionFactor > 1)
    {
    // We have to come up with a more consistent way to compute reduction.
    newReductionFactor = this->ReductionFactor;
    if (this->CompositeManagerProxy)
      {
      vtkPVTreeComposite *composite = 
        vtkPVTreeComposite::SafeDownCast( pm->GetObjectFromID( 
            this->CompositeManagerProxy->GetID(0)));
      if( composite ) // we know this is a vtkPVTreeComposite
        {
        // Leave halve time for compositing.
        renderTime = renderTime * 0.5;
        // Try to factor in user preference.
        renderTime = renderTime / (float)(this->ReductionFactor);
        // Compute time for each pixel on the last render.
        area = windowSize[0] * windowSize[1];
        reductionFactor = (float)composite->GetImageReductionFactor();
        reducedArea = (int)(area / (reductionFactor * reductionFactor));
        getBuffersTime = composite->GetGetBuffersTime();

        setBuffersTime = composite->GetSetBuffersTime();
        transmitTime = composite->GetCompositeTime();

        // Do not consider SetBufferTime because 
        //it is not dependent on reduction factor.,
        timePerPixel = (getBuffersTime + transmitTime) / reducedArea;
        newReductionFactor = sqrt(area * timePerPixel / renderTime);

        // Do not let the width go below 150.
        maxReductionFactor = windowSize[0] / 150.0;
        if (maxReductionFactor > this->ReductionFactor)
          {
          maxReductionFactor = this->ReductionFactor;
          }
        if (newReductionFactor > maxReductionFactor)
          {
          newReductionFactor = maxReductionFactor;
          }
        if (newReductionFactor < 1.0)
          {
          newReductionFactor = 1.0;
          }
        }
      }
    }

  if (this->CompositeManagerProxy)
    {
    // Will using properties here slow us down considerably?
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->CompositeManagerProxy->GetID(0) 
      << "SetImageReductionFactor" << int(newReductionFactor)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT, stream);
    }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::SetUseCompositing(vtkSMProxy* p, int flag)
{
  if (!p)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    p->GetProperty("UseCompositing"));
  if (!ivp)
    {
    return;
    }
  vtkTypeUInt32 old_servers = p->GetServers();
  p->SetServers(vtkProcessModule::CLIENT);
  
  ivp->SetElement(0, flag);
  p->UpdateVTKObjects();
  p->SetServers(old_servers);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::SetSquirtLevel(vtkSMProxy* p, int level)
{
  if (!p)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    p->GetProperty("SquirtLevel"));
  if (!ivp)
    {
    return;
    }
  vtkTypeUInt32 old_servers = p->GetServers();
  p->SetServers(vtkProcessModule::CLIENT);
  
  ivp->SetElement(0, level);
  p->UpdateVTKObjects();
  p->SetServers(old_servers);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::SetImageReductionFactor(vtkSMProxy* p,
  int factor)
{
  if (!p)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    p->GetProperty("ImageReductionFactor"));
  if (!ivp)
    {
    return;
    }
  vtkTypeUInt32 old_servers = p->GetServers();
  p->SetServers(vtkProcessModule::CLIENT);
  ivp->SetElement(0, factor);
  p->UpdateVTKObjects();
  p->SetServers(old_servers);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::SetCollectionDecision(
  vtkSMCompositeDisplayProxy* pDisp, int decision)
{
  // We don't use properties since it slows us down considerably.
  pDisp->SetCollectionDecision(decision);
  
}

//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::SetLODCollectionDecision(
  vtkSMCompositeDisplayProxy* pDisp, int decision)
{
  // We don't use properties since it slows us down considerably.
  pDisp->SetLODCollectionDecision(decision);
}

//-----------------------------------------------------------------------------
double vtkSMCompositeRenderModuleProxy::GetZBufferValue(int x, int y)
{
  if (this->LocalRender)
    {
    return this->Superclass::GetZBufferValue(x,y);
    }

  // Only MPI has a pointer to a composite.
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  if (!this->CompositeManagerProxy)
    {
    vtkErrorMacro("CompositeManagerProxy not defined!");
    return 0;
    }

  vtkPVTreeComposite *composite = 
    vtkPVTreeComposite::SafeDownCast( pm->GetObjectFromID( 
        this->CompositeManagerProxy->GetID(0)));
  if( composite ) // we know this is a vtkPVTreeComposite
    {
    return composite->GetZ(x, y);
    }

  // If client-server...
  if (pm->GetOptions()->GetClientMode())
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->CompositeManagerProxy->GetID(0) 
      << "GetZBufferValue" << x << y
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT, stream);
    float z = 0;
    if(pm->GetLastResult(vtkProcessModule::CLIENT).GetArgument(0, 0, &z))
      {
      return z;
      }
    else
      {
      vtkErrorMacro("Error getting float value from GetZBufferValue result.");
      }
    }

  vtkErrorMacro("Unknown RenderModule mode.");
  return 0;
}


//-----------------------------------------------------------------------------
void vtkSMCompositeRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
