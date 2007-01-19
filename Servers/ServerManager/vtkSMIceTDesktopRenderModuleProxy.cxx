/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTDesktopRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTDesktopRenderModuleProxy.h"

#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMCompositeDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"

#include <vtkstd/set>

vtkStandardNewMacro(vtkSMIceTDesktopRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMIceTDesktopRenderModuleProxy, "1.25");

vtkCxxSetObjectMacro(vtkSMIceTDesktopRenderModuleProxy, 
                     ServerRenderWindowProxy,
                     vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMIceTDesktopRenderModuleProxy, 
                     ServerCompositeManagerProxy,
                     vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMIceTDesktopRenderModuleProxy, 
                     ServerDisplayManagerProxy,
                     vtkSMProxy);

//-----------------------------------------------------------------------------
class vtkSMIceTDesktopRenderModuleProxyProxySet
  : public vtkstd::set<vtkSMProxy *> { };

//-----------------------------------------------------------------------------
vtkSMIceTDesktopRenderModuleProxy::vtkSMIceTDesktopRenderModuleProxy()
{
  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->TileMullions[0] = this->TileMullions[1] = 1;
  this->RemoteDisplay = 1;
  this->DisableOrderedCompositing = 0;
  this->OrderedCompositing = 0;

  this->DisplayManagerProxy = 0;
  this->PKdTreeProxy = 0;
  this->PKdTreeGeneratorProxy = 0;

  this->ServerRenderWindowProxy = 0;
  this->ServerCompositeManagerProxy = 0;
  this->ServerDisplayManagerProxy = 0;

  this->RenderModuleId = 0;
  this->UsingCustomKdTree = 0;

  this->PartitionedData = new vtkSMIceTDesktopRenderModuleProxyProxySet;
}

