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
#include "vtkSMProxyProperty.h"
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

vtkCxxRevisionMacro(vtkSMRenderModuleProxy, "1.1.2.4");
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
  this->Displays = vtkCollection::New();
  this->DisplayXMLName = 0;
  this->ResetCameraClippingRangeTag = 0;
  this->AbortCheckTag = 0;
  this->RenderInterruptsEnabled = 1;
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
  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->SetDisplayXMLName(0);
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

  if (!this->RendererProxy)
    {
    vtkErrorMacro("Renderer subproxy must be defined in the configuration file.");
    return;
    }
  if (!this->Renderer2DProxy)
    {
    vtkErrorMacro("Renderer2D subproxy must be defined in the configuration file.");
    return;
    }
  if (!this->ActiveCameraProxy)
    {
    vtkErrorMacro("ActiveCamera subproxy must be defined in the configuration file.");
    return;
    }
  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined in the configuration file.");
    return;
    }
  if (!this->InteractorProxy)
    {
    vtkErrorMacro("Interactor subproxy must be defined in the configuration file.");
    return;
    }

  // I don't directly use this->SetServers() to set the servers of the subproxies,
  // as the subclasses may have special subproxies that have specific servers on which
  // they want those to be created.
  this->SetServersSelf(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->RendererProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Renderer2DProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER); 
  // Camera vtkObject is only created on the client. 
  // This is so as we don't change the active camera on the
  // servers as creation renderer (like IceT Tile renderer) fail if the active camera
  // on  the renderer is modified.
  this->ActiveCameraProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->RenderWindowProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->InteractorProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects(numObjects);

  vtkPVProcessModule* pvm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  // Set the active camera for the renderers.
  /*
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->RendererProxy->GetID(0)
    << "GetActiveCamera"
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Assign
    << this->ActiveCameraProxy->GetID(0)
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
    << this->Renderer2DProxy->GetID(0)
    << "GetActiveCamera"
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Assign
    << this->ActiveCameraProxy->GetID(0)
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pvm->SendStream(vtkProcessModule::RENDER_SERVER, stream);

  this->ActiveCameraProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
 // We can't use the Proxy Property since Camera is only create on the CLIENT.
  vtkSMProxyProperty* pp1 = vtkSMProxyProperty::SafeDownCast(
    this->RendererProxy->GetProperty("ActiveCamera"));
  vtkSMProxyProperty* pp2 = vtkSMProxyProperty::SafeDownCast(
    this->Renderer2DProxy->GetProperty("ActiveCamera"));
  
  if (!pp1)
    {
    vtkErrorMacro("Failed to find property ActiveCamera.");
    return;
    }

  if (!pp2)
    {
    vtkErrorMacro("Failed to find property ActiveCamera.");
    return;
    }
  pp1->RemoveAllProxies();
  pp2->RemoveAllProxies();
  pp1->AddProxy(this->ActiveCameraProxy);
  pp2->AddProxy(this->ActiveCameraProxy);
  */
  // Need to fix the camera behaviour.
  vtkCamera *camera = vtkCamera::SafeDownCast(
    pvm->GetObjectFromID(this->ActiveCameraProxy->GetID(0)));
  if (!camera)
    {
    vtkErrorMacro("Failed to create Camera.");
    }
  else
    {
    this->GetRenderer()->SetActiveCamera(camera);
    this->GetRenderer2D()->SetActiveCamera(camera);
    }
  
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
    this->GetRenderer()->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
  cbc->Delete();

  vtkCallbackCommand* abc = vtkCallbackCommand::New();
  abc->SetCallback(vtkSMRenderModuleProxyAbortCheck);
  abc->SetClientData(this);
  this->AbortCheckTag =
    this->GetRenderWindow()->AddObserver(vtkCommand::AbortCheckEvent, abc);
  abc->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::InteractiveRender()
{
  this->UpdateAllDisplays();

  vtkRenderWindow *renWin = this->GetRenderWindow(); 
  renWin->SetDesiredUpdateRate(5.0);
  this->ResetCameraClippingRange();

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
  vtkTimerLog::MarkEvent("Interactive Render");
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
  this->ResetCameraClippingRange();
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
void vtkSMRenderModuleProxy::CacheUpdate(int idx, int total)
{
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDisplayProxy* disp = vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp || !this->GetDisplayVisibility(disp))
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
    vtkSMDisplayProxy* disp = vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp)
      {
      continue;
      }
    vtkSMProperty *p = vtkSMIntVectorProperty::SafeDownCast(
      disp->GetProperty("InvalidateGeometry"));
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
    vtkSMDisplayProxy* disp = vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp || !disp->cmGetVisibility())
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
    vtkSMDisplayProxy* disp = vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
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
    vtkSMDisplayProxy* disp = vtkSMDisplayProxy::SafeDownCast(iter->GetCurrentObject());
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
int vtkSMRenderModuleProxy::GetDisplayVisibility(vtkSMDisplayProxy* pDisp)
{
  if (!pDisp)
    {
    return 0;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on vtkSMDisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
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
    vtkErrorMacro("'displays' ," <<  this->DisplayXMLName << " must be a subclass of "
      "vtkSMDisplayProxy.");
    p->Delete();
    return NULL;
    }
  return pDisp;
}

//-----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkSMRenderModuleProxy::GetInteractor()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return vtkRenderWindowInteractor::SafeDownCast(
    pm->GetObjectFromID(this->InteractorProxy->GetID(0)));
    
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkSMRenderModuleProxy::GetRenderer()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(this->RendererProxy->GetID(0))); 
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkSMRenderModuleProxy::GetRenderer2D()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(this->Renderer2DProxy->GetID(0))); 
}

//-----------------------------------------------------------------------------
vtkRenderWindow* vtkSMRenderModuleProxy::GetRenderWindow()
{
  return vtkRenderWindow::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(
      this->RenderWindowProxy->GetID(0)));
}
//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SetBackgroundColor(double rgb[3])
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
void vtkSMRenderModuleProxy::ResetCamera(double bds[6])
{
  this->GetRenderer()->ResetCamera(bds);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::ResetCameraClippingRange()
{
  double bounds[6];
  vtkRenderer* ren = this->GetRenderer();
  this->ComputeVisiblePropBounds(bounds);
  ren->ResetCameraClippingRange(bounds);
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
    if (pDisp && this->GetDisplayVisibility(pDisp) )
      {
      double *tmp = pDisp->GetGeometryInformation()->GetBounds();
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
void vtkSMRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderInterruptsEnabled: " << this->RenderInterruptsEnabled 
    << endl;
}
