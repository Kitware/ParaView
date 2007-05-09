/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMRenderViewProxy.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageWriter.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkPVVisibleCellSelector.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimerLog.h"
#include "vtkWindowToImageFilter.h"

#include "vtkSMClientServerRenderModuleProxy.h"

vtkCxxRevisionMacro(vtkSMRenderViewProxy, "1.1");
vtkStandardNewMacro(vtkSMRenderViewProxy);

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
{
  // All the subproxies are created on Client and Render Server.
  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->LightKitProxy = 0;
  this->HelperProxy = 0;
  this->RenderInterruptsEnabled = 1;

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;
  this->Helper = 0;

  this->UseTriangleStrips = 0;
  this->ForceTriStripUpdate = 0;
  this->UseImmediateMode = 1;

  this->RenderTimer = vtkTimerLog::New();
  this->ResetPolygonsPerSecondResults();
  this->MeasurePolygonsPerSecond = 0;

  this->CacheLimit = 100*1024; // 100 MBs.
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy()
{
  this->RemoveAllRepresentations();

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
vtkSMRepresentationStrategy* vtkSMRenderViewProxy::NewStrategyInternal(
  int dataType, int type)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;

  if (type == SURFACE && dataType == VTK_POLY_DATA)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "PolyDataStrategy"));
    }
  else
    {
    vtkWarningMacro("This view does not provide a suitable strategy for "
      << dataType << ": " << type);
    }

  return strategy;
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::CreateVTKObjects(int numObjects)
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
    vtkProcessModule::CLIENT); // | vtkProcessModule::RENDER_SERVER);
  this->LightKitProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LightProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->HelperProxy->SetServers(
    vtkProcessModule::CLIENT_AND_SERVERS);

  this->Superclass::CreateVTKObjects(numObjects);

  vtkProcessModule* pvm = vtkProcessModule::GetProcessModule();
  
  // Set all the client side pointers.
  this->Renderer = vtkRenderer::SafeDownCast(
    pvm->GetObjectFromID(this->RendererProxy->GetID(0)));
  this->Renderer2D = vtkRenderer::SafeDownCast(
    pvm->GetObjectFromID(this->Renderer2DProxy->GetID(0)));
  this->RenderWindow = vtkRenderWindow::SafeDownCast(
    pvm->GetObjectFromID(this->RenderWindowProxy->GetID(0)));
  this->Interactor = vtkPVGenericRenderWindowInteractor::SafeDownCast(
    pvm->GetObjectFromID(this->InteractorProxy->GetID(0)));
  this->ActiveCamera = vtkCamera::SafeDownCast(
    pvm->GetObjectFromID(this->ActiveCameraProxy->GetID(0)));
  this->Helper = vtkPVRenderModuleHelper::SafeDownCast(
    pvm->GetObjectFromID(this->HelperProxy->GetID(0))); 
 
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

  // TODO: Enable this to enable depth peeling.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RendererProxy->GetProperty("DepthPeeling"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property DepthPeeling.");
    return;
    }
  ivp->SetElement(0, 1);
   
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderWindowProxy->GetProperty("NumberOfLayers"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property NumberOfLayers.");
    return;
    }
  ivp->SetElement(0, 3);

  this->Connect(this->RendererProxy, this->RenderWindowProxy, "Renderer");
  this->Connect(this->Renderer2DProxy, this->RenderWindowProxy, "Renderer");
  this->Connect(this->RenderWindowProxy, this->InteractorProxy, "RenderWindow");
  this->Connect(this->RendererProxy, this->InteractorProxy, "Renderer");
  this->Connect(this->LightProxy, this->RendererProxy, "Lights");

  // Set the active camera for the renderers.  We can't use the Proxy
  // Property since Camera is only create on the CLIENT.  Proxy properties
  // don't take intersection of servers on which they are created before
  // setting as yet.
  // the 2D active camera is excplicitly synchronized with this->ActiveCamera.
  this->Renderer->SetActiveCamera(this->ActiveCamera);

  this->RendererProxy->UpdateVTKObjects();
  this->Renderer2DProxy->UpdateVTKObjects();
  this->RenderWindowProxy->UpdateVTKObjects();
  this->InteractorProxy->UpdateVTKObjects();

  // Initialize observers.
  vtkCommand* observer = this->GetObserver();
  this->Renderer->AddObserver(
    vtkCommand::ResetCameraClippingRangeEvent, observer);
  this->Renderer->AddObserver(vtkCommand::StartEvent, observer);
  this->RenderWindow->AddObserver(vtkCommand::AbortCheckEvent, 
    this->GetObserver());
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData)
{
  if (eventId == vtkCommand::AbortCheckEvent && caller == this->RenderWindow)
    {
    if (this->RenderInterruptsEnabled)
      {
      this->InvokeEvent(vtkCommand::AbortCheckEvent);
      }
    }
  else if (eventId == vtkCommand::ResetCameraClippingRangeEvent &&
    caller == this->Renderer)
    {
    // Reset clipping range when ResetCameraClippingRange() is called on  the
    // renderer.
    this->ResetCameraClippingRange();
    }
  else if (eventId == vtkCommand::StartEvent && caller == this->Renderer)
    {
    // At the start of every render ensure that the 2D renderer and 3D renderer
    // have the same camera.
    this->SynchronizeRenderers();
    }

  this->Superclass::ProcessEvents(caller, eventId, callData);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SetUseLight(int enable)
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
void vtkSMRenderViewProxy::SetLODFlag(int val)
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
void vtkSMRenderViewProxy::BeginInteractiveRender()
{
  vtkRenderWindow *renWin = this->GetRenderWindow(); 
  renWin->SetDesiredUpdateRate(5.0);
  this->GetRenderer()->ResetCameraClippingRange();

  this->Superclass::BeginInteractiveRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::EndInteractiveRender()
{
  // This is a fast operation since we directly use the client side camera.
  this->ActiveCameraProxy->UpdatePropertyInformation();

  this->Superclass::EndInteractiveRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::BeginStillRender()
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

  this->SetLODFlag(0);
  this->Superclass::BeginStillRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::EndStillRender()
{
  // This is a fast operation since we directly use the client side camera.
  this->ActiveCameraProxy->UpdatePropertyInformation();

  this->Superclass::EndStillRender();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PerformRender()
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
void vtkSMRenderViewProxy::AddRepresentationInternal(
  vtkSMRepresentationProxy* repr)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    repr->GetProperty("UseStrips"));
  if (ivp)
    {
    ivp->SetElement(0, this->UseTriangleStrips);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    repr->GetProperty("ImmediateModeRendering"));
  if (ivp)
    {
    ivp->SetElement(0, this->UseImmediateMode);
    }
 
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    repr->GetProperty("RenderModuleHelper"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->HelperProxy);
    }

  this->Superclass::AddRepresentationInternal(repr);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SetUseTriangleStrips(int val)
{
  this->UseTriangleStrips = val;
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Representations->NewIterator());

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr)
      {
      continue;
      }

    vtkSMIntVectorProperty *fivp = vtkSMIntVectorProperty::SafeDownCast(
      repr->GetProperty("ForceStrips"));
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
      repr->GetProperty("UseStrips"));
    if (ivp)
      {
      if (fivp)
        {
        fivp->SetElement(0, this->ForceTriStripUpdate);
        }
      ivp->SetElement(0, val);
      repr->UpdateVTKObjects();
      repr->MarkModified(this);
      }
    }

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
void vtkSMRenderViewProxy::SetUseImmediateMode(int val)
{
  this->UseImmediateMode = val;

  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Representations->NewIterator());
  
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr)
      {
      continue;
      }
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
      repr->GetProperty("ImmediateModeRendering"));
    if (ivp)
      {
      ivp->SetElement(0, val);
      repr->UpdateVTKObjects();
      }
    }

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
void vtkSMRenderViewProxy::SetBackgroundColorCM(double rgb[3])
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
double vtkSMRenderViewProxy::GetZBufferValue(int x, int y)
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
vtkPVClientServerIdCollectionInformation* vtkSMRenderViewProxy
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
    setRendererMethod->AddProxy(this->RendererProxy);
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
vtkSMProxy *vtkSMRenderViewProxy::GetProxyFromPropID(
  vtkClientServerID *id,
  int proxyType)
{
  vtkSMProxy *ret = NULL;
// FIXME
#if 0 
  vtkCollectionIterator* iter = NULL;

  //find the SMDisplayProxy that contains the CSID.
  iter = this->Representations->NewIterator();
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
#endif
  return ret;
}