//-----------------------------------------------------------------------------
vtkSMIceTDesktopRenderModuleProxy::~vtkSMIceTDesktopRenderModuleProxy()
{
  this->SetServerRenderWindowProxy(0);
  this->SetServerCompositeManagerProxy(0);
  this->SetServerDisplayManagerProxy(0);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::SetGUISize(int x, int y)
{
  if (this->CompositeManagerProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(0)
           << "SetGUISize" << x << y
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
    }
  this->Superclass::SetGUISize(x, y);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::SetWindowPosition(int x, int y)
{
  if (this->CompositeManagerProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(0)
           << "SetWindowPosition" << x << y
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
    }
  this->Superclass::SetWindowPosition(x, y);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->RendererProxy = this->GetSubProxy("Renderer");
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");
  this->DisplayManagerProxy = this->GetSubProxy("DisplayManager");
  this->PKdTreeProxy = this->GetSubProxy("PKdTree");
  this->PKdTreeGeneratorProxy = this->GetSubProxy("PKdTreeGenerator");

  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined.");
    return;
    }

  if (!this->RendererProxy)
    {
    vtkErrorMacro("Renderer subproxy must be defined.");
    return;
    }

  if (!this->DisplayManagerProxy)
    {
    vtkErrorMacro("DisplayManager subproxy must be defined.");
    return;
    }

  if (!this->PKdTreeProxy)
    {
    vtkErrorMacro("PKdTree subproxy must be defined.");
    return;
    }

  if (!this->PKdTreeGeneratorProxy)
    {
    vtkErrorMacro("PKdTreeGenerator subproxy must be defined.");
    return;
    }


  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  this->DisplayManagerProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  if (this->ServerDisplayManagerProxy)
    {
    vtkClientServerID id = pm->GetUniqueID();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Assign 
           << id
           << this->ServerDisplayManagerProxy->GetID(0)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    this->DisplayManagerProxy->CreateVTKObjects(0);
    this->DisplayManagerProxy->SetID(0, id);
    }
  this->DisplayManagerProxy->UpdateVTKObjects();

  this->PKdTreeProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  this->PKdTreeGeneratorProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->PKdTreeGeneratorProxy->GetProperty("KdTree"));
  pp->AddProxy(this->PKdTreeProxy);
  this->PKdTreeGeneratorProxy->UpdateVTKObjects();

  // Allow a minimum number of cells in case we break up small data.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                                   this->PKdTreeProxy->GetProperty("MinCells"));
  ivp->SetElements1(0);
  this->PKdTreeProxy->UpdateVTKObjects();

  // We must create an ICE-T Renderer on the server and a regular renderer
  // on the client.
  this->RendererProxy->SetServers(vtkProcessModule::CLIENT);
  this->RendererProxy->UpdateVTKObjects(); // this will create the regular
                                           // renderer on the client.

  vtkClientServerStream stream1;
  stream1 << vtkClientServerStream::New 
          << "vtkIceTRenderer" << this->RendererProxy->GetID(0)
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream1);

  this->RendererProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  // Now we can use the RendererProxy as one!

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

  // Anti-aliasing screws up the compositing.  Turn it off.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
    (pm->GetNumberOfPartitions(this->ConnectionID) > 1))
    {
    stream1 << vtkClientServerStream::Invoke
            << this->RenderWindowProxy->GetID(0) << "SetMultiSamples" << 0
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream1);
    }

  // Ordered compositing requires alpha bit planes.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
                        this->RenderWindowProxy->GetProperty("AlphaBitPlanes"));
  ivp->SetElements1(1);

  this->RenderWindowProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::CreateCompositeManager()
{
  this->CompositeManagerProxy = this->GetSubProxy("CompositeManager");
  if (!this->CompositeManagerProxy)
    {
    vtkErrorMacro("CompositeManager subproxy must be defined.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  // Similar to the renderer, we create our peculiar CompositeManager.
  this->CompositeManagerProxy->SetServers(vtkProcessModule::CLIENT);
  // The XML must define only the manager to be created on the client.

  this->CompositeManagerProxy->UpdateVTKObjects();
  // If there is a server side composite manager, use it. This is used in
  // multi-view mode where the server has one desktop delivery server
  // whereas the client can have many desktop delivery clients.
  if (this->ServerCompositeManagerProxy)
    {
    stream << vtkClientServerStream::Assign 
           << this->CompositeManagerProxy->GetID(0)
           << this->ServerCompositeManagerProxy->GetID(0)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);
    }
  else
    {
    // Create the vtkDesktopDeliveryServer.
    stream << vtkClientServerStream::New << "vtkPVDesktopDeliveryServer"
           << this->CompositeManagerProxy->GetID(0)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER_ROOT, stream);
    }
    
  this->CompositeManagerProxy->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER_ROOT);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::InitializeCompositingPipeline()
{
  vtkSMIntVectorProperty* ivp;
  vtkSMProxyProperty* pp;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // Cannot do this with the server manager because this method only
  // exists on the render server, not the client.
  stream << vtkClientServerStream::Invoke << this->RendererProxy->GetID(0)
         << "SetSortingKdTree" << this->PKdTreeProxy->GetID(0)
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DisplayManagerProxy->GetProperty("TileDimensions"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property TileDimensions on DisplayManagerProxy.");
    return;
    }
  ivp->SetElements(this->TileDimensions);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DisplayManagerProxy->GetProperty("TileMullions"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property TileMullions on DisplayManagerProxy.");
    return;
    }
  ivp->SetElements(this->TileMullions);
  this->DisplayManagerProxy->UpdateVTKObjects(); 
  // Tile Dimensions must be set before the RenderWindow is set,
  // hence the call to DisplayManagerProxy->UpdateVTKObjects()  is required.
  // I know, calling UpdateVTKObjects so many times is waaaay expensive,
  // but hey! This is called only during creating.
  
  pp = vtkSMProxyProperty::SafeDownCast(
    this->DisplayManagerProxy->GetProperty("RenderWindow"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property RenderWindow on DisplayManagerProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RenderWindowProxy);
  this->DisplayManagerProxy->UpdateVTKObjects(); 

  unsigned int i;

  if (this->RenderModuleId == 0)
    {
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID()
           << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DisplayManagerProxy->GetID(0) << "SetController"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DisplayManagerProxy->GetID(0) 
           << "InitializeRMIs"
           << vtkClientServerStream::End;
    }
    
  for (i = 0; i < this->PKdTreeProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;

    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->PKdTreeProxy->GetID(i) << "SetController"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult << "GetNumberOfProcesses"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->PKdTreeProxy->GetID(i) << "SetNumberOfRegionsOrMore"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    // Set up logging for building kd-tree.
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent"
        << "Build kd-tree" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->PKdTreeProxy->GetID(i) << "AddObserver"
           << "StartEvent" << cmd << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent"
        << "Build kd-tree" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->PKdTreeProxy->GetID(i) << "AddObserver"
           << "EndEvent" << cmd << vtkClientServerStream::End;
    }

  for (i=0; i < this->PKdTreeGeneratorProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult << "GetNumberOfProcesses"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->PKdTreeGeneratorProxy->GetID(i) << "SetNumberOfPieces"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream);

  //************************************************************
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
         << "GetRenderServerSocketController"
         << pm->GetConnectionClientServerID(this->ConnectionID)
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << this->CompositeManagerProxy->GetID(0)
         << "SetController" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    this->CompositeManagerProxy->GetServers(), stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
         this->CompositeManagerProxy->GetProperty("SyncRenderWindowRenderers"));
  if (!ivp)
    {
    vtkErrorMacro("Falied to find property SyncRenderWindowRenderers");
    return;
    }
  ivp->SetElement(0, 0);

  pp = vtkSMProxyProperty::SafeDownCast(
                         this->CompositeManagerProxy->GetProperty("Renderers"));
  if (this->RenderModuleId == 0)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->RendererProxy);
    //pp->AddProxy(this->Renderer2DProxy);
    this->CompositeManagerProxy->UpdateVTKObjects();
    }
  else
    {
    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(0)
           << "AddRenderer" << this->RenderModuleId 
           << this->RendererProxy->GetID(0) << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(0)
           << "AddRenderer" << this->RendererProxy->GetID(0)
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(0)
           << "SetId" << this->RenderModuleId 
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
    }

  this->Superclass::InitializeCompositingPipeline();
  
  
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    // The client server manager needs to set parameters on the IceT manager.
    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(i)
           << "SetParallelRenderManager" 
           << this->DisplayManagerProxy->GetID(i)
           << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke 
           << this->CompositeManagerProxy->GetID(i)
           << "SetRemoteDisplay" 
           << this->RemoteDisplay 
           << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

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
      vtkSMProperty* p = 
        this->DisplayManagerProxy->GetProperty("InitializeOffScreen");
      if (!p)
        {
        vtkErrorMacro("Failed to find property InitializeOffScreen "
                      "on CompositeManagerProxy.");
        return;
        }
      p->Modified();
      this->DisplayManagerProxy->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::SetOrderedCompositing(int flag)
{
  if (this->OrderedCompositing == flag) 
    {
    return;
    }

  this->OrderedCompositing = flag;

  vtkObject *obj;
  vtkCollection* displays = this->GetDisplays();
  displays->InitTraversal();
  for (obj = displays->GetNextItemAsObject(); obj != NULL;
       obj = displays->GetNextItemAsObject())
    {
    vtkSMCompositeDisplayProxy *disp = 
      vtkSMCompositeDisplayProxy::SafeDownCast(obj);
    if (disp)
      {
      disp->SetOrderedCompositing(this->OrderedCompositing);
      }
    }

  if (this->OrderedCompositing)
    {
    // Cannot do this with the server manager because this method only
    // exists on the render server, not the client.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->RendererProxy->GetID(0)
           << "SetComposeOperation" << 1 // Over
           << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
                   this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    }
  else
    {
    // Cannot do this with the server manager because this method only
    // exists on the render server, not the client.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->RendererProxy->GetID(0)
           << "SetComposeOperation" << 0 // Closest
           << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
                   this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::AddDisplay(
  vtkSMAbstractDisplayProxy *disp)
{
  this->Superclass::AddDisplay(disp);

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    disp->GetProperty("OrderedCompositingTree"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->PKdTreeProxy);
    disp->UpdateProperty("OrderedCompositingTree");
    }

}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::RemoveDisplay(
  vtkSMAbstractDisplayProxy* disp)
{
  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    disp->GetProperty("OrderedCompositingTree"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(0);
    disp->UpdateProperty("OrderedCompositingTree");
    }

  this->Superclass::RemoveDisplay(disp);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::StillRender()
{
  int orderedCompositingNeeded = 0;
  bool new_dataset = false;
  if (!this->DisableOrderedCompositing)
    {
    // Update the PKdTree, but only if there is something to divide that has
    // not been divided the last time or some geometry has become invalid.
    int doBuildLocator = 0;

    // Check to see if there is anything new added to the k-d tree.
    vtkSMProxyProperty *toPartition = vtkSMProxyProperty::SafeDownCast(
      this->PKdTreeProxy->GetProperty("DataSets"));
    unsigned int i, numProxies;
    numProxies = toPartition->GetNumberOfProxies();
    for (i = 0; i < numProxies; i++)
      {
      if (this->PartitionedData->find(toPartition->GetProxy(i)) == 
          this->PartitionedData->end() )
        {
        doBuildLocator = 1;
        new_dataset = true;
        break;
        }
      }


    // Check to see if the geometry of any visible objects has changed.
    vtkObject *obj;
    vtkCollectionSimpleIterator cookie;
    vtkCollection* displays = this->GetDisplays();
    displays->InitTraversal(cookie);
    for (obj = displays->GetNextItemAsObject(cookie); obj != NULL;
         obj = displays->GetNextItemAsObject(cookie))
      {
      vtkSMCompositeDisplayProxy *disp = 
        vtkSMCompositeDisplayProxy::SafeDownCast(obj);
      if (disp && disp->GetVisibilityCM())
        {
        // If any of the displays is volume rendering, we need to
        // turn on ordered compositing.
        if (!orderedCompositingNeeded)
          {
          if (disp->GetVolumeRenderMode())
            {
            orderedCompositingNeeded = 1;
            }
          else
            {
            vtkSMDoubleVectorProperty* opacity = 
              vtkSMDoubleVectorProperty::SafeDownCast(
                disp->GetProperty("Opacity"));
            if (opacity && opacity->GetElement(0) < 1.0)
              {
              orderedCompositingNeeded = 1;
              }
            }
          }

        if (!disp->IsDistributedGeometryValid())
          {
          doBuildLocator = 1;
          }
        }

      if (doBuildLocator && orderedCompositingNeeded)
        {
        break;
        }
      }

    if (orderedCompositingNeeded)
      {
      if (doBuildLocator)
        {
        // Update PartitionedData ivar.
        this->PartitionedData->erase(this->PartitionedData->begin(),
                                     this->PartitionedData->end());
        for (i = 0; i < numProxies; i++)
          {
          this->PartitionedData->insert(toPartition->GetProxy(i));
          }
        
        // For all visibile displays, make sure their geometry is up to date
        // for the k-d tree and make sure the distribution gets updated after
        // the tree is reformed.
        // At the same time, we will also check if any display is
        // volume rendering structured data. If so, we need to 
        // generate the k-d tree using the structured data's distribution.
        // Currently, at most one display can volume render structured
        // data.
        displays = this->GetDisplays();
        displays->InitTraversal(cookie);
        int self_generate_kdtree = 0;
        for (obj = displays->GetNextItemAsObject(cookie); obj != NULL;
          obj = displays->GetNextItemAsObject(cookie))
          {
          vtkSMCompositeDisplayProxy *disp = 
            vtkSMCompositeDisplayProxy::SafeDownCast(obj);
          if (disp && disp->GetVisibilityCM())
            {
            disp->Update(this);
            disp->InvalidateDistributedGeometry();

            if (!self_generate_kdtree &&
              disp->GetVolumeRenderMode() && disp->GetVolumePipelineType() 
              == vtkSMDataObjectDisplayProxy::IMAGE_DATA)
              {
              // We are volume rendering structured data. We need to build 
              // the k-d tree using the data distribution. 
              self_generate_kdtree = 1;
              disp->BuildKdTreeUsingDataPartitions(this->PKdTreeGeneratorProxy);
              }
            }
          }

        vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
        if (!self_generate_kdtree && this->UsingCustomKdTree)
          {
          // we need to ensure that the PKdTreeProxy no longer
          // uses the user-defined cuts. Settings the cuts to NULL
          // ensures that. 
          vtkClientServerStream stream;
          stream << vtkClientServerStream::Invoke
            << this->PKdTreeProxy->GetID(0)
            << "SetCuts" << 0
            << vtkClientServerStream::End;
          pm->SendStream(this->PKdTreeProxy->GetConnectionID(),
            this->PKdTreeProxy->GetServers(), stream);
          }
        this->UsingCustomKdTree = self_generate_kdtree;
        
        // Build the global k-d tree.
        pm->SendPrepareProgress(this->GetConnectionID());
        this->PKdTreeProxy->InvokeCommand("BuildLocator");
        pm->SendCleanupPendingProgress(this->GetConnectionID());
        }
      }
    }

  if (new_dataset && this->OrderedCompositing && orderedCompositingNeeded)
    {
    // SetOrderedCompositing has no effect if OrderedCompositing is value
    // is unchanged. However, a new display was added and it's 
    // OrderedCompositing flag hasn;t been synchronized with that
    // of the render module. Clearing the value will ensure
    // that SetOrderedCompositing() will update all displays.
    this->OrderedCompositing = 0;
    }

  this->SetOrderedCompositing(orderedCompositingNeeded);

  this->Superclass::StillRender();
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TileDimensions: " << this->TileDimensions[0] 
    << ", " << this->TileDimensions[1] << endl;
  os << indent << "RemoteDisplay: " << this->RemoteDisplay 
    << endl;
  os << indent << "DisableOrderedCompositing: " 
     << this->DisableOrderedCompositing << endl;

  os << indent << "DisplayManagerProxy: ";
  if (this->DisplayManagerProxy)
    {
    this->DisplayManagerProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "PKdTreeProxy: ";
  if (this->PKdTreeProxy)
    {
    this->PKdTreeProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ServerCompositeManagerProxy: ";
  if (this->ServerCompositeManagerProxy)
    {
    this->ServerCompositeManagerProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ServerDisplayManagerProxy: ";
  if (this->ServerDisplayManagerProxy)
    {
    this->ServerDisplayManagerProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ServerRenderWindowProxy: ";
  if (this->ServerRenderWindowProxy)
    {
    this->ServerRenderWindowProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "RenderModuleId: " << this->RenderModuleId << endl;
}

