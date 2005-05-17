/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMRenderModuleProxy.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVProcessModule.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVOptions.h"
#include "vtkCommand.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkClientServerID.h"
#include "vtkTimerLog.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkClientServerStream.h"
#include "vtkCallbackCommand.h"
#include "vtkPVGeometryInformation.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkSMPropertyIterator.h"
#include "vtkWindowToImageFilter.h"
#include "vtkImageWriter.h"
#include "vtkInstantiator.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkErrorCode.h"

vtkCxxRevisionMacro(vtkSMRenderModuleProxy, "1.9");
//-----------------------------------------------------------------------------
// This is a bit of a pain.  I do ResetCameraClippingRange as a call back
// because the PVInteractorStyles call ResetCameraClippingRange 
// directly on the renderer.  Since they are PV styles, I might
// have them call the render module directly like they do for render.
void vtkSMRenderModuleResetCameraClippingRange(
 vtkObject *, unsigned long vtkNotUsed(event),void *clientData, void *)
{
  vtkSMRenderModuleProxy* self = (vtkSMRenderModuleProxy*)clientData;
  if(self)
    {
    self->ResetCameraClippingRange();
    }
}
//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxyAbortCheck(
  vtkObject*, unsigned long, void* arg, void*)
{
  vtkSMRenderModuleProxy* self = (vtkSMRenderModuleProxy*) arg;
  if (self && self->GetRenderInterruptsEnabled())
    {
    self->InvokeEvent(vtkCommand::AbortCheckEvent, NULL);
    }
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy::vtkSMRenderModuleProxy()
{
  // All the subproxies are created on Client and Render Server.
  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->LightKitProxy = 0;
  this->Displays = vtkCollection::New();
  this->RendererProps = vtkCollection::New();
  this->Renderer2DProps = vtkCollection::New();
  this->DisplayXMLName = 0;
  this->ResetCameraClippingRangeTag = 0;
  this->AbortCheckTag = 0;
  this->RenderInterruptsEnabled = 1;

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy::~vtkSMRenderModuleProxy()
{
  if (this->ResetCameraClippingRangeTag)
    {
    vtkRenderer* ren = this->GetRenderer();
    ren->RemoveObserver(this->ResetCameraClippingRangeTag);
    this->ResetCameraClippingRangeTag = 0;
    }
  if (this->AbortCheckTag)
    {
    this->GetRenderWindow()->RemoveObserver(this->AbortCheckTag);
    this->AbortCheckTag = 0;
    }
  this->Displays->Delete();
  this->RendererProps->Delete();
  this->Renderer2DProps->Delete();
  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->LightKitProxy = 0;
  this->SetDisplayXMLName(0);

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->RendererProxy = this->GetSubProxy("Renderer");
  this->Renderer2DProxy = this->GetSubProxy("Renderer2D");
  this->ActiveCameraProxy = this->GetSubProxy("ActiveCamera");
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");
  this->InteractorProxy = this->GetSubProxy("Interactor");
  this->LightKitProxy = this->GetSubProxy("LightKit");

  if (!this->RendererProxy)
    {
    vtkErrorMacro("Renderer subproxy must be defined in the "
                  "configuration file.");
    return;
    }
  if (!this->Renderer2DProxy)
    {
    vtkErrorMacro("Renderer2D subproxy must be defined in the "
                  "configuration file.");
    return;
    }
  if (!this->ActiveCameraProxy)
    {
    vtkErrorMacro("ActiveCamera subproxy must be defined in the "
                  "configuration file.");
    return;
    }
  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined in the configuration "
                  "file.");
    return;
    }
  if (!this->InteractorProxy)
    {
    vtkErrorMacro("Interactor subproxy must be defined in the configuration "
                  "file.");
    return;
    }
  if (!this->LightKitProxy)
    {
    vtkErrorMacro("LightKit subproxy must be defined in the configuration "
                  "file.");
    }
    

  // I don't directly use this->SetServers() to set the servers of the
  // subproxies, as the subclasses may have special subproxies that have
  // specific servers on which they want those to be created.
  this->SetServersSelf(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->RendererProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Renderer2DProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER); 
  // Camera vtkObject is only created on the client.  This is so as we
  // don't change the active camera on the servers as creation renderer
  // (like IceT Tile renderer) fail if the active camera on the renderer is
  // modified.
  this->ActiveCameraProxy->SetServers(vtkProcessModule::CLIENT);
  this->RenderWindowProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->InteractorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LightKitProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects(numObjects);

  vtkPVProcessModule* pvm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  
  // Set all the client side pointers.
  this->Renderer2D = vtkRenderer::SafeDownCast(
    pvm->GetObjectFromID(this->Renderer2DProxy->GetID(0)));
  this->Renderer = vtkRenderer::SafeDownCast(
    pvm->GetObjectFromID(this->RendererProxy->GetID(0)));
  this->RenderWindow = vtkRenderWindow::SafeDownCast(
    pvm->GetObjectFromID(this->RenderWindowProxy->GetID(0)));
  this->Interactor = vtkRenderWindowInteractor::SafeDownCast(
    pvm->GetObjectFromID(this->InteractorProxy->GetID(0)));
  this->ActiveCamera = vtkCamera::SafeDownCast(
    pvm->GetObjectFromID(this->ActiveCameraProxy->GetID(0)));

  //Set the defaul style for the interactor.
  vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
  this->Interactor->SetInteractorStyle(style);
  style->Delete();

  // Set the active camera for the renderers.  We can't use the Proxy
  // Property since Camera is only create on the CLIENT.  Proxy properties
  // don't take intersection of servers on which they are created before
  // setting as yet.
  this->GetRenderer()->SetActiveCamera(this->ActiveCamera);
  this->GetRenderer2D()->SetActiveCamera(this->ActiveCamera);
  
  if (pvm->GetOptions()->GetUseStereoRendering())
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->RenderWindowProxy->GetProperty("StereoCapableWindow"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find propery StereoCapableWindow.");
      return;
      }
    ivp->SetElement(0, 1);

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->RenderWindowProxy->GetProperty("StereoRender"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property StereoRender.");
      return;
      }
    ivp->SetElement(0, 1);
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Renderer2DProxy->GetProperty("Erase"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Erase.");
    return;
    }
  ivp->SetElement(0, 0);
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Renderer2DProxy->GetProperty("Layer"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Layer.");
    return;
    }
  ivp->SetElement(0, 2);
   
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderWindowProxy->GetProperty("NumberOfLayers"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property NumberOfLayers.");
    return;
    }
  ivp->SetElement(0, 3);

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->RenderWindowProxy->GetProperty("Renderer"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find propery Renderer.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RendererProxy);
  pp->AddProxy(this->Renderer2DProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->InteractorProxy->GetProperty("RenderWindow"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property RenderWindow.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RenderWindowProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->InteractorProxy->GetProperty("Renderer"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Renderer.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RendererProxy);
  this->RendererProxy->UpdateVTKObjects();
  this->Renderer2DProxy->UpdateVTKObjects();
  this->RenderWindowProxy->UpdateVTKObjects();
  //this->UpdateVTKObjects();
  
  // Make sure we have a chance to set the clipping range properly.
  vtkCallbackCommand* cbc;
  cbc = vtkCallbackCommand::New();
  cbc->SetCallback(vtkSMRenderModuleResetCameraClippingRange);
  cbc->SetClientData((void*)this);
  // ren will delete the cbc when the observer is removed.
  this->ResetCameraClippingRangeTag = 
    this->GetRenderer()->AddObserver(
      vtkCommand::ResetCameraClippingRangeEvent,cbc);
  cbc->Delete();

  vtkCallbackCommand* abc = vtkCallbackCommand::New();
  abc->SetCallback(vtkSMRenderModuleProxyAbortCheck);
  abc->SetClientData(this);
  this->AbortCheckTag =
    this->GetRenderWindow()->AddObserver(vtkCommand::AbortCheckEvent, abc);
  abc->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SetUseLight(int enable)
{
  if (!this->RendererProxy || !this->LightKitProxy)
    {
    vtkErrorMacro("Proxies not created yet!");
    return;
    }
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->LightKitProxy->GetProperty("Renderers"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Renderers on LightKitProxy.");
    return;
    }
  pp->RemoveAllProxies();
  if (enable)
    {
    pp->AddProxy(this->RendererProxy);
    }
  this->LightKitProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::InteractiveRender()
{
  this->UpdateAllDisplays();

  vtkRenderWindow *renWin = this->GetRenderWindow(); 
  renWin->SetDesiredUpdateRate(5.0);
  this->GetRenderer()->ResetCameraClippingRange();

  this->BeginInteractiveRender();
  renWin->Render();
  this->EndInteractiveRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::BeginInteractiveRender()
{
  vtkTimerLog::MarkStartEvent("Interactive Render");
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::EndInteractiveRender()
{
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::StillRender()
{
  this->UpdateAllDisplays();

  vtkRenderWindow *renWindow = this->GetRenderWindow(); 
  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->GetRenderer()->ResetCameraClippingRange();
  renWindow->SetDesiredUpdateRate(0.002);

  this->BeginStillRender();
  renWindow->Render();
  this->EndStillRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::BeginStillRender()
{
  vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule())->
    SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::EndStillRender()
{
  vtkTimerLog::MarkEndEvent("Still Render");
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::AddDisplay(vtkSMDisplayProxy* disp)
{
  if (!disp)
    {
    return;
    }
  this->Displays->AddItem(disp);
  disp->AddToRenderModule(this);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::RemoveDisplay(vtkSMDisplayProxy* disp)
{
  if (!disp)
    {
    return;
    }
  disp->RemoveFromRenderModule(this);
  this->Displays->RemoveItem(disp);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::RemoveAllDisplays()
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = 
      vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    disp->RemoveFromRenderModule(this);
    }
  iter->Delete();  
  this->Displays->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::CacheUpdate(int idx, int total)
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = 
      vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp || !disp->GetVisibilityCM())
      {
      continue;
      }
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      disp->GetProperty("CacheUpdate"));
    if (ivp)
      {
      // Not all displays support this property. 
      // Call CacheUpdate on those which support.
      ivp->SetElement(0, idx);
      ivp->SetElement(1, total);
      disp->UpdateVTKObjects();
      }
    }
  iter->Delete();
}


//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::InvalidateAllGeometries()
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = 
      vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp)
      {
      continue;
      }
    vtkSMProperty *p = disp->GetProperty("InvalidateGeometry");
    if (p)
      {
      p->Modified();
      disp->UpdateVTKObjects();
      }
    }
  iter->Delete(); 
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::UpdateAllDisplays()
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = 
      vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp || !disp->GetVisibilityCM())
      {
      // Some displays don't need updating.
      continue;
      }
    disp->Update();
    // We don;t use properties here since it tends to slow things down.
    }
  iter->Delete();  
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SetUseTriangleStrips(int val)
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = 
      vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp)
      {
      continue;
      }
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
      disp->GetProperty("UseStrips"));
    if (ivp)
      {
      ivp->SetElement(0, val);
      disp->UpdateVTKObjects();
      disp->MarkConsumersAsModified();
      }
    }
  iter->Delete();  
  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable triangle strips.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable triangle strips.");
    }
}