//----------------------------------------------------------------------------
vtkSMProxy *vtkSMRenderViewProxy::GetProxyForDisplay(
  int number,
  int proxyType)
{
  vtkSMProxy *ret = NULL;

  /* FIXME
  vtkSMDataObjectDisplayProxy* dodp = 
    vtkSMDataObjectDisplayProxy::SafeDownCast(
      this->Representations->GetItemAsObject(number)
      );
    
  if (dodp)
    {
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
    }    
  */
  return ret;
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera()
{
  double bds[6];
  this->ComputeVisiblePropBounds(bds);
  if (bds[0] <= bds[1] && bds[2] <= bds[3] && bds[4] <= bds[5])
    {
    this->ResetCamera(bds);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(double bds[6])
{
  this->GetRenderer()->ResetCamera(bds);
  this->ActiveCameraProxy->UpdatePropertyInformation();
  this->SynchronizeCameraProperties();

  this->Modified();
  this->InvokeEvent(vtkCommand::ResetCameraEvent);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCameraClippingRange()
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
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ComputeVisiblePropBounds(double bds[6])
{
  // Compute the bounds for our sources.
  bds[0] = bds[2] = bds[4] = VTK_DOUBLE_MAX;
  bds[1] = bds[3] = bds[5] = -VTK_DOUBLE_MAX;

  vtkCollectionIterator* iter = this->Representations->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (repr && repr->GetVisibility())
      {
      vtkPVDataInformation* info = repr->GetDisplayedDataInformation();
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
void vtkSMRenderViewProxy::AddPropToRenderer(vtkSMProxy* proxy)
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
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetConnectionID(),
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AddPropToRenderer2D(vtkSMProxy* proxy)
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
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetConnectionID(),
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::RemovePropFromRenderer(vtkSMProxy* proxy)
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
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetConnectionID(),
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::RemovePropFromRenderer2D(vtkSMProxy* proxy)
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
    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetConnectionID(),
      this->RendererProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SynchronizeRenderers()
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
void vtkSMRenderViewProxy::SynchronizeCameraProperties()
{
  if (!this->ObjectsCreated)
    {
    return;
    }
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
vtkPVXMLElement* vtkSMRenderViewProxy::SaveState(vtkPVXMLElement* root)
{
  this->SynchronizeCameraProperties();
  return this->Superclass::SaveState(root);
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMRenderViewProxy::CaptureWindow(int magnification)
{
  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.
#if !defined(__APPLE__)
  this->GetRenderWindow()->SetOffScreenRendering(1);
#endif
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
#if !defined(__APPLE__)
  this->GetRenderWindow()->SetOffScreenRendering(0);
#endif

  // Update image extents based on WindowPosition.
  int extents[6];
  capture->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    // FIXME
    //extents[cc] += this->WindowPosition[cc/2]*magnification;
    }
  capture->SetExtent(extents);

  return capture;
}

//-----------------------------------------------------------------------------
int vtkSMRenderViewProxy::WriteImage(const char* filename,
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
int vtkSMRenderViewProxy::GetServerRenderWindowSize(int size[2])
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
  if (true /*|| this->IsRenderLocal()*/) // FIXME
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
void vtkSMRenderViewProxy::CalculatePolygonsPerSecond(double time)
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
vtkIdType vtkSMRenderViewProxy::GetTotalNumberOfPolygons()
{
  vtkIdType totalPolygons = 0;
  vtkCollectionIterator* iter = this->Representations->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (repr && repr->GetVisibility())
      {
      vtkPVDataInformation* info = repr->GetDisplayedDataInformation();
      if (!info)
        {
        continue;
        }
      totalPolygons += info->GetPolygonCount();
      }
    }
  iter->Delete();
  return totalPolygons;
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetPolygonsPerSecondResults()
{
  this->LastPolygonsPerSecond = 0;
  this->MaximumPolygonsPerSecond = 0;
  this->AveragePolygonsPerSecond = 0;
  this->AveragePolygonsPerSecondAccumulated = 0;
  this->AveragePolygonsPerSecondCount = 0;
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderInterruptsEnabled: " << this->RenderInterruptsEnabled 
    << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "Renderer2D: " << this->Renderer2D << endl;
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "Interactor: " << this->Interactor << endl;
  os << indent << "ActiveCamera: " << this->ActiveCamera << endl;
  os << indent << "MeasurePolygonsPerSecond: " 
    << this->MeasurePolygonsPerSecond << endl;
  os << indent << "AveragePolygonsPerSecond: " 
    << this->AveragePolygonsPerSecond << endl;
  os << indent << "MaximumPolygonsPerSecond: " 
    << this->MaximumPolygonsPerSecond << endl;
  os << indent << "LastPolygonsPerSecond: " 
    << this->LastPolygonsPerSecond << endl;
}

//-----------------------------------------------------------------------------
int vtkSMRenderViewProxy::IsSelectionAvailable()
{  
  //check if we are not supposed to turn compositing on (in parallel)
  //the paraview gui uses a big number to say never composite
  //it we are not supposed to turn it on, then disallow selections
  double compThresh = 0;
  vtkSMClientServerRenderModuleProxy *me2 = 
    vtkSMClientServerRenderModuleProxy::SafeDownCast(this);
  if (me2 != NULL)
    {
    compThresh = me2->GetRemoteRenderThreshold();
    }
  if (compThresh > 100.0) //the highest setting in the paraview gui
    {
    return 0;
    }

  //check if we don't have enough color depth to do color buffer selection
  //if we don't then disallow selection
  int rgba[4];
  vtkRenderWindow *rwin = this->GetRenderWindow();
  if (!rwin)
    {
    return 0;
    }
  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    return 0;
    }

  //yeah!
  return 1;
}

//-----------------------------------------------------------------------------
vtkSelection* vtkSMRenderViewProxy::SelectVisibleCells(unsigned int x0, 
  unsigned int y0, unsigned int x1, unsigned int y1)
{  
  if (!this->IsSelectionAvailable())
    {
    vtkSelection *selection = vtkSelection::New();
    selection->Clear();
    return selection;
    }

  int *win_size=this->GetRenderWindow()->GetSize();
  unsigned int wsx = (unsigned int) win_size[0];
  unsigned int wsy = (unsigned int) win_size[1];
  x0 = (x0 >= wsx)? wsx-1: x0;
  x1 = (x1 >= wsx)? wsx-1: x1;
  y0 = (y0 >= wsy)? wsy-1: y0;
  y1 = (y1 >= wsy)? wsy-1: y1;

  //Find number of rendering processors.
  int numProcessors = 1;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  numProcessors = pm->GetNumberOfPartitions(this->ConnectionID);

  //Find largest polygon count in any actor
  vtkIdType maxNumCells = 0;
  vtkCollectionIterator* iter = this->Representations->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr || !repr->GetVisibility())
      {
      continue;
      }

    vtkPVDataInformation *gi = repr->GetDisplayedDataInformation();
    if (!gi)
      {
      continue;
      }
    vtkIdType numCells = gi->GetNumberOfCells();
    if (numCells > maxNumCells)
      {
      maxNumCells = numCells;
      }
    }
  iter->Delete();

  vtkIdType needs_2_passes = (maxNumCells+1)>>24;//more than 2^24-1 cells
  vtkIdType needs_3_passes = needs_2_passes>>24; //more than 2^48-1

  vtkSMProxyManager* proxyManager = vtkSMObject::GetProxyManager();  
  vtkSMProxy *vcsProxy = proxyManager->NewProxy("PropPickers", "PVVisibleCellSelector");
  vcsProxy->SetConnectionID(this->ConnectionID);
  vcsProxy->SetServers(this->Servers);

  //don't let the RenderSyncManager control back/front buffer swapping so that
  //we can do it here instead.
  vtkSMProxy *renderSyncManager = this->GetSubProxy("RenderSyncManager");
  vtkSMIntVectorProperty *setAllowBuffSwap = NULL;
  if (renderSyncManager)
    {
    setAllowBuffSwap = vtkSMIntVectorProperty::SafeDownCast(
      renderSyncManager->GetProperty("SetUseBackBuffer"));
    }
  if (setAllowBuffSwap != NULL)
    {
    setAllowBuffSwap->SetElements1(0);
    renderSyncManager->UpdateVTKObjects();
    }

  //how we get access to the renderers
  vtkSMProxyProperty *setRendererMethod = vtkSMProxyProperty::SafeDownCast(
    vcsProxy->GetProperty("SetRenderer"));
  setRendererMethod->AddProxy(this->RendererProxy);
  vcsProxy->UpdateVTKObjects();   

  //how we put the renderers into selection mode
  vtkSMIntVectorProperty *setModeMethod = vtkSMIntVectorProperty::SafeDownCast(
    vcsProxy->GetProperty("SetSelectMode"));
  vtkSMProperty *setPIdMethod = vcsProxy->GetProperty("LookupProcessorId");

  //I'm using the auto created PVVisCellSelectors in the vcsProxy above just
  //to set the SelectionMode of the remote renderers.
  //I use this one below to convert the composited images that arrive here
  //into a selection.
  vtkPVVisibleCellSelector *pti = vtkPVVisibleCellSelector::New();
  pti->SetRenderer(this->GetRenderer());
  pti->SetArea(x0,y0,x1,y1);
  pti->GetArea(x0,y0,x1,y1);

  //Use the back buffer
  int usefrontbuf = 0;
  if (!usefrontbuf)
    {
    this->GetRenderWindow()->SwapBuffersOff();
    }

  //Set background to black to indicate misses.
  //vtkRenderer::UpdateGeometryForSelection does this also, but in that case
  //iceT ignores it. Thus I duplicate the same effect here.
  double origBG[3];
  this->GetRenderer()->GetBackground(origBG);
  double black[3] = {0.0,0.0,0.0};
  this->SetBackgroundColorCM(black);

  //don't draw the scalar bar, text annotation, orientation axes
  vtkRendererCollection *rcol = this->RenderWindow->GetRenderers();
  int numlayers = rcol->GetNumberOfItems();
  int *renOldVis = new int[numlayers];
  for (int i = 0; i < numlayers; i++)
    {
    //I tried Using vtkRendererCollection::GetItem but that was giving me 
    //NULL for i=1,2 so I am doing it the long way
    vtkObject *anObj = rcol->GetItemAsObject(i);
    if (!anObj)
      {
      continue;
      }
    vtkRenderer *nextRen = vtkRenderer::SafeDownCast(anObj);
    if (!nextRen)
      {
      continue;
      }
    renOldVis[i] = nextRen->GetDraw();
    if (nextRen != this->Renderer)
      {
      nextRen->DrawOff();      
      }
    }

  //If stripping is on, turn it off (if anything has been changed since the 
  //last time we turned it off)
  //TODO: encode the cell original ids directly into the color and
  //to make this ugly hack uneccessary
  int use_strips = this->UseTriangleStrips;
  if (use_strips)
    {
    this->ForceTriStripUpdate = 1;
    this->SetUseTriangleStrips(0);    
    this->ForceTriStripUpdate = 0;
    }

  //Force parallel compositing on for the selection render.
  //TODO: intelligently code dataserver rank into originalcellids to
  //make this ugly hack unecessary.
  double compThresh = 0.0;
  vtkSMClientServerRenderModuleProxy *me2 = 
    vtkSMClientServerRenderModuleProxy::SafeDownCast(this);
  if (me2 != NULL)
    {
    compThresh = me2->GetRemoteRenderThreshold();
    me2->SetRemoteRenderThreshold(0.0);
    }      

  unsigned char *buf;  
  for (int p = 0; p < 5; p++)
    {
    if ((p==0) && (numProcessors==1))
      {
      p++;
      }
    if ((p==2) && (needs_3_passes==0))
      {
      p++;
      }
    if ((p==3) && (needs_2_passes==0))
      {
      p++;
      }
    //put into a selection mode
    setModeMethod->SetElements1(p+1);
    if (p==0)
      {
      setPIdMethod->Modified();
      }
    vcsProxy->UpdateVTKObjects();   

    //draw
    this->StillRender();  

    //get the intermediate results
    //renderwindow allocates this buffer
    buf = this->GetRenderWindow()->GetRGBACharPixelData(x0,y0,x1,y1,usefrontbuf); 
    //pti will deallocate the buffer when it is deleted
    pti->SavePixelBuffer(p, buf); 
    }
  
  //restore original rendering state
  //clear selection mode to resume normal rendering
  setModeMethod->SetElements1(0);
  vcsProxy->UpdateVTKObjects();   

  //Turn stripping back on if we had turned it off
  if (use_strips)
    {
    this->SetUseTriangleStrips(1);
    }

  //Force parallel compositing on for the selection render.
  if (me2 != NULL)
    {
    me2->SetRemoteRenderThreshold(compThresh);
    }      

  for (int i = 0; i < numlayers; i++)
    {
    vtkObject *anObj = rcol->GetItemAsObject(i);
    if (!anObj)
      {
      continue;
      }
    vtkRenderer *nextRen = vtkRenderer::SafeDownCast(anObj);
    if (!nextRen)
      {
      continue;
      }
    nextRen->SetDraw(renOldVis[i]);
    }
  delete[] renOldVis;

  this->SetBackgroundColorCM(origBG);
  if (!usefrontbuf)
    {
    this->GetRenderWindow()->SwapBuffersOn();
    }

  if (setAllowBuffSwap != NULL)
    {
    //let the RenderSyncManager control back/front buffer swapping
    setAllowBuffSwap->SetElements1(1);
    renderSyncManager->UpdateVTKObjects();
    }
  
  //convert the intermediate results into  the selection data structure 
  pti->ComputeSelectedIds();
  vtkSelection *selection = vtkSelection::New();
  pti->GetSelectedIds(selection);

  //cleanup
  pti->Delete();
  vcsProxy->Delete();

  return selection;
}

