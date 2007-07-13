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
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
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
#include "vtkSMDataRepresentationProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimerLog.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPVDisplayInformation.h"
#include "vtkSMRenderViewHelper.h"

#include "vtkSMMultiProcessRenderView.h"

#include <vtkstd/map>
#include <vtkstd/set>

//-----------------------------------------------------------------------------
inline bool SetIntVectorProperty(vtkSMProxy* proxy, const char* pname,
  int val, bool report_error=true)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (!ivp)
    {
    if (report_error)
      {
      vtkGenericWarningMacro("Failed to locate property "
        << pname << " on proxy  " << proxy->GetXMLName());
      }
    return false;
    }
  ivp->SetElement(0, val);
  return true;
}

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMRenderViewProxy, "1.28");
vtkStandardNewMacro(vtkSMRenderViewProxy);

vtkInformationKeyMacro(vtkSMRenderViewProxy, LOD_RESOLUTION, Integer);
vtkInformationKeyMacro(vtkSMRenderViewProxy, USE_COMPOSITING, Integer);
vtkInformationKeyMacro(vtkSMRenderViewProxy, USE_LOD, Integer);
vtkInformationKeyMacro(vtkSMRenderViewProxy, USE_ORDERED_COMPOSITING, Integer);

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
{
  // All the subproxies are created on Client and Render Server.
  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->InteractorStyleProxy = 0;
  this->LightKitProxy = 0;
  this->RenderInterruptsEnabled = 1;

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;

  this->RenderViewHelper = vtkSMRenderViewHelper::New();
  this->RenderViewHelper->SetRenderViewProxy(this); //not reference counted.

  this->UseTriangleStrips = 0;
  this->ForceTriStripUpdate = 0;
  this->UseImmediateMode = 1;

  this->RenderTimer = vtkTimerLog::New();
  this->ResetPolygonsPerSecondResults();
  this->MeasurePolygonsPerSecond = 0;

  this->LODThreshold = 0.0;

  this->OpenGLExtensionsInformation = 0;

  this->SetUseLOD(false);
  this->SetLODResolution(50);
  this->Information->Set(USE_ORDERED_COMPOSITING(), 0);
  this->Information->Set(USE_COMPOSITING(), 0);
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy()
{
  this->RenderViewHelper->SetRenderViewProxy(0);
  this->RenderViewHelper->Delete();

  this->RemoveAllRepresentations();

  this->RendererProxy = 0;
  this->Renderer2DProxy = 0;
  this->ActiveCameraProxy = 0;
  this->RenderWindowProxy = 0;
  this->InteractorProxy = 0;
  this->LightKitProxy = 0;

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->Interactor = 0;
  this->ActiveCamera = 0;
  this->RenderTimer->Delete();
  this->RenderTimer = 0;
  if (this->OpenGLExtensionsInformation)
    {
    this->OpenGLExtensionsInformation->Delete();
    this->OpenGLExtensionsInformation = 0;
    }
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMRenderViewProxy::NewStrategyInternal(
  int dataType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;

  if ((dataType == VTK_POLY_DATA) || dataType == VTK_UNIFORM_GRID)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "PolyDataStrategy"));
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "UnstructuredGridStrategy"));
    }
  else
    {
    vtkWarningMacro("This view does not provide a suitable strategy for "
      << dataType);
    }

  return strategy;
}