//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SetUseImmediateMode(int val)
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = 
      vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp)
      {
      continue;
      }
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
      disp->GetProperty("ImmediateModeRendering"));
    if (ivp)
      {
      ivp->SetElement(0, val);
      disp->UpdateVTKObjects();
      }
    }
  iter->Delete();  
  if (val)
    {
    vtkTimerLog::MarkEvent("--- Disable display lists.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Enable display lists.");
    }
}
    
//-----------------------------------------------------------------------------
vtkSMDisplayProxy* vtkSMRenderModuleProxy::CreateDisplayProxy()
{
  if (!this->DisplayXMLName)
    {
    vtkErrorMacro("DisplayXMLName must be set to create Display proxies.");
    return NULL;
    }
  
  vtkSMProxy* p = vtkSMObject::GetProxyManager()->NewProxy(
    "displays", this->DisplayXMLName);
  if (!p)
    {
    return NULL;
    }
  vtkSMDisplayProxy *pDisp = vtkSMDisplayProxy::SafeDownCast(p);
  if (!pDisp)
    {
    vtkErrorMacro(<< "'displays' ," <<  this->DisplayXMLName 
                  << " must be a subclass of vtkSMDisplayProxy.");
    p->Delete();
    return NULL;
    }
  return pDisp;
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SetBackgroundColorCM(double rgb[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Background"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Background on RenderModule.");
    return;
    }
  dvp->SetElements(rgb);
  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
double vtkSMRenderModuleProxy::GetZBufferValue(int x, int y)
{
  vtkFloatArray *array = vtkFloatArray::New();
  double val;

  array->SetNumberOfTuples(1);
  this->GetRenderWindow()->GetZbufferData(x,y, x, y,
                                     array);
  val = array->GetValue(0);
  array->Delete();
  return val;  
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::ResetCamera()
{
  double bds[6];
  this->ComputeVisiblePropBounds(bds);
  if (bds[0] <= bds[1] && bds[2] <= bds[3] && bds[4] <= bds[5])
    {
    this->ResetCamera(bds);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::ResetCamera(double bds[6])
{
  this->GetRenderer()->ResetCamera(bds);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::ResetCameraClippingRange()
{
  double bds[6];
  double range1[2];
  double range2[2];
  vtkRenderer* ren = this->GetRenderer();
  // Get default clipping range.
  // Includes 3D widgets but not all processes.
  ren->GetActiveCamera()->GetClippingRange(range1);

  this->ComputeVisiblePropBounds(bds);
  ren->ResetCameraClippingRange(bds);
  
  // Get part clipping range.
  // Includes all process partitions, but not 3d Widgets.
  ren->GetActiveCamera()->GetClippingRange(range2);

  // Merge
  if (range1[0] < range2[0])
    {
    range2[0] = range1[0];
    }
  if (range1[1] > range2[1])
    {
    range2[1] = range1[1];
    }
  // Includes all process partitions and 3D Widgets.
  ren->GetActiveCamera()->SetClippingRange(range2);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::ComputeVisiblePropBounds(double bds[6])
{
  // Compute the bounds for our sources.
  bds[0] = bds[2] = bds[4] = VTK_DOUBLE_MAX;
  bds[1] = bds[3] = bds[5] = -VTK_DOUBLE_MAX;

  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* pDisp = vtkSMDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp && pDisp->GetVisibilityCM() )
      {
      vtkPVGeometryInformation* info = pDisp->GetGeometryInformation();
      if (!info)
        {
        continue;
        }
      double *tmp = info->GetBounds();
      if (tmp[0] < bds[0]) { bds[0] = tmp[0]; }  
      if (tmp[1] > bds[1]) { bds[1] = tmp[1]; }  
      if (tmp[2] < bds[2]) { bds[2] = tmp[2]; }  
      if (tmp[3] > bds[3]) { bds[3] = tmp[3]; }  
      if (tmp[4] < bds[4]) { bds[4] = tmp[4]; }  
      if (tmp[5] > bds[5]) { bds[5] = tmp[5]; }  
      }
    }

  if ( bds[0] > bds[1])
    {
    bds[0] = bds[2] = bds[4] = -1.0;
    bds[1] = bds[3] = bds[5] = 1.0;
    }
  iter->Delete();
}


//-----------------------------------------------------------------------------
// We deliberately use streams for this addtion of actor proxies.
// Using property (and clean_command) causes problems with
// 3D widgets (since they get cleaned out when a source is added!).
void vtkSMRenderModuleProxy::AddPropToRenderer(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->RendererProxy->GetNumberOfIDs(); i++)
    {
    for (unsigned int j=0; j < proxy->GetNumberOfIDs(); j++)
      {
      stream << vtkClientServerStream::Invoke
        << this->RendererProxy->GetID(i)
        << "AddViewProp"
        << proxy->GetID(j)
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    this->RendererProps->AddItem(proxy);
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::AddPropToRenderer2D(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->Renderer2DProxy->GetNumberOfIDs(); i++)
    {
    for (unsigned int j=0; j < proxy->GetNumberOfIDs(); j++)
      {
      stream << vtkClientServerStream::Invoke
        << this->Renderer2DProxy->GetID(i)
        << "AddViewProp"
        << proxy->GetID(j)
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    this->Renderer2DProps->AddItem(proxy);
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::RemovePropFromRenderer(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->RendererProxy->GetNumberOfIDs(); i++)
    {
    for (unsigned int j=0; j < proxy->GetNumberOfIDs(); j++)
      {
      stream << vtkClientServerStream::Invoke
        << this->RendererProxy->GetID(i)
        << "RemoveViewProp"
        << proxy->GetID(j)
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    this->RendererProps->RemoveItem(proxy);
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::RemovePropFromRenderer2D(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->Renderer2DProxy->GetNumberOfIDs(); i++)
    {
    for (unsigned int j=0; j < proxy->GetNumberOfIDs(); j++)
      {
      stream << vtkClientServerStream::Invoke
        << this->Renderer2DProxy->GetID(i)
        << "RemoveViewProp"
        << proxy->GetID(j)
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    this->Renderer2DProps->RemoveItem(proxy);
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SynchronizeCameraProperties()
{
  this->ActiveCameraProxy->UpdateInformation();
  vtkSMPropertyIterator* iter = this->ActiveCameraProxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty *cur_property = iter->GetProperty();
    vtkSMProperty *info_property = cur_property->GetInformationProperty();
    if (!info_property)
      {
      continue;
      }
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      cur_property);
    vtkSMDoubleVectorProperty* info_dvp = 
      vtkSMDoubleVectorProperty::SafeDownCast(info_property);
    if (dvp && info_dvp)
      {
      dvp->SetElements(info_dvp->GetElements());
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SaveState(const char* name, ostream* file, 
  vtkIndent indent)
{
  this->SynchronizeCameraProperties();
  this->Superclass::SaveState(name, file, indent);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SaveInBatchScript(ofstream* file)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Render module not created yet!");
    return;
    }
  this->SynchronizeCameraProperties();

  *file << "set Ren1 [$proxyManager NewProxy "
    << this->GetXMLGroup() << " " << this->GetXMLName() << "]" << endl;
  *file << "  $proxyManager RegisterProxy " << this->GetXMLGroup()
    << " Ren1 $Ren1" << endl;
  *file << "  $Ren1 UnRegister {}" << endl;

  // Now, we save all the properties that are not Input.
  // Also note that only exposed properties are getting saved.
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* p = iter->GetProperty();
    if (vtkSMInputProperty::SafeDownCast(p))
      {
      continue;
      }

    if (!p->GetSaveable() || p->GetInformationOnly())
      {
      *file << "  # skipping proxy property " << p->GetXMLName() << endl;
      continue;
      }

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(p);
    vtkSMDoubleVectorProperty* dvp = 
      vtkSMDoubleVectorProperty::SafeDownCast(p);
    vtkSMStringVectorProperty* svp = 
      vtkSMStringVectorProperty::SafeDownCast(p);
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(p);
    if (ivp)
      {
      for (unsigned int i=0; i < ivp->GetNumberOfElements(); i++)
        {
        *file << "  [$Ren1 GetProperty {"
          << ivp->GetXMLName() << "}] SetElement "
          << i << " " << ivp->GetElement(i) 
          << endl;
        }
      }
    else if (dvp)
      {
      for (unsigned int i=0; i < dvp->GetNumberOfElements(); i++)
        {
        *file << "  [$Ren1 GetProperty {"
          << dvp->GetXMLName() << "}] SetElement "
          << i << " " << dvp->GetElement(i) 
          << endl;
        }
      }
    else if (svp)
      {
      for (unsigned int i=0; i < svp->GetNumberOfElements(); i++)
        {
        *file << "  [$Ren1 GetProperty {"
          << svp->GetXMLName() << "}] SetElement "
          << i << " {" << svp->GetElement(i) << "}"
          << endl;
        }
      }
    else if (pp)
      {
      // the only proxy property the RenderModule exposes is
      // Displays.
      for (unsigned int i=0; i < pp->GetNumberOfProxies(); i++)
        {
        vtkSMProxy* proxy = pp->GetProxy(i);
        // Some displays get saved in batch other don't,
        // instead of mirroring that logic to determine if
        // the display got saved in batch, we just catch the
        // exception.
        if (proxy->GetNumberOfIDs() == 0)
          {
          *file << "  catch { [$Ren1 GetProperty {"
            << pp->GetXMLName() << "}] AddProxy $pvTemp"
            << proxy->GetSelfID()
            << " } ;#--- " << proxy->GetXMLName() << endl;
          }
        else
          {
          for (unsigned int kk=0; kk < proxy->GetNumberOfIDs(); kk++)
            {
            *file << "  catch { [$Ren1 GetProperty {"
              << pp->GetXMLName() << "}] AddProxy $pvTemp"
              << proxy->GetID(kk)
              << " } ;#--- " << proxy->GetXMLName() 
              << " part " << kk << endl; 
            }
          }
        }
      }
    else
      {
      *file << "  # skipping property " << p->GetXMLName() << endl;
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMRenderModuleProxy::WriteImage(const char* filename,
  const char* writerName)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkObject* object = vtkInstantiator::CreateInstance(writerName);
  if (!object)
    {
    vtkErrorMacro("Failed to create Writer " << writerName);
    return vtkErrorCode::UnknownError;
    }
  vtkImageWriter* writer = vtkImageWriter::SafeDownCast(object);
  if (!writer)
    {
    vtkErrorMacro("Object is not a vtkImageWriter: " << object->GetClassName());
    object->Delete();
    return vtkErrorCode::UnknownError;
    }
 
  // I am using the vtkPVRenderView approach for saving the image.
  // instead of vtkSMDisplayWindowProxy approach of creating a proxy.
  this->GetRenderWindow()->SwapBuffersOff();
  this->StillRender();
  
  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->Update();

  this->GetRenderWindow()->SwapBuffersOn();
  this->GetRenderWindow()->Frame();
  

  writer->SetInput(w2i->GetOutput());
  writer->SetFileName(filename);
  writer->Write();
  int error_code = writer->GetErrorCode();

  writer->Delete();
  w2i->Delete();
  
  return error_code;
}

//-----------------------------------------------------------------------------
int vtkSMRenderModuleProxy::GetServerRenderWindowSize(int size[2])
{
  if (!this->RenderWindowProxy)
    {
    return 0;
    }
  vtkSMIntVectorProperty* winSize =
    vtkSMIntVectorProperty::SafeDownCast(
      this->RenderWindowProxy->GetProperty("RenderWindowSizeInfo"));
  if (!winSize)
    {
    return 0;
    }
  vtkTypeUInt32 servers =
    this->RenderWindowProxy->GetServers();
  this->RenderWindowProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  this->RenderWindowProxy->UpdatePropertyInformation(winSize);
  this->RenderWindowProxy->SetServers(servers);
  size[0] = winSize->GetElement(0);
  size[1] = winSize->GetElement(1);
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderInterruptsEnabled: " << this->RenderInterruptsEnabled 
    << endl;
  os << indent << "Displays: " << this->Displays << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "Renderer2D: " << this->Renderer2D << endl;
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "Interactor: " << this->Interactor << endl;
  os << indent << "ActiveCamera: " << this->ActiveCamera << endl;

  
}
