// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMClientServerRenderModuleProxy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkSMClientServerRenderModuleProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkProcessModule.h"
#include "vtkObjectFactory.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkRenderWindow.h"
#include "vtkSMCompositeDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMClientServerRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMClientServerRenderModuleProxy, "1.2");

vtkCxxSetObjectMacro(vtkSMClientServerRenderModuleProxy, 
                     ServerRenderWindowProxy,
                     vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMClientServerRenderModuleProxy, 
                     ServerRenderSyncManagerProxy,
                     vtkSMProxy);

//-----------------------------------------------------------------------------
vtkSMClientServerRenderModuleProxy::vtkSMClientServerRenderModuleProxy()
{
  this->LocalRender = 1;
  this->RemoteRenderThreshold = 20.0;
  this->SquirtLevel = 0;
  this->ReductionFactor = 2;

  this->RenderModuleId = 0;

  this->RenderSyncManagerProxy = 0;
  this->ServerRenderWindowProxy = 0;
  this->ServerRenderSyncManagerProxy = 0;

  // vtkSMCompositeDisplayProxy has functionality for both client/server
  // and for various parallel rendering tasks.  It is not worth the time
  // to split this into two different classes.
  this->SetDisplayXMLName("CompositeDisplay");
}

vtkSMClientServerRenderModuleProxy::~vtkSMClientServerRenderModuleProxy()
{
  this->RenderSyncManagerProxy = 0;
  this->SetServerRenderWindowProxy(0);
  this->SetServerRenderSyncManagerProxy(0);
}