//-----------------------------------------------------------------------------
vtkPVOpenGLExtensionsInformation* 
vtkSMRenderViewProxy::GetOpenGLExtensionsInformation()
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet. Cannot get OpenGL extensions information");
    return 0;
    }
  if (this->OpenGLExtensionsInformation)
    {
    return this->OpenGLExtensionsInformation;
    }

  this->OpenGLExtensionsInformation = vtkPVOpenGLExtensionsInformation::New();
  /*
  // FIXME:
  // When in client-server mode, if the client has not created the
  // server-side windows, then asking for extentions on the server side
  // triggers a render on the server-side render window which hangs.
  // Hence for now, using only the client side information.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID, vtkProcessModule::CLIENT,
    this->OpenGLExtensionsInformation, 
    this->RenderWindowProxy->GetID());
    */
  return this->OpenGLExtensionsInformation;
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::BeginCreateVTKObjects()
{
  // Initialize all subproxy pointers and servers on which those subproxies are
  // to be created.
  this->RendererProxy = this->GetSubProxy("Renderer");
  this->Renderer2DProxy = this->GetSubProxy("Renderer2D");
  this->ActiveCameraProxy = this->GetSubProxy("ActiveCamera");
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");
  this->InteractorProxy = this->GetSubProxy("Interactor");
  this->InteractorStyleProxy = this->GetSubProxy("InteractorStyle");
  this->LightKitProxy = this->GetSubProxy("LightKit");
  this->LightProxy = this->GetSubProxy("Light");

  if (!this->RendererProxy)
    {
    vtkErrorMacro("Renderer subproxy must be defined in the "
                  "configuration file.");
    return false;
    }

  if (!this->Renderer2DProxy)
    {
    vtkErrorMacro("Renderer2D subproxy must be defined in the "
                  "configuration file.");
    return false;
    }

  if (!this->ActiveCameraProxy)
    {
    vtkErrorMacro("ActiveCamera subproxy must be defined in the "
                  "configuration file.");
    return false;
    }

  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined in the configuration "
                  "file.");
    return false;
    }

  if (!this->InteractorProxy)
    {
    vtkErrorMacro("Interactor subproxy must be defined in the configuration "
                  "file.");
    return false;
    }

  if (!this->InteractorStyleProxy)
    {
    vtkErrorMacro("InteractorStyleProxy subproxy must be defined in the configuration "
                  "file.");
    return false;
    }

  if (!this->LightKitProxy)
    {
    vtkErrorMacro("LightKit subproxy must be defined in the configuration "
                  "file.");
    return false;
    }

  if (!this->LightProxy)
    {
    vtkErrorMacro("Light subproxy must be defined in the configuration "
                  "file.");
    return false;
    }

  // Set the servers on which each of the subproxies is to be created.

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
  this->InteractorStyleProxy->SetServers(
    vtkProcessModule::CLIENT); // | vtkProcessModule::RENDER_SERVER);
  this->LightKitProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LightProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return this->Superclass::BeginCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  // Set all the client side pointers.
  this->Renderer = vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(this->RendererProxy->GetID()));
  this->Renderer2D = vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(this->Renderer2DProxy->GetID()));
  this->RenderWindow = vtkRenderWindow::SafeDownCast(
    pm->GetObjectFromID(this->RenderWindowProxy->GetID()));
  this->Interactor = vtkPVGenericRenderWindowInteractor::SafeDownCast(
    pm->GetObjectFromID(this->InteractorProxy->GetID()));
  this->ActiveCamera = vtkCamera::SafeDownCast(
    pm->GetObjectFromID(this->ActiveCameraProxy->GetID()));

  // Set the helper for interaction.
  this->Interactor->SetPVRenderView(this->RenderViewHelper);

  if (pm->GetOptions()->GetUseStereoRendering())
    {
    SetIntVectorProperty(this->RenderWindowProxy, "StereoCapableWindow", 1);
    SetIntVectorProperty(this->RenderWindowProxy, "StereoRender", 1);
    }

  SetIntVectorProperty(this->Renderer2DProxy, "Erase", 0);
  SetIntVectorProperty(this->Renderer2DProxy, "Layer", 2);

  SetIntVectorProperty(this->RendererProxy, "DepthPeeling", 1);
  SetIntVectorProperty(this->RenderWindowProxy, "NumberOfLayers", 3);
   
  this->Connect(this->RendererProxy, this->RenderWindowProxy, "Renderer");
  this->Connect(this->Renderer2DProxy, this->RenderWindowProxy, "Renderer");
  this->Connect(this->RenderWindowProxy, this->InteractorProxy, "RenderWindow");
  this->Connect(this->RendererProxy, this->InteractorProxy, "Renderer");
  this->Connect(this->LightProxy, this->RendererProxy, "Lights");
  this->Connect(this->LightProxy, this->Renderer2DProxy, "Lights");
  this->Connect(this->InteractorStyleProxy, this->InteractorProxy, "InteractorStyle");

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

  // Initialize offscreen rendering.
  // We don't go through the parallel render managers to initialize offscreen,
  // this is because there are some complex interactions happening between the
  // parallel render managers with respect to enabling offscreen rendering. It
  // way simpler this way.
  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    // Non-mesa, X offscreen rendering requires access to the display
    vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
    pm->GatherInformation(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
    if (di->GetCanOpenDisplay())
      {
      vtkClientServerStream stream;
      stream  << vtkClientServerStream::Invoke
              << this->RenderWindowProxy->GetID()
              << "SetOffScreenRendering"
              << 1
              << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID, 
        vtkProcessModule::RENDER_SERVER, stream);

      // Ensure that the client always does onscreen rendering.
      stream  << vtkClientServerStream::Invoke
              << this->RenderWindowProxy->GetID()
              << "SetOffScreenRendering"
              << 0
              << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID, 
        vtkProcessModule::CLIENT, stream);
      }
    di->Delete();
    }

  this->Interactor->Enable();
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
void vtkSMRenderViewProxy::SetLODResolution(int res)
{
  this->Information->Set(LOD_RESOLUTION(), res);
}

