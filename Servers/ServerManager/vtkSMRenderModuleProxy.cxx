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

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkImageWriter.h"
#include "vtkInstantiator.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimerLog.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWindowToImageFilter.h"

#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSMDataObjectDisplayProxy.h"

vtkCxxRevisionMacro(vtkSMRenderModuleProxy, "1.38");
//-----------------------------------------------------------------------------
// This is a bit of a pain.  I do ResetCameraClippingRange as a call back
// because the PVInteractorStyles call ResetCameraClippingRange 
// directly on the renderer.  Since they are PV styles, I might
// have them call the render module directly like they do for render.
void vtkSMRenderModuleResetCameraClippingRange(
 vtkObject *, unsigned long vtkNotUsed(event),void *clientData, void *)
{
  vtkSMRenderModuleProxy* self = 
    reinterpret_cast<vtkSMRenderModuleProxy*>(clientData);
  if(self)
    {
    self->ResetCameraClippingRange();
    }
}
//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxyAbortCheck(
  vtkObject*, unsigned long, void* arg, void*)
{
  vtkSMRenderModuleProxy* self = 
    reinterpret_cast<vtkSMRenderModuleProxy*>(arg);
  if (self && self->GetRenderInterruptsEnabled())
    {
    self->InvokeEvent(vtkCommand::AbortCheckEvent, NULL);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxyStartRenderEvent(
  vtkObject*, unsigned long, void* arg, void*)
{
  vtkSMRenderModuleProxy* self = reinterpret_cast<vtkSMRenderModuleProxy*>(arg);
  if (self)
    {
    self->SynchronizeRenderers();
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
  this->HelperProxy = 0;
  this->RendererProps = vtkCollection::New();
  this->Renderer2DProps = vtkCollection::New();
  this->ResetCameraClippingRangeTag = 0;
  this->AbortCheckTag = 0;
  this->StartRenderEventTag = 0;
  this->RenderInterruptsEnabled = 1;

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;
  this->Helper = 0;

  this->UseTriangleStrips = 0;
  this->UseImmediateMode = 1;

  this->RenderTimer = vtkTimerLog::New();
  this->ResetPolygonsPerSecondResults();
  this->MeasurePolygonsPerSecond = 0;
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
  if (this->StartRenderEventTag && this->Renderer)
    {
    this->Renderer->RemoveObserver(this->StartRenderEventTag);
    this->StartRenderEventTag = 0;
    }
  this->RendererProps->Delete();
  this->Renderer2DProps->Delete();
  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->LightKitProxy = 0;
  this->HelperProxy = 0;

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;
  this->Helper = 0;
  this->RenderTimer->Delete();
  this->RenderTimer = 0;
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
  this->LightProxy = this->GetSubProxy("Light");
  this->HelperProxy = this->GetSubProxy("Helper");

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
    return;
    }
  if (!this->LightProxy)
    {
    vtkErrorMacro("Light subproxy must be defined in the configuration "
                  "file.");
    return;
    }
  if (!this->HelperProxy)
    {
    vtkErrorMacro("Helper subproxy must be defined in the configuration "
                  "file.");
    return;
    }
    

  // I don't directly use this->SetServers() to set the servers of the
  // subproxies, as the subclasses may have special subproxies that have
  // specific servers on which they want those to be created.
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
  this->LightProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->HelperProxy->SetServers(
    vtkProcessModule::CLIENT_AND_SERVERS);

  this->Superclass::CreateVTKObjects(numObjects);

  vtkProcessModule* pvm = vtkProcessModule::GetProcessModule();
  
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
  this->Helper = vtkPVRenderModuleHelper::SafeDownCast(
    pvm->GetObjectFromID(this->HelperProxy->GetID(0)));

  //Set the defaul style for the interactor.
  vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
  this->Interactor->SetInteractorStyle(style);
  style->Delete();

  // Set the active camera for the renderers.  We can't use the Proxy
  // Property since Camera is only create on the CLIENT.  Proxy properties
  // don't take intersection of servers on which they are created before
  // setting as yet.
  this->GetRenderer()->SetActiveCamera(this->ActiveCamera);
  // the 2D active camera is excplicitly synchronized with this->ActiveCamera.
  
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

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Renderer2DProxy->GetProperty("AutomaticLightCreation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property AutomaticLightCreation.");
    return;
    }
  ivp->SetElement(0, 0);
  pp = vtkSMProxyProperty::SafeDownCast(
    this->RendererProxy->GetProperty("Lights"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Lights.");
    return;
    }
  pp->AddProxy(this->LightProxy);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LightProxy->GetProperty("LightType"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property LightType.");
    return;
    }
  ivp->SetElement(0, 1); // Headlight

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

  vtkCallbackCommand* src = vtkCallbackCommand::New();
  src->SetCallback(vtkSMRenderModuleProxyStartRenderEvent);
  src->SetClientData(this);
  this->StartRenderEventTag =
    this->GetRenderer()->AddObserver(vtkCommand::StartEvent, src);
  src->Delete();
  
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
void vtkSMRenderModuleProxy::SetLODFlag(int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->HelperProxy->GetProperty("LODFlag"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property LODFlag on HelperProxy.");
    return;
    }
  if (ivp->GetElement(0) != val)
    {
    ivp->SetElement(0, val);
    this->HelperProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::BeginInteractiveRender()
{
  vtkRenderWindow *renWin = this->GetRenderWindow(); 
  renWin->SetDesiredUpdateRate(5.0);
  this->GetRenderer()->ResetCameraClippingRange();

  //vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  //pm->SendPrepareProgress(this->ConnectionID, this->GetRenderingProgressServers());
  this->Superclass::BeginInteractiveRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::EndInteractiveRender()
{
  this->Superclass::EndInteractiveRender();
  //pm->SendCleanupPendingProgress(this->ConnectionID);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::BeginStillRender()
{
  vtkRenderWindow *renWindow = this->GetRenderWindow(); 
  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  if ( ! vtkProcessModule::GetStreamBlock())
    {
    this->GetRenderer()->ResetCameraClippingRange();
    }
  renWindow->SetDesiredUpdateRate(0.002);

  // vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  //pm->SendPrepareProgress(this->ConnectionID, this->GetRenderingProgressServers());

  this->SetLODFlag(0);
  this->Superclass::BeginStillRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::EndStillRender()
{
  this->Superclass::EndStillRender();

  //pm->SendCleanupPendingProgress(this->ConnectionID);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::PerformRender()
{
  if ( this->MeasurePolygonsPerSecond )
    {
    this->RenderTimer->StartTimer();
    }

  vtkRenderWindow *renWindow = this->GetRenderWindow(); 
  renWindow->Render();

  if ( this->MeasurePolygonsPerSecond )
    {
    this->RenderTimer->StopTimer();
    this->CalculatePolygonsPerSecond(this->RenderTimer->GetElapsedTime());
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::AddDisplay(vtkSMDisplayProxy* disp)
{
  if (!disp)
    {
    return;
    }
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    disp->GetProperty("UseStrips"));
  if (ivp)
    {
    ivp->SetElement(0, this->UseTriangleStrips);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    disp->GetProperty("ImmediateModeRendering"));
  if (ivp)
    {
    ivp->SetElement(0, this->UseImmediateMode);
    }
 
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    disp->GetProperty("RenderModuleHelper"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->HelperProxy);
    }

  disp->AddToRenderModule(this);

  this->Superclass::AddDisplay(disp);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::RemoveDisplay(vtkSMDisplayProxy* disp)
{
  if (!disp)
    {
    return;
    }
  disp->RemoveFromRenderModule(this);
  this->Superclass::RemoveDisplay(disp);
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::CacheUpdate(int idx, int total)
{
  vtkCollectionIterator* iter = this->GetDisplays()->NewIterator();
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
  vtkCollectionIterator* iter = this->GetDisplays()->NewIterator();
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
void vtkSMRenderModuleProxy::SetUseTriangleStrips(int val)
{
  this->UseTriangleStrips = val;
  vtkCollectionIterator* iter = this->GetDisplays()->NewIterator();
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
      disp->MarkModified(this);
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
  this->UseImmediateMode = val;
  vtkCollectionIterator* iter = this->GetDisplays()->NewIterator();
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

//----------------------------------------------------------------------------
vtkPVClientServerIdCollectionInformation* vtkSMRenderModuleProxy
  ::Pick(int xs, int ys, int xe, int ye)
{
  vtkProcessModule* processModule = NULL;
  vtkSMProxyManager* proxyManager = NULL;
  vtkSMProxy *areaPickerProxy = NULL;
  vtkSMProxyProperty *setRendererMethod = NULL;
  vtkSMDoubleVectorProperty *setCoordsMethod = NULL;
  vtkSMProperty *pickMethod = NULL;

  bool OK = true;

  //create an areapicker and find its methods
  processModule = vtkProcessModule::GetProcessModule();
  if (!processModule)
    {
    vtkErrorMacro("Failed to find processmodule.");
    OK = false;
    }  
  proxyManager = vtkSMObject::GetProxyManager();  
  if (OK && !proxyManager)
    {
    vtkErrorMacro("Failed to find the proxy manager.");
    OK = false;
    }
  areaPickerProxy = proxyManager->NewProxy("PropPickers", "AreaPicker"); 
  if (OK && !areaPickerProxy)
    {
    vtkErrorMacro("Failed to make AreaPicker proxy.");
    OK = false;
    }
  setRendererMethod = vtkSMProxyProperty::SafeDownCast(
    areaPickerProxy->GetProperty("SetRenderer"));
  if (OK && !setRendererMethod)
    {
    vtkErrorMacro("Failed to find the set renderer property.");
    OK = false;
    }
  setCoordsMethod = vtkSMDoubleVectorProperty::SafeDownCast(
    areaPickerProxy->GetProperty("SetPickCoords"));
  if (OK && !setCoordsMethod)
    {
    vtkErrorMacro("Failed to find the set pick coords property.");
    OK = false;
    }
  pickMethod = areaPickerProxy->GetProperty("Pick");
  if (OK && !pickMethod)
    {
    vtkErrorMacro("Failed to find the pick property.");
    OK = false;
    }

  vtkPVClientServerIdCollectionInformation *propCollectionInfo = NULL;    
  if (OK)
    {
    //execute the areapick
    setRendererMethod->AddProxy(this->GetRendererProxy());
    setCoordsMethod->SetElements4(xs, ys, xe, ye);
    areaPickerProxy->UpdateVTKObjects();   
    pickMethod->Modified();
    areaPickerProxy->UpdateVTKObjects();   
    
    //gather the results from the AreaPicker
    propCollectionInfo = vtkPVClientServerIdCollectionInformation::New();
    processModule->GatherInformation(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::RENDER_SERVER, 
      propCollectionInfo, 
      areaPickerProxy->GetID(0)
      );
    }

  if (areaPickerProxy)
    {
    areaPickerProxy->Delete();
    }
  
  return propCollectionInfo;
}

//----------------------------------------------------------------------------
vtkSMProxy *vtkSMRenderModuleProxy::GetProxyFromPropID(
  vtkClientServerID *id,
  int proxyType)
{
  vtkCollectionIterator* iter = NULL;
  vtkSMProxy *ret = NULL;

  //find the SMDisplayProxy that contains the CSID.
  iter = this->GetDisplays()->NewIterator();
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem())
    {
    vtkSMDataObjectDisplayProxy* dodp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    
    if (dodp)
      {
      vtkSMProxy *actorProxy = dodp->GetActorProxy();
      vtkClientServerID idA = actorProxy->GetID(0);
      if (idA == *id)
        {
        //we found it, now return the proxy for the algorithm that produced it
        if (proxyType == DISPLAY)
          {
          ret = dodp;
          }
        else if (proxyType == INPUT)
          {
          ret = dodp->GetInput(0);
          }
        else if (proxyType == GEOMETRY)
          {
          ret = dodp->GetGeometryFilterProxy();
          }
        break;
        }
      }    
    }

  iter->Delete();  
  return ret;
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

  vtkCollectionIterator* iter = this->GetDisplays()->NewIterator();
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
    // Make sure that the display was created
    proxy->CreateVTKObjects(1);
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
      this->RendererProxy->GetConnectionID(),
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
      this->RendererProxy->GetConnectionID(),
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
      this->RendererProxy->GetConnectionID(),
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
      this->RendererProxy->GetConnectionID(),
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SynchronizeRenderers()
{
  // Synchronize the camera properties between the 3D and 2D renders.
  if (this->Renderer && this->Renderer2D)
    {
    this->Renderer2D->GetActiveCamera()->SetClippingRange(
      this->Renderer->GetActiveCamera()->GetClippingRange());
    this->Renderer2D->GetActiveCamera()->SetPosition(
      this->Renderer->GetActiveCamera()->GetPosition());
    this->Renderer2D->GetActiveCamera()->SetFocalPoint(
      this->Renderer->GetActiveCamera()->GetFocalPoint());
    this->Renderer2D->GetActiveCamera()->SetViewUp(
      this->Renderer->GetActiveCamera()->GetViewUp());

    this->Renderer2D->GetActiveCamera()->SetParallelProjection(
      this->Renderer->GetActiveCamera()->GetParallelProjection());
    this->Renderer2D->GetActiveCamera()->SetParallelScale(
      this->Renderer->GetActiveCamera()->GetParallelScale());
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::SynchronizeCameraProperties()
{
  this->ActiveCameraProxy->UpdatePropertyInformation();
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
      dvp->UpdateLastPushedValues();
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMRenderModuleProxy::SaveState(vtkPVXMLElement* root)
{
  this->SynchronizeCameraProperties();
  return this->Superclass::SaveState(root);
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
  this->Superclass::SaveInBatchScript(file);
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMRenderModuleProxy::CaptureWindow(int magnification)
{
  // I am using the vtkPVRenderView approach for saving the image.
  // instead of vtkSMDisplayWindowProxy approach of creating a proxy.
  this->GetRenderWindow()->SetOffScreenRendering(1);
  this->GetRenderWindow()->SwapBuffersOff();
  this->StillRender();

  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->Update();

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  w2i->Delete();

  this->GetRenderWindow()->SwapBuffersOn();
  this->GetRenderWindow()->Frame();
  this->GetRenderWindow()->SetOffScreenRendering(0);

  return capture;
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

  vtkImageData* shot = this->CaptureWindow(1);
  writer->SetInput(shot);
  writer->SetFileName(filename);
  writer->Write();
  int error_code = writer->GetErrorCode();

  writer->Delete();
  shot->Delete();
  
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
  if (this->IsRenderLocal())
    {
    this->RenderWindowProxy->SetServers(vtkProcessModule::CLIENT);
    }
  else
    {
    this->RenderWindowProxy->SetServers(vtkProcessModule::RENDER_SERVER);
    }
  this->RenderWindowProxy->UpdatePropertyInformation(winSize);
  this->RenderWindowProxy->SetServers(servers);
  size[0] = winSize->GetElement(0);
  size[1] = winSize->GetElement(1);
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::CalculatePolygonsPerSecond(double time)
{
  vtkIdType numPolygons = this->GetTotalNumberOfPolygons();
  if ( numPolygons <= 0 || time <= 0 )
    {
    return;
    }
  this->LastPolygonsPerSecond = (numPolygons / time);
  if ( this->LastPolygonsPerSecond > this->MaximumPolygonsPerSecond )
    {
    this->MaximumPolygonsPerSecond = this->LastPolygonsPerSecond;
    }
  this->AveragePolygonsPerSecondAccumulated += this->LastPolygonsPerSecond;
  this->AveragePolygonsPerSecondCount ++;
  this->AveragePolygonsPerSecond
    = this->AveragePolygonsPerSecondAccumulated /
    this->AveragePolygonsPerSecondCount;
}

//-----------------------------------------------------------------------------
vtkIdType vtkSMRenderModuleProxy::GetTotalNumberOfPolygons()
{
  vtkIdType totalPolygons = 0;
  vtkCollectionIterator* iter = this->GetDisplays()->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDataObjectDisplayProxy* pDisp = vtkSMDataObjectDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp && pDisp->GetVisibilityCM())
      {
      vtkPVGeometryInformation* info = pDisp->GetGeometryInformation();
      if (!info)
        {
        continue;
        }
      if (pDisp->GetVolumeRenderMode())
        {
        totalPolygons += 0;
        }
      else
        {
        totalPolygons += info->GetPolygonCount();
        }
      }
    }
  iter->Delete();
  return totalPolygons;
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::ResetPolygonsPerSecondResults()
{
  this->LastPolygonsPerSecond = 0;
  this->MaximumPolygonsPerSecond = 0;
  this->AveragePolygonsPerSecond = 0;
  this->AveragePolygonsPerSecondAccumulated = 0;
  this->AveragePolygonsPerSecondCount = 0;
}

//-----------------------------------------------------------------------------
void vtkSMRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderInterruptsEnabled: " << this->RenderInterruptsEnabled 
    << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "Renderer2D: " << this->Renderer2D << endl;
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "Interactor: " << this->Interactor << endl;
  os << indent << "ActiveCamera: " << this->ActiveCamera << endl;
}