void vtkSMClientServerRenderModuleProxy::PrintSelf(ostream &os,
                                                   vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LocalRender: " << this->LocalRender << endl;
  os << indent << "RemoteRenderThreshold: "
     << this->RemoteRenderThreshold << endl;
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;
  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;

  os << indent << "RenderModuleId: " << this->RenderModuleId << endl;

  os << indent << "ServerRenderWindowProxy: ";
  if (this->ServerRenderWindowProxy)
    {
    this->ServerRenderWindowProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ServerRenderSyncManagerProxy: ";
  if (this->ServerRenderSyncManagerProxy)
    {
    this->ServerRenderSyncManagerProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated )
    {
    return;
    }
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");

  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Give subclasses a chance to decide on the RenderSyncManager.
  this->CreateRenderSyncManager();
  
  this->RenderSyncManagerProxy = this->GetSubProxy("RenderSyncManager");
  
  if (!this->RenderSyncManagerProxy)
    {
    //TODO: remove this before committing.
    vtkWarningMacro("RenderSyncManagerProxy not defined. ");
    }

  // If there is a server side render window, use it. This is
  // used in multi-view mode where the server has one render window
  // whereas the client can have many.
  if (this->ServerRenderWindowProxy)
    {
    this->RenderWindowProxy->SetServers(vtkProcessModule::CLIENT);

    // this will create the regular render window on the client.
    this->RenderWindowProxy->UpdateVTKObjects(); 

    vtkClientServerStream stream2;
    stream2 << vtkClientServerStream::Assign 
           << this->RenderWindowProxy->GetID(0)
           << this->ServerRenderWindowProxy->GetID(0)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream2);

    this->RenderWindowProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    }

  this->Superclass::CreateVTKObjects(numObjects);

  // Give subclasses a chance to initialize the render sync.
  this->InitializeRenderSyncPipeline();
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::CreateRenderSyncManager()
{
  this->RenderSyncManagerProxy = this->GetSubProxy("RenderSyncManager");
  if (!this->RenderSyncManagerProxy)
    {
    vtkErrorMacro("RenderSyncManager subproxy must be defined.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  // Similar to the renderer, we create our peculiar RenderSyncManager.
  this->RenderSyncManagerProxy->SetServers(vtkProcessModule::CLIENT);
  // The XML must define only the manager to be created on the client.

  this->RenderSyncManagerProxy->UpdateVTKObjects();
  // If there is a server side composite manager, use it. This is used in
  // multi-view mode where the server has one desktop delivery server
  // whereas the client can have many desktop delivery clients.
  if (this->ServerRenderSyncManagerProxy)
    {
    stream << vtkClientServerStream::Assign 
           << this->RenderSyncManagerProxy->GetID(0)
           << this->ServerRenderSyncManagerProxy->GetID(0)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);
    }
  else
    {
    // Create the vtkDesktopDeliveryServer.
    stream << vtkClientServerStream::New << "vtkPVDesktopDeliveryServer"
           << this->RenderSyncManagerProxy->GetID(0)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER_ROOT, stream);
    }
    
  this->RenderSyncManagerProxy->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER_ROOT);
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::InitializeRenderSyncPipeline()
{
  if (!this->RenderSyncManagerProxy)
    {
    return;
    }
  
  vtkSMProperty *p;
  vtkSMProxyProperty* pp;
  vtkSMIntVectorProperty* ivp;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkPVServerInformation* serverInfo = pm->GetServerInformation(this->ConnectionID);
  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  vtkClientServerStream stream;
  for (idx = 0; idx < numMachines; idx++)
    {
    if (serverInfo->GetEnvironment(idx))
      {
      stream << vtkClientServerStream::Invoke 
             << pm->GetProcessModuleID() << "SetProcessEnvironmentVariable"
             << idx << serverInfo->GetEnvironment(idx)
             << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, 
      stream);
    }

  stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
         << "GetRenderServerSocketController"
         << pm->GetConnectionClientServerID(this->ConnectionID)
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << this->RenderSyncManagerProxy->GetID(0)
         << "SetController" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 this->RenderSyncManagerProxy->GetServers(), stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
        this->RenderSyncManagerProxy->GetProperty("SyncRenderWindowRenderers"));
  if (!ivp)
    {
    vtkErrorMacro("Falied to find property SyncRenderWindowRenderers");
    return;
    }
  ivp->SetElement(0, 0);

  p = this->RenderSyncManagerProxy->GetProperty("InitializeRMIs");
  if (!p)
    {
    vtkErrorMacro("Failed to find property InitializeRMIs on "
                  "RenderSyncManagerProxy.");
    return;
    }
  p->Modified();
  this->RenderSyncManagerProxy->UpdateVTKObjects();
  // Some RenderSyncManagerProxies need that InitializeRMIs is 
  // called before RenderWindow is set.
  
  pp = vtkSMProxyProperty::SafeDownCast(
    this->RenderSyncManagerProxy->GetProperty("RenderWindow"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property RenderWindow on "
                  "RenderSyncManagerProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RenderWindowProxy);
  
  // Update the server process so that the render window is set before
  // we initialize offscreen rendering.
  this->RenderSyncManagerProxy->UpdateVTKObjects();
  
  if (getenv("PV_DISABLE_COMPOSITE_INTERRUPTS"))
    {
    p = this->RenderSyncManagerProxy->GetProperty("EnableAbort");
    // Does anything support EnableAbort right now?
    if (p)
      {
      p->Modified();
      }
    }
  
  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    int enableOffscreen = 1;
    
    // Non-mesa, X offscreen rendering requires access to the display
    vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
    pm->GatherInformation(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
    if (!di->GetCanOpenDisplay())
      {
      enableOffscreen = 0;
      }
    di->Delete();

    if (enableOffscreen)
      {
      p = this->RenderSyncManagerProxy->GetProperty("InitializeOffScreen");
      if (!p)
        {
        vtkErrorMacro("Failed to find property InitializeOffScreen on "
                      "RenderSyncManagerProxy.");
        return;
        }
      p->Modified();
      }
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManagerProxy->GetProperty("UseCompositing"));
  if (ivp)
    {
    // So that the server window does not popup until needed.
    ivp->SetElement(0, 0); 
    }
  
  this->RenderSyncManagerProxy->UpdateVTKObjects();

    {
    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID(0)
           << "AddRenderer" << this->RenderModuleId 
           << this->RendererProxy->GetID(0) << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID(0)
           << "AddRenderer" << this->RendererProxy->GetID(0)
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID(0)
           << "SetId" << this->RenderModuleId 
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::PassCollectionDecisionToDisplays(
  int collectionDecision, bool use_lod)
{
  vtkCollection* displays = this->GetDisplays();
  displays->InitTraversal();
  vtkObject* object;
  while ( (object = displays->GetNextItemAsObject()) )
    {
    vtkSMCompositeDisplayProxy* pDisp = vtkSMCompositeDisplayProxy::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibilityCM())
      {
      if (!use_lod)
        {
        this->SetCollectionDecision(pDisp, collectionDecision);
        }
      else
        {
        this->SetLODCollectionDecision(pDisp, collectionDecision);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::StillRender()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);

  // This ensures that we use the update data sizes for collection decision.
  this->UpdateAllDisplays();

  // Find out whether we are going to render localy.
  // Save this so we know where to get the z buffer (for picking?).
  this->LocalRender = this->GetLocalRenderDecision(
    this->GetTotalVisibleGeometryMemorySize(), 1);

  // Change the collection flags and update.
  this->PassCollectionDecisionToDisplays(this->LocalRender, false);

  // We don't need to call UpdateAllDisplays explicitly, since
  // Superclass::StillRender() will call UpdateAllDisplays because
  // SetCollectionDecision invalidates geometry.
  // this->UpdateAllDisplays();

  //Turn of ImageReductionFactor if the RenderSyncManager supports it.
  if (this->RenderSyncManagerProxy)
    {
    if ( ! this->IsA("vtkSMIceTRenderModuleProxy") )
      {
      this->SetImageReductionFactor(this->RenderSyncManagerProxy, 1);
      }
    this->SetSquirtLevel(this->RenderSyncManagerProxy,
                         ((this->SquirtLevel)? 1 : 0) );
    this->SetUseCompositing(this->RenderSyncManagerProxy,
                            ((this->LocalRender)? 0 : 1));
    }
  
  this->Superclass::StillRender();

  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::InteractiveRender()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);

  this->UpdateAllDisplays();

  int useLOD = this->GetUseLODDecision();
  unsigned long totalMemory = 0;
  totalMemory = (useLOD)? this->GetTotalVisibleLODGeometryMemorySize() :
    this->GetTotalVisibleGeometryMemorySize();

  this->LocalRender = this->GetLocalRenderDecision(totalMemory, 0);

  // Change the collection flags and update.
  this->PassCollectionDecisionToDisplays(this->LocalRender, useLOD);
  if (this->RenderSyncManagerProxy)
    {
    // Set Squirt Level (if supported).
    this->SetSquirtLevel(this->RenderSyncManagerProxy, this->SquirtLevel );
    this->SetUseCompositing(this->RenderSyncManagerProxy,
                            ((this->LocalRender)? 0 : 1));
    }
  
  if (!this->LocalRender)
    {
    this->GetRenderWindow()->SetDesiredUpdateRate(5.0);
    this->ComputeReductionFactor(this->ReductionFactor);
    }

  this->Superclass::InteractiveRender();
  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//-----------------------------------------------------------------------------
double vtkSMClientServerRenderModuleProxy::GetZBufferValue(int x, int y)
{
  if (this->LocalRender)
    {
    return this->Superclass::GetZBufferValue(x,y);
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!this->RenderSyncManagerProxy)
    {
    vtkErrorMacro("RenderSyncManagerProxy not defined!");
    return 0;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RenderSyncManagerProxy->GetID(0) 
         << "GetZBufferValue" << x << y
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
  float z = 0;
  if(pm->GetLastResult(this->ConnectionID,
                       vtkProcessModule::CLIENT).GetArgument(0, 0, &z))
    {
    return z;
    }
  else
    {
    vtkErrorMacro("Error getting float value from GetZBufferValue result.");
    }

  vtkErrorMacro("Unknown RenderModule mode.");
  return 0;
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMClientServerRenderModuleProxy::GetRenderingProgressServers()
{
  if (this->LocalRender)
    {
    return vtkProcessModule::CLIENT;
    }
  return this->Superclass::GetRenderingProgressServers();
}

//-----------------------------------------------------------------------------
int vtkSMClientServerRenderModuleProxy::IsRenderLocal()
{
  return this->GetLocalRenderDecision(
                                  this->GetTotalVisibleGeometryMemorySize(), 1);
}

//-----------------------------------------------------------------------------
int vtkSMClientServerRenderModuleProxy::GetLocalRenderDecision(
                                                    unsigned long totalMemory,
                                                    int vtkNotUsed(stillRender))
{
  if (static_cast<float>(totalMemory)/1000.0 < this->GetRemoteRenderThreshold())
    {
    return 1; // Local render.
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::SetGUISize(int x, int y)
{
  if (this->RenderSyncManagerProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID(0)
           << "SetGUISize" << x << y
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
    }
  this->Superclass::SetGUISize(x, y);
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::SetWindowPosition(int x, int y)
{
  if (this->RenderSyncManagerProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID(0)
           << "SetWindowPosition" << x << y
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
    }
  this->Superclass::SetWindowPosition(x, y);
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::SetUseCompositing(vtkSMProxy* p,
                                                           int flag)
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
void vtkSMClientServerRenderModuleProxy::SetSquirtLevel(vtkSMProxy* p,
                                                        int level)
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
void vtkSMClientServerRenderModuleProxy::SetImageReductionFactor(vtkSMProxy* p,
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
void vtkSMClientServerRenderModuleProxy::ComputeReductionFactor(
                                                          int inReductionFactor)
{
  if (this->RenderSyncManagerProxy)
    {
    this->SetImageReductionFactor(this->RenderSyncManagerProxy,
                                  inReductionFactor);
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::SetCollectionDecision(
                                              vtkSMCompositeDisplayProxy* pDisp,
                                              int decision)
{
  // We don't use properties since it slows us down considerably.
  pDisp->SetCollectionDecision(decision);
  
}

//-----------------------------------------------------------------------------
void vtkSMClientServerRenderModuleProxy::SetLODCollectionDecision(
                                              vtkSMCompositeDisplayProxy* pDisp,
                                              int decision)
{
  // We don't use properties since it slows us down considerably.
  pDisp->SetLODCollectionDecision(decision);
}
