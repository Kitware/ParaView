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
#include "vtkPVClientServerModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"

#include <vtkstd/set>

vtkStandardNewMacro(vtkSMIceTDesktopRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMIceTDesktopRenderModuleProxy, "1.6.2.1");

//-----------------------------------------------------------------------------
class vtkSMIceTDesktopRenderModuleProxyProxySet
  : public vtkstd::set<vtkSMProxy *> { };

//-----------------------------------------------------------------------------
vtkSMIceTDesktopRenderModuleProxy::vtkSMIceTDesktopRenderModuleProxy()
{
  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->RemoteDisplay = 1;
  this->OrderedCompositing = 0;

  this->DisplayManagerProxy = NULL;
  this->PKdTreeProxy = NULL;

  this->PartitionedData = new vtkSMIceTDesktopRenderModuleProxyProxySet;
}

//-----------------------------------------------------------------------------
vtkSMIceTDesktopRenderModuleProxy::~vtkSMIceTDesktopRenderModuleProxy()
{
  delete this->PartitionedData;
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->RendererProxy = this->GetSubProxy("Renderer");
  this->DisplayManagerProxy = this->GetSubProxy("DisplayManager");
  this->PKdTreeProxy = this->GetSubProxy("PKdTree");

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


  this->DisplayManagerProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  this->DisplayManagerProxy->UpdateVTKObjects();

  this->PKdTreeProxy->SetServers(vtkProcessModule::RENDER_SERVER);

  // Allow a minimum number of cells in case we break up small data.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                                   this->PKdTreeProxy->GetProperty("MinCells"));
  ivp->SetElements1(0);
  this->PKdTreeProxy->UpdateVTKObjects();

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  // We must create an ICE-T Renderer on the server and a regular renderer on the
  // client.
  this->RendererProxy->SetServers(vtkProcessModule::CLIENT);
  this->RendererProxy->UpdateVTKObjects(); // this will create the regular renderer on the client.

  vtkClientServerStream stream;
  stream << vtkClientServerStream::New << "vtkIceTRenderer" << this->RendererProxy->GetID(0)
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);

  this->RendererProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  // Now we can use the RendererProxy as one!

  
  this->Superclass::CreateVTKObjects(numObjects);

  // Anti-aliasing screws up the compositing.  Turn it off.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
    (pm->GetNumberOfPartitions() > 1))
    {
    stream << vtkClientServerStream::Invoke
      << this->RenderWindowProxy->GetID(0) << "SetMultiSamples" << 0
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
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

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  vtkClientServerStream stream;
  // Similar to the renderer, we create our peculiar CompositeManager.
  this->CompositeManagerProxy->SetServers(vtkProcessModule::CLIENT);
  // The XML must define only the manager to be created on the client.

  this->CompositeManagerProxy->UpdateVTKObjects();
  // Create the vtkDesktopDeliveryServer.
  stream << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
    << this->CompositeManagerProxy->GetID(0)
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);

  this->CompositeManagerProxy->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER_ROOT);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::InitializeCompositingPipeline()
{
  vtkSMIntVectorProperty* ivp;
  vtkSMProxyProperty* pp;

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  vtkClientServerStream stream;

  // Cannot do this with the server manager because this method only
  // exists on the render server, not the client.
  stream << vtkClientServerStream::Invoke << this->RendererProxy->GetID(0)
         << "SetSortingKdTree" << this->PKdTreeProxy->GetID(0)
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DisplayManagerProxy->GetProperty("TileDimensions"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property TileDimensions on DisplayManagerProxy.");
    return;
    }
  ivp->SetElements(this->TileDimensions);
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

  for (i=0; i < this->DisplayManagerProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetController"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->DisplayManagerProxy->GetID(i) << "SetController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->DisplayManagerProxy->GetID(i) 
      << "InitializeRMIs"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
    }

  for (i = 0; i < this->PKdTreeProxy->GetNumberOfIDs(); i++)
    {
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
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
    }

  //************************************************************
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
      << "GetRenderServerSocketController"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
      << this->CompositeManagerProxy->GetID(i)
      << "SetController" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::RENDER_SERVER_ROOT | vtkProcessModule::CLIENT, stream);

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
  pp->RemoveAllProxies();
  pp->AddProxy(this->RendererProxy);
  pp->AddProxy(this->Renderer2DProxy);

  this->CompositeManagerProxy->UpdateVTKObjects();
  
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
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);

  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    int enableOffscreen = 1;

    // Non-mesa, X offscreen rendering requires access to the display
    vtkPVClientServerModule* csm = 
      vtkPVClientServerModule::SafeDownCast(pm);
    if (csm)
      {
      vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
      csm->GatherInformationRenderServer(di, csm->GetProcessModuleID());
      if (!di->GetCanOpenDisplay())
        {
        enableOffscreen = 0;
        }
      di->Delete();
      }
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
  if (this->OrderedCompositing == flag) return;

  this->OrderedCompositing = flag;

  vtkObject *obj;
  this->Displays->InitTraversal();
  for (obj = this->Displays->GetNextItemAsObject(); obj != NULL;
       obj = this->Displays->GetNextItemAsObject())
    {
    vtkSMDisplayProxy *disp = vtkSMDisplayProxy::SafeDownCast(obj);
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                                       disp->GetProperty("OrderedCompositing"));
    if (ivp)
      {
      ivp->SetElements1(this->OrderedCompositing);
      disp->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::AddDisplay(vtkSMDisplayProxy *disp)
{
  this->Superclass::AddDisplay(disp);

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
                                   disp->GetProperty("OrderedCompositingTree"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->PKdTreeProxy);
    }

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                                       disp->GetProperty("OrderedCompositing"));
  if (ivp)
    {
    ivp->SetElements1(this->OrderedCompositing);
    }

  disp->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::StillRender()
{
  if (this->OrderedCompositing)
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
      if (   this->PartitionedData->find(toPartition->GetProxy(i))
          == this->PartitionedData->end() )
        {
        doBuildLocator = 1;
        break;
        }
      }

    // Check to see if the geometry of any visible objects has changed.
    vtkObject *obj;
    vtkCollectionSimpleIterator cookie;
    this->Displays->InitTraversal(cookie);
    for (obj = this->Displays->GetNextItemAsObject(cookie); obj != NULL;
         obj = this->Displays->GetNextItemAsObject(cookie))
      {
      vtkSMDisplayProxy *disp = vtkSMDisplayProxy::SafeDownCast(obj);
      if (disp && disp->GetVisibilityCM())
        {
        vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                               disp->GetProperty("IsDistributedGeometryValid"));
        if (ivp)
          {
          disp->UpdatePropertyInformation(ivp);
          if (!ivp->GetElement(0))
            {
            doBuildLocator = 1;
            break;
            }
          }
        }
      }

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
      this->Displays->InitTraversal(cookie);
      for (obj = this->Displays->GetNextItemAsObject(cookie); obj != NULL;
           obj = this->Displays->GetNextItemAsObject(cookie))
        {
        vtkSMDisplayProxy *disp = vtkSMDisplayProxy::SafeDownCast(obj);
        if (disp && disp->GetVisibilityCM())
          {
          vtkSMProperty *p = disp->GetProperty("Update");
          if (p)
            {
            p->Modified();
            }
          p = disp->GetProperty("InvalidateDistributedGeometry");
          if (p)
            {
            p->Modified();
            }
          disp->UpdateVTKObjects();
          }
        }

      // Build the global k-d tree.
      vtkSMProperty *p = this->PKdTreeProxy->GetProperty("BuildLocator");
      p->Modified();
      this->PKdTreeProxy->UpdateVTKObjects();

      // Cannot do this with the server manager because this method only
      // exists on the render server, not the client.
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << this->RendererProxy->GetID(0)
             << "SetComposeOperation" << 1 // Over
             << vtkClientServerStream::End;
      vtkPVProcessModule::GetProcessModule()->SendStream(
                                       vtkProcessModule::RENDER_SERVER, stream);
      }
    }
  else
    {
    // Cannot do this with the server manager because this method only
    // exists on the render server, not the client.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->RendererProxy->GetID(0)
           << "SetComposeOperation" << 0 // Closest
           << vtkClientServerStream::End;
    vtkPVProcessModule::GetProcessModule()->SendStream(
                                       vtkProcessModule::RENDER_SERVER, stream);
    }

  this->Superclass::StillRender();
}
//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::UpdateAllDisplays()
{
  if (this->OrderedCompositing)
    {
    // Make sure the distribution is up to date.
    vtkObject *obj;
    vtkCollectionSimpleIterator cookie;
    this->Displays->InitTraversal(cookie);
    for (obj = this->Displays->GetNextItemAsObject(cookie); obj != NULL;
         obj = this->Displays->GetNextItemAsObject(cookie))
      {
      vtkSMDisplayProxy *disp = vtkSMDisplayProxy::SafeDownCast(obj);
      if (disp && disp->GetVisibilityCM())
        {
        vtkSMProperty *p = disp->GetProperty("UpdateDistributedGeometry");
        if (p)
          {
          p->Modified();
          }
        disp->UpdateVTKObjects();
        }
      }
    }

  this->Superclass::UpdateAllDisplays();
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TileDimensions: " << this->TileDimensions[0] 
    << ", " << this->TileDimensions[1] << endl;
  os << indent << "RemoteDisplay: " << this->RemoteDisplay 
    << endl;
  os << indent << "OrderedCompositing: " << this->OrderedCompositing << endl;
  os << indent << "DisplayManagerProxy: " << this->DisplayManagerProxy
    <<endl;
  os << indent << "PKdTreeProxy: " << this->PKdTreeProxy << endl;
}