//-----------------------------------------------------------------------------
int vtkSMRenderViewProxy::GetLODResolution()
{
  return this->Information->Get(LOD_RESOLUTION());
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SetUseLOD(bool use_lod)
{
  this->Information->Set(USE_LOD(), use_lod);
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::GetUseLOD()
{
  return (this->Information->Get(USE_LOD())>0);
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::GetLODDecision()
{
  return (this->GetVisibileFullResDataSize() >= this->LODThreshold* 1000);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::BeginInteractiveRender()
{
  vtkRenderWindow *renWin = this->GetRenderWindow(); 
  renWin->SetDesiredUpdateRate(5.0);

  bool using_lod = this->GetUseLOD();

  // Determine if we are using LOD or not.
  if (this->GetLODDecision())
    {
    this->SetUseLOD(true);
    if (!using_lod)
      {
      // We changed LOD decision, implying that the LOD pipelines for all
      // representations aren't up-to-date, ensure that they are updated.
      // This can be done by setting the ForceRepresentationUpdate to true
      // as a result of which the superclass will update the representations once 
      // again before performing render.
      this->SetForceRepresentationUpdate(true);
      }
    }
  else
    {
    this->SetUseLOD(false);
    // Even if using_lod was true, we don't need to UpdateAllRepresentations
    // since calling UpdateAllRepresentations when UseLOD is true updates the
    // full-res pipeline anyways.
    }

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

  this->SetUseLOD(false);
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
  SetIntVectorProperty(repr, "UseStrips", this->UseTriangleStrips, false);
  SetIntVectorProperty(repr, "ImmediateModeRendering", this->UseImmediateMode, false);

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
    if (SetIntVectorProperty(repr, "ImmediateModeRendering", val, false))
      {
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
      areaPickerProxy->GetID()
      );
    }

  if (areaPickerProxy)
    {
    areaPickerProxy->Delete();
    }
  
  return propCollectionInfo;
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
// We deliberately use streams for this addition of actor proxies.
// Using property (and clean_command) causes problems with
// 3D widgets (since they get cleaned out when a source is added!).
void vtkSMRenderViewProxy::AddPropToRenderer(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RendererProxy->GetID()
         << "AddViewProp"
         << proxy->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->RendererProxy->GetConnectionID(),
    this->RendererProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AddPropToRenderer2D(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->Renderer2DProxy->GetID()
         << "AddViewProp"
         << proxy->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->RendererProxy->GetConnectionID(),
    this->RendererProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::RemovePropFromRenderer(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RendererProxy->GetID()
         << "RemoveViewProp"
         << proxy->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->RendererProxy->GetConnectionID(),
    this->RendererProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::RemovePropFromRenderer2D(vtkSMProxy* proxy)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->Renderer2DProxy->GetID()
         << "RemoveViewProp"
         << proxy->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->RendererProxy->GetConnectionID(),
    this->RendererProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SynchronizeRenderers()
{
  // Synchronize the camera properties between the 
  // 3D and 2D renders on client and servers.

  if (this->Renderer && this->RendererProxy && 
    this->Renderer2D && this->Renderer2DProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    vtkCamera* pCamera = this->Renderer->GetActiveCamera();
    
    double dArray[3];
    vtkClientServerID renderID = this->Renderer2DProxy->GetID();  
    stream << vtkClientServerStream::Invoke << renderID
      << "GetActiveCamera" <<  vtkClientServerStream::End;
    vtkClientServerID cameraID = pm->GetUniqueID();
    stream << vtkClientServerStream::Assign << cameraID
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;

    pCamera->GetPosition(dArray);
    stream << vtkClientServerStream::Invoke
      << cameraID
      << "SetPosition"
      << dArray[0] << dArray[1] << dArray[2]
      << vtkClientServerStream::End;

    pCamera->GetFocalPoint(dArray);
    stream << vtkClientServerStream::Invoke
      << cameraID
      << "SetFocalPoint"
      << dArray[0] << dArray[1] << dArray[2]
      << vtkClientServerStream::End;

   pCamera->GetViewUp(dArray);
    stream << vtkClientServerStream::Invoke
      << cameraID
      << "SetViewUp"
      << dArray[0] << dArray[1] << dArray[2]
      << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke
      << cameraID
      << "SetParallelProjection"
      << pCamera->GetParallelProjection()
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << cameraID
      << "SetParallelScale"
      << pCamera->GetParallelScale()
      << vtkClientServerStream::End;

    vtkProcessModule::GetProcessModule()->SendStream(
      this->RendererProxy->GetConnectionID(),
      this->RendererProxy->GetServers(), stream);
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

  // Update image extents based on ViewPosition
  int extents[6];
  capture->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] += this->ViewPosition[cc/2]*magnification;
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
int vtkSMRenderViewProxy::IsSelectionAvailable()
{ 
  //check if we are not supposed to turn compositing on (in parallel)
  //the paraview gui uses a big number to say never composite
  //it we are not supposed to turn it on, then disallow selections
  double compThresh = 0;
  vtkSMMultiProcessRenderView *me2 = 
    vtkSMMultiProcessRenderView::SafeDownCast(this);
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
vtkSelection* vtkSMRenderViewProxy::NewSelectionForProp(
  vtkSelection* selection, vtkClientServerID propId)
{
  vtkSelection* newSelection = vtkSelection::New();
  newSelection->GetProperties()->Copy(selection->GetProperties(), 0);
  unsigned int numChildren = selection->GetNumberOfChildren();
  for(unsigned int i=0; i<numChildren; i++)
    {
    vtkSelection* child = selection->GetChild(i);
    vtkInformation* properties = child->GetProperties();
    if (properties->Has(vtkSelection::PROP_ID()) &&
      properties->Get(vtkSelection::PROP_ID()) == (int)propId.ID)
      {
      vtkSelection* newChildSelection = vtkSelection::New();
      newChildSelection->ShallowCopy(child);
      newSelection->AddChild(newChildSelection);
      newChildSelection->Delete();
      }
    }

  return newSelection;
}

//-----------------------------------------------------------------------------
static void vtkSMRenderViewProxyShrinkSelection(vtkSelection* sel)
{
  unsigned int numChildren = sel->GetNumberOfChildren();
  unsigned int cc;
  vtkSmartPointer<vtkSelection> preferredChild;
  int maxPixels = -1;
  for (cc=0; cc < numChildren; cc++)
    {
    vtkSelection* child = sel->GetChild(cc);
    vtkInformation* properties = child->GetProperties();
    if (properties->Has(vtkSelection::PIXEL_COUNT()))
      {
      int numPixels = properties->Get(vtkSelection::PIXEL_COUNT());
      if (numPixels > maxPixels)
        {
        maxPixels = numPixels;
        preferredChild = child;
        }
      }
    }

  if (preferredChild)
    {
    int i = int(numChildren)-1;
    for (; i >=0; i--)
      {
      if (sel->GetChild(i) != preferredChild.GetPointer())
        {
        sel->RemoveChild(i);
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectOnSurface(unsigned int x0, 
  unsigned int y0, unsigned int x1, unsigned int y1,
  vtkCollection* selectedRepresentations/*=0*/,
  vtkCollection* surfaceSelections/*=0*/,
  bool multiple_selections/*=true*/)
{
  // 1) Create surface selection.
  //   Will returns a surface selection in terms of cells selected on the 
  //   visible props from all representations.
  vtkSelection* surfaceSel = this->SelectVisibleCells(x0, y0, x1, y1);

  if (!multiple_selections)
    {
    vtkSMRenderViewProxyShrinkSelection(surfaceSel);
    }

  // 2) Ask each representation to convert the selection.
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Representations->NewIterator());

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDataRepresentationProxy* repr = 
      vtkSMDataRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr)
      {
      continue;
      }
    vtkSMProxy* selectionSource = repr->ConvertSelection(surfaceSel);
    if (!selectionSource)
      {
      continue;
      }

    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      repr->GetProperty("Selection"));
    if (pp)
      {
      pp->RemoveAllProxies();
      pp->AddProxy(selectionSource);
      }
    repr->UpdateProperty("Selection");

    if (selectedRepresentations)
      {
      selectedRepresentations->AddItem(repr);
      }
    if (surfaceSelections)
      {
      surfaceSelections->AddItem(surfaceSel);
      }
    selectionSource->Delete();
    }

  surfaceSel->Delete();
  return true;
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
  vtkSMMultiProcessRenderView *me2 = 
    vtkSMMultiProcessRenderView::SafeDownCast(this);
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

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", 
    "UnstructuredGridRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UnstructuredGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "UniformGridRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UniformGridRepresentation"));
    }

  return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "GeometryRepresentation"));
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderInterruptsEnabled: " << this->RenderInterruptsEnabled 
    << endl;
  os << indent << "ActiveCamera: " << this->ActiveCamera << endl;
  os << indent << "InteractorProxy: " << this->InteractorProxy << endl;
  os << indent << "Interactor: " << this->Interactor << endl;
  os << indent << "Renderer2DProxy: " << this->Renderer2DProxy << endl;
  os << indent << "Renderer2D: " << this->Renderer2D << endl;
  os << indent << "RendererProxy: " << this->RendererProxy << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "MeasurePolygonsPerSecond: " 
    << this->MeasurePolygonsPerSecond << endl;
  os << indent << "AveragePolygonsPerSecond: " 
    << this->AveragePolygonsPerSecond << endl;
  os << indent << "MaximumPolygonsPerSecond: " 
    << this->MaximumPolygonsPerSecond << endl;
  os << indent << "LastPolygonsPerSecond: " 
    << this->LastPolygonsPerSecond << endl;
  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  if (this->OpenGLExtensionsInformation)
    {
    os << endl;
    this->OpenGLExtensionsInformation->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }  
}
