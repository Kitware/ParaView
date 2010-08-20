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
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkIdTypeArray.h"
#include "vtkImageWriter.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInstantiator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataRepresentationProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMHardwareSelector.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropRepresentationProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewHelper.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUtilities.h"
#include "vtkTimerLog.h"
#include "vtkWindowToImageFilter.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>

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
  this->CenterAxesProxy = 0;
  this->OrientationWidgetProxy = 0;
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
  this->UseOffscreenRenderingForScreenshots = 0;
  this->UseInteractiveRenderingForSceenshots = 0;

  this->LODThreshold = 0.0;

  this->OpenGLExtensionsInformation = 0;

  this->SetUseLOD(false);
  this->SetLODResolution(50);
  this->Information->Set(USE_ORDERED_COMPOSITING(), 0);
  this->Information->Set(USE_COMPOSITING(), 0);

  this->LightKitAdded = false;

  this->HardwareSelector = 0;
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
  this->CenterAxesProxy = 0;
  this->OrientationWidgetProxy = 0;

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

  if (this->HardwareSelector)
    {
    this->HardwareSelector->Delete();
    this->HardwareSelector = NULL;
    }
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMRenderViewProxy::NewStrategyInternal(
  int dataType)
{
  if (this->NewStrategyHelper)
    {
    return this->NewStrategyHelper->NewStrategyInternal(dataType);
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;

  if (dataType == VTK_POLY_DATA || dataType == VTK_UNIFORM_GRID || 
    dataType == VTK_IMAGE_DATA)
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
  this->CenterAxesProxy = this->GetSubProxy("CenterAxes");
  this->OrientationWidgetProxy = this->GetSubProxy("OrientationWidget");

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

  if (!this->CenterAxesProxy)
    {
    vtkErrorMacro("CenterAxes subproxy missing.");
    return false;
    }

  if (!this->OrientationWidgetProxy)
    {
    vtkErrorMacro("OrientationWidget subproxy missing.");
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
  this->OrientationWidgetProxy->SetServers(vtkProcessModule::CLIENT);

  return this->Superclass::BeginCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* pvoptions = pm->GetOptions();
  
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
  this->Interactor->AddObserver(vtkCommand::StartInteractionEvent,
    this->GetObserver());
  this->Interactor->AddObserver(vtkCommand::EndInteractionEvent,
    this->GetObserver());

  // Mark 2D renderer as non-interactive, since it's a slave to the 3D renderer.
  this->Renderer2D->SetInteractive(0);

  if (pvoptions->GetUseStereoRendering())
    {
    SetIntVectorProperty(this->RenderWindowProxy, "StereoCapableWindow", 1);
    SetIntVectorProperty(this->RenderWindowProxy, "StereoRender", 1);
    vtkSMEnumerationDomain* domain = vtkSMEnumerationDomain::SafeDownCast(
      this->RenderWindowProxy->GetProperty("StereoType")->GetDomain("enum"));
    if (domain && domain->HasEntryText(pvoptions->GetStereoType()))
      {
      SetIntVectorProperty(this->RenderWindowProxy, "StereoType",
        domain->GetEntryValueForText(pvoptions->GetStereoType()));
      }
    }

  SetIntVectorProperty(this->Renderer2DProxy, "Erase", 0);
  SetIntVectorProperty(this->Renderer2DProxy, "Layer", 2);

  // This property is now exposed, hence it's not safe to modify it.
  //SetIntVectorProperty(this->RendererProxy, "DepthPeeling", 1);
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

  // Synchronize the renderers before any of the parallel render manager code
  // takes control. Otherwise, interprocess communication may become
  // interleaved because iceT as well as this observer make MPI calls.
  this->RenderWindow->AddObserver(vtkCommand::StartEvent, observer, 1);
  this->RenderWindow->AddObserver(vtkCommand::AbortCheckEvent, 
    this->GetObserver());

  // Initialize offscreen rendering.
  // We don't go through the parallel render managers to initialize offscreen,
  // this is because there are some complex interactions happening between the
  // parallel render managers with respect to enabling offscreen rendering. It's
  // way simpler this way.
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(this->ConnectionID);
  if (serverInfo && serverInfo->GetUseOffscreenRendering() )
    {
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->RenderWindowProxy->GetID()
            << "SetOffScreenRendering"
            << 1
            << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER, stream);

    // Ensure that the client always does onscreen rendering, unless we are
    // running in batch mode and offscreen rendering is requested.
    if (pvoptions->GetUseOffscreenRendering() == 0)
      {
      // pvoptions->GetUseOffscreenRendering() can be set only in batch mode.
      stream  << vtkClientServerStream::Invoke
              << this->RenderWindowProxy->GetID()
              << "SetOffScreenRendering"
              << 0
              << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID, 
        vtkProcessModule::CLIENT, stream);
      }
    }

  // Setup and add the center axes.
  vtkSMPropertyHelper(this->CenterAxesProxy, "Scale").Set(0, 0.25);
  vtkSMPropertyHelper(this->CenterAxesProxy, "Scale").Set(1, 0.25);
  vtkSMPropertyHelper(this->CenterAxesProxy, "Scale").Set(2, 0.25);
  vtkSMPropertyHelper(this->CenterAxesProxy, "Pickable").Set(0);
  this->CenterAxesProxy->UpdateVTKObjects();
  this->AddRepresentation(
    vtkSMRepresentationProxy::SafeDownCast(this->CenterAxesProxy));

  this->Interactor->Enable();

  if (this->GetProperty("CenterOfRotation"))
    {
    this->GetProperty("CenterOfRotation")->AddObserver(
      vtkCommand::ModifiedEvent, this->GetObserver());
    }

  // Updates the position and scale for the center axes.
  this->UpdateCenterAxesPositionAndScale();

  vtkPVAxesWidget* orientationWidget = vtkPVAxesWidget::SafeDownCast(
    this->OrientationWidgetProxy->GetClientSideObject());

  if (
    (pm->GetOptions()->GetProcessType() & vtkPVOptions::PVBATCH) == 0 &&
    pm->GetNumberOfLocalPartitions() == 1)
    {
    orientationWidget->SetParentRenderer(this->Renderer);
    orientationWidget->SetViewport(0, 0, 0.25, 0.25);
    orientationWidget->SetInteractor(this->GetInteractor());
    }
  this->OrientationWidgetProxy->UpdateVTKObjects();
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
  else if (eventId == vtkCommand::StartEvent && caller == this->RenderWindow)
    {
    // At the start of every render ensure that the 2D renderer and 3D renderer
    // have the same camera.
    this->SynchronizeRenderers();
    }
  else if (eventId == vtkCommand::ModifiedEvent && caller ==
    this->GetProperty("CenterOfRotation"))
    {
    this->UpdateCenterAxesPositionAndScale();
    }

  // REFER TO BUG #10672.
  // Sending Prepare/Cleanup and start/end on inteaction ensures we don't send
  // repeated progress on/off while interacting, ensuring better performance
  // when interacting.
  else if (eventId == vtkCommand::StartInteractionEvent &&
    caller == this->GetInteractor())
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendPrepareProgress(this->ConnectionID);
    }
  else if (eventId == vtkCommand::EndInteractionEvent &&
    caller == this->GetInteractor())
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendCleanupPendingProgress(this->ConnectionID);
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

  if ((enable==1) == this->LightKitAdded)
    {
    // nothing to do.
    return;
    }

  this->LightKitAdded = enable==1;
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  stream  << vtkClientServerStream::Invoke
          << this->LightKitProxy->GetID()
          << (enable? "AddLightsToRenderer" : "RemoveLightsFromRenderer")
          << this->RendererProxy->GetID()
          << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
    this->LightKitProxy->GetServers(), stream);
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
  if (this->GetUseLOD() != use_lod)
    {
    // we only need to invalidate the DisplayDataSize, but we do both for now.
    this->InvalidateDataSizes();
    }
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

  // Determine if we are using LOD or not
  // This may partially update the representation pipelines to get correct data
  // size information.
  bool use_lod = this->GetLODDecision();
  this->SetUseLOD(use_lod);

  if (use_lod)
    {
    renWin->SetDesiredUpdateRate(5.0);
    }
  else
    {
    renWin->SetDesiredUpdateRate(0.002);
    }

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

  this->ResetCameraClippingRange();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowProxy->GetID()
         << "Render"
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT,
    stream);

  //vtkRenderWindow *renWindow = this->GetRenderWindow(); 
  //renWindow->Render();

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

  if (this->HardwareSelector)
    {
    this->HardwareSelector->ClearBuffers();
    }
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
  double val = this->GetRenderWindow()->GetZbufferDataAtPoint(x,y);
  return val;  
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera()
{
  // Dont' call UpdateAllRepresentations() explicitly, since
  // UpdateAllRepresentations() results in delivering the data to rendering
  // nodes as well. All we want is for the pipeline to be updated until enough
  // information about the bounds can be known. That's already done by
  // vtkSMRepresentationProxy::GetBounds(). So we don't need to call update on
  // representations explicitly here.
  // This fixes BUG #9055.
  // this->UpdateAllRepresentations();

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

  this->UpdateCenterAxesPositionAndScale();

  this->Modified();
  this->InvokeEvent(vtkCommand::ResetCameraEvent);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCameraClippingRange()
{
  double bds[6];
  vtkRenderer* ren = this->GetRenderer();

  this->ComputeVisiblePropBounds(bds);
  ren->ResetCameraClippingRange(bds);
  
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
      double tmp[6];
      if (repr->GetBounds(tmp))
        {
        if (tmp[0] < bds[0]) { bds[0] = tmp[0]; }  
        if (tmp[1] > bds[1]) { bds[1] = tmp[1]; }  
        if (tmp[2] < bds[2]) { bds[2] = tmp[2]; }  
        if (tmp[3] > bds[3]) { bds[3] = tmp[3]; }  
        if (tmp[4] < bds[4]) { bds[4] = tmp[4]; }  
        if (tmp[5] > bds[5]) { bds[5] = tmp[5]; }  
        }
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
  if (vtkProcessModule::GetProcessModule())
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
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::RemovePropFromRenderer2D(vtkSMProxy* proxy)
{
  if (vtkProcessModule::GetProcessModule())
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
void vtkSMRenderViewProxy::SetUseOffscreen(int offscreen)
{
  if (this->ObjectsCreated)
    {
    this->GetRenderWindow()->SetOffScreenRendering(offscreen);
    }
}

//-----------------------------------------------------------------------------
int vtkSMRenderViewProxy::GetUseOffscreen()
{
  if (this->ObjectsCreated)
    {
    return this->GetRenderWindow()->GetOffScreenRendering();
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMRenderViewProxy::CaptureWindow(int magnification)
{
  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.
#if !defined(__APPLE__)
  int useOffscreenRenderingForScreenshots = this->UseOffscreenRenderingForScreenshots;
  int prevOffscreen = this->GetRenderWindow()->GetOffScreenRendering();
  if (useOffscreenRenderingForScreenshots && !prevOffscreen)
    {
    this->GetRenderWindow()->SetOffScreenRendering(1);
    }
#endif

  this->GetRenderWindow()->SwapBuffersOff();

  if(this->UseInteractiveRenderingForSceenshots)
    {
    this->InteractiveRender();
    }
  else
    {
    this->StillRender();
    }


  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();

  // BUG #8715: We go through this indirection since the active connection needs
  // to be set during update since it may request re-renders if magnification >1.
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << w2i << "Update"
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT,
    stream);

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  w2i->Delete();

  this->GetRenderWindow()->SwapBuffersOn();
  this->GetRenderWindow()->Frame();

#if !defined(__APPLE__)
  if (useOffscreenRenderingForScreenshots && !prevOffscreen)
    {
    this->GetRenderWindow()->SetOffScreenRendering(0);
    }

  if (useOffscreenRenderingForScreenshots)
    {
    vtkDataArray* scalars = capture->GetPointData()->GetScalars();
    bool invalid_image = true;
    for (int comp=0; comp < scalars->GetNumberOfComponents(); comp++)
      {
      double range[2];
      scalars->GetRange(range, comp);
      if (range[0] != 0.0 || range[1] != 0.0)
        {
        invalid_image = false;
        break;
        }
      }

    if (invalid_image && 
      vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions() == 1)
      {
      // free up current image.
      capture->Delete();
      capture = 0;
      vtkWarningMacro("Disabling offscreen rendering since empty image was detected.");
      this->UseOffscreenRenderingForScreenshots = false;
      if (prevOffscreen)
        {
        this->GetRenderWindow()->SetOffScreenRendering(0);
        }
      return this->CaptureWindow(magnification);
      }
    }
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
int vtkSMRenderViewProxy::WriteImage(const char* filename, int magnification)
{
  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));
  return vtkSMUtilities::SaveImage(shot, filename);
}

//-----------------------------------------------------------------------------
int vtkSMRenderViewProxy::WriteImage(const char* filename,
  const char* writerName, int magnification)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));

  if (vtkProcessModule::GetProcessModule()->GetOptions()->GetSymmetricMPIMode())
    {
    return vtkSMUtilities::SaveImageOnProcessZero(shot, filename, writerName);
    }
  else
    {
    return vtkSMUtilities::SaveImage(shot, filename, writerName);
    }
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
      vtkPVDataInformation* info = repr->GetRepresentedDataInformation(true);
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
bool vtkSMRenderViewProxy::IsSelectionAvailable()
{
  const char* msg = this->IsSelectVisibleCellsAvailable();
  if (msg)
    {
    vtkErrorMacro(<< msg);
    return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisibleCellsAvailable()
{ 
  //check if we don't have enough color depth to do color buffer selection
  //if we don't then disallow selection
  int rgba[4];
  vtkRenderWindow *rwin = this->GetRenderWindow();
  if (!rwin)
    {
    return "No render window available";
    }

  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    return "Selection not supported due to insufficient color depth.";
    }

  //yeah!
  return NULL;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisiblePointsAvailable()
{
  return this->IsSelectVisibleCellsAvailable();
}

//-----------------------------------------------------------------------------
vtkSelection* vtkSMRenderViewProxy::NewSelectionForProp(
  vtkSelection* selection, vtkClientServerID propId)
{
  vtkSelection* newSelection = vtkSelection::New();
  unsigned int numNodes = selection->GetNumberOfNodes();
  for(unsigned int i=0; i<numNodes; i++)
    {
    vtkSelectionNode* node = selection->GetNode(i);
    vtkInformation* properties = node->GetProperties();
    if (properties->Has(vtkSelectionNode::PROP_ID()) &&
      properties->Get(vtkSelectionNode::PROP_ID()) == (int)propId.ID)
      {
      vtkSelectionNode* newNode = vtkSelectionNode::New();
      newNode->ShallowCopy(node);
      newSelection->AddNode(newNode);
      newNode->Delete();
      }
    }

  return newSelection;
}

//-----------------------------------------------------------------------------
static void vtkSMRenderViewProxyShrinkSelection(vtkSelection* sel)
{
  vtkstd::map<int, int> propToPixelCount;
  
  unsigned int numNodes = sel->GetNumberOfNodes();
  unsigned int cc;

  int choosenPropId = -1;
  int maxPixels = -1;
  for (cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);
    vtkInformation* properties = node->GetProperties();
    if (properties->Has(vtkSelectionNode::PIXEL_COUNT()) && 
      properties->Has(vtkSelectionNode::PROP_ID()))
      {
      int numPixels = properties->Get(vtkSelectionNode::PIXEL_COUNT());
      int prop_id = properties->Get(vtkSelectionNode::PROP_ID());
      if (propToPixelCount.find(prop_id) != propToPixelCount.end())
        {
        numPixels += propToPixelCount[prop_id];
        }

      propToPixelCount[prop_id] = numPixels;
      if (numPixels > maxPixels)
        {
        maxPixels = numPixels;
        choosenPropId = prop_id;
        }
      }
    }

  vtkstd::vector<vtkSmartPointer<vtkSelectionNode> > choosenNodes;
  if (choosenPropId != -1)
    {
    for (cc=0; cc < numNodes; cc++)
      {
      vtkSelectionNode* node = sel->GetNode(cc);
      vtkInformation* properties = node->GetProperties();
      if (properties->Has(vtkSelectionNode::PROP_ID()) && 
        properties->Get(vtkSelectionNode::PROP_ID()) == choosenPropId)
        {
        choosenNodes.push_back(node);
        }
      }
    }
  sel->RemoveAllNodes();
  for (cc=0; cc <choosenNodes.size(); cc++)
    {
    sel->AddNode(choosenNodes[cc]);
    }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectOnSurface(unsigned int x0, 
  unsigned int y0, unsigned int x1, unsigned int y1,
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  vtkCollection* surfaceSelections,
  bool multiple_selections/*=true*/,
  bool ofPoints/*=false*/)
{
  // 1) Create surface selection.
  //   Will returns a surface selection in terms of cells selected on the 
  //   visible props from all representations.
  vtkSelection* surfaceSel = this->SelectVisibleCells(x0, y0, x1, y1, ofPoints);
  // cout << surfaceSel->GetNumberOfChildren() << endl;

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

    if (surfaceSelections)
      {
      surfaceSelections->AddItem(surfaceSel);
      }
    selectionSources->AddItem(selectionSource);
    selectedRepresentations->AddItem(repr);

    selectionSource->Delete();
    }

  surfaceSel->Delete();
  return true;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::Pick(unsigned int x, unsigned int y)
{
  // 1) Create surface selection.
  //   Will returns a surface selection in terms of cells selected on the
  //   visible props from all representations.
  vtkSmartPointer<vtkSelection> surfaceSel;
  surfaceSel.TakeReference(this->SelectVisibleCells(x, y, x, y, false));
  vtkSMRenderViewProxyShrinkSelection(surfaceSel);

  if (surfaceSel->GetNumberOfNodes() == 0)
    {
    return NULL;
    }

  vtkClientServerID propID(
    surfaceSel->GetNode(0)->GetProperties()->Get(vtkSelectionNode::PROP_ID()));

  vtkProp3D* prop = vtkProp3D::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(propID));

  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Representations->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMPropRepresentationProxy* repr =
      vtkSMPropRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (repr && repr->HasVisibleProp3D(prop))
      {
      return repr;
      }
    }

  return NULL;
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustum(unsigned int x0, 
                                         unsigned int y0, unsigned int x1, unsigned int y1,
                                         vtkCollection* selectedRepresentations,
                                         vtkCollection* selectionSources,
                                         vtkCollection* frustumSelections,
                                         bool multiple_selections,
                                         bool ofPoints/*=false*/)
{
  int displayRectangle[4] = {x0, y0, x1, y1};
  if (displayRectangle[0] == displayRectangle[2])
    {
    displayRectangle[2] += 1;
    }
  if (displayRectangle[1] == displayRectangle[3])
    {
    displayRectangle[3] += 1;
    }

  // 1) Create frustum selection 
  vtkDoubleArray *frustcorners = vtkDoubleArray::New();
  frustcorners->SetNumberOfComponents(4);
  frustcorners->SetNumberOfTuples(8);
  //convert screen rectangle to world frustum
  vtkRenderer *renderer = this->GetRenderer();
  double worldP[32]; 
  int index=0;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&worldP[index*4]);
  frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1], 
    worldP[index*4+2], worldP[index*4+3]);

  vtkSelection* frustumSel = vtkSelection::New();
  vtkSelectionNode* frustumNode = vtkSelectionNode::New();
  frustumSel->AddNode(frustumNode);
  frustumNode->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::FRUSTUM);
  if (ofPoints)
    {
    frustumNode->GetProperties()->Set(
      vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
    }
  frustumNode->SetSelectionList(frustcorners);
  frustcorners->Delete();

  // 2) Figure out which representation is "selected".
  vtkExtractSelectedFrustum* FrustumExtractor = 
    vtkExtractSelectedFrustum::New();
  FrustumExtractor->CreateFrustum(worldP);
  double bounds[6];

  vtkSelection* frustumParent = vtkSelection::New();

  // Now we just use the first selected representation,
  // until we have other mechanisms to select one.
  vtkSmartPointer<vtkCollectionIterator> reprIter;
  reprIter.TakeReference(this->Representations->NewIterator());

  for (reprIter->InitTraversal(); 
    !reprIter->IsDoneWithTraversal(); reprIter->GoToNextItem())
    {
    vtkSMDataRepresentationProxy* repr = 
      vtkSMDataRepresentationProxy::SafeDownCast(reprIter->GetCurrentObject());
    if (!repr || !repr->GetVisibility())
      {
      continue;
      }
    if (repr->GetProperty("Pickable") &&
      vtkSMPropertyHelper(repr, "Pickable").GetAsInt() == 0)
      {
      // skip non-pickable representations.
      continue;
      }
    vtkPVDataInformation* datainfo = repr->GetRepresentedDataInformation(true);
    if (!datainfo)
      {
      continue;
      }
    datainfo->GetBounds(bounds);

    if(FrustumExtractor->OverallBoundsTest(bounds))
      {
      frustumParent->AddNode(frustumNode);

      vtkSMProxy* selectionSource = repr->ConvertSelection(frustumParent);
      if (!selectionSource)
        {
        continue;
        }
      selectionSources->AddItem(selectionSource);
      if (frustumSelections)
        {
        frustumSelections->AddItem(frustumSel);
        }
      // Add the found repr, and exit for loop
      selectedRepresentations->AddItem(repr);

      selectionSource->Delete();
      if(!multiple_selections)
        {
        break;
        }
      }
    }

  frustumSel->Delete();
  frustumNode->Delete();
  frustumParent->Delete();
  FrustumExtractor->Delete();
  return true;
}

//-----------------------------------------------------------------------------
vtkSelection* vtkSMRenderViewProxy::SelectVisibleCells(unsigned int x0,
  unsigned int y0, unsigned int x1, unsigned int y1, int ofPoints)
{
  if (!this->IsSelectionAvailable())
    {
    vtkSelection *selection = vtkSelection::New();
    selection->Initialize();
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

    vtkPVDataInformation *gi = repr->GetRepresentedDataInformation(true);
    if (!gi)
      {
      continue;
      }
    vtkIdType numCells = ofPoints? gi->GetNumberOfPoints() : gi->GetNumberOfCells();
    if (numCells > maxNumCells)
      {
      maxNumCells = numCells;
      }
    }
  iter->Delete();

  if (this->HardwareSelector == NULL)
    {
    vtkSMProxyManager* proxyManager = vtkSMObject::GetProxyManager();
    this->HardwareSelector = vtkSMHardwareSelector::SafeDownCast(
      proxyManager->NewProxy("PropPickers", "HardwareSelector"));
    this->HardwareSelector->SetConnectionID(this->ConnectionID);
    this->HardwareSelector->SetServers(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  vtkSMHardwareSelector *vcsProxy = this->HardwareSelector;

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

  // Set default property values for the selector.
  // if any of the properties here change, then the hardware selector ensures
  // that it clears the old buffers.
  //int area[4] = {x0, y0, x1, y1};
  int area[4] = {0, 0, win_size[0] - 1, win_size[1] - 1};
  vtkSMPropertyHelper(vcsProxy, "Renderer").Set(this->RendererProxy);
  vtkSMPropertyHelper(vcsProxy, "Area").Set(area, 4);
  vtkSMPropertyHelper(vcsProxy, "FieldAssociation").Set(ofPoints? 0 : 1);
  vtkSMPropertyHelper(vcsProxy, "NumberOfProcesses").Set(numProcessors);
  vtkSMPropertyHelper(vcsProxy, "NumberOfIDs").Set(maxNumCells);
  vcsProxy->UpdateVTKObjects();

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

  unsigned int region[4] = {x0, y0, x1, y1};
  vtkSelection* selection = vcsProxy->Select(region);

  //Turn stripping back on if we had turned it off
  if (use_strips)
    {
    this->SetUseTriangleStrips(1);
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

  if (setAllowBuffSwap != NULL)
    {
    //let the RenderSyncManager control back/front buffer swapping
    setAllowBuffSwap->SetElements1(1);
    renderSyncManager->UpdateVTKObjects();
    }

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

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    sproxy->UpdatePipeline(this->GetViewUpdateTime());
    }

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

  prototype = pxm->GetPrototypeProxy("representations",
    "GeometryRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "GeometryRepresentation"));
    }

  vtkPVXMLElement* hints = source->GetHints();
  if (hints)
    {
    // If the source has an hint as follows, then it's a text producer and must
    // be is display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>

    unsigned int numElems = hints->GetNumberOfNestedElements(); 
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      if (child->GetName() &&
        strcmp(child->GetName(), "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) &&
        index == opport &&
        child->GetAttribute("type") &&
        strcmp(child->GetAttribute("type"), "text") == 0)
        {
        return vtkSMRepresentationProxy::SafeDownCast(
          pxm->NewProxy("representations", "TextSourceRepresentation"));
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::GetSuggestedViewType(vtkIdType connectionID)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Default is built-in connection
  const char* renderViewName = "RenderView";
  if (pm->IsRemote(connectionID))
    {
    // Client-server
    vtkPVServerInformation* server_info = pm->GetServerInformation(
      connectionID);
    if (server_info && server_info->GetUseIceT())
      {
      // With ice-t
      if (server_info->GetTileDimensions()[0] )
        {
        // tiled-display
        renderViewName = "IceTMultiDisplayRenderView";
        }
      else if (server_info->GetNumberOfMachines())
        {
        renderViewName = "CaveRenderView";
        }
      else
        {
        // regular client-server
        renderViewName = "IceTDesktopRenderView";
        }
      } 
    else
      {
      // This fallback render module does not handle parallel rendering or tile
      // display, but it will handle remote serial rendering and multiple views.
      renderViewName = "ClientServerRenderView";
      }
    }
  else if (pm->GetNumberOfPartitions(connectionID) > 1) 
    {
    // MPI but not client-server. pvbatch and disconnected server generating
    // animation.
    renderViewName = "IceTCompositeView";
    }
  
  return renderViewName;
}

namespace
{
  static double vtkMAX(double x, double y) { return x > y ? x : y; }
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::UpdateCenterAxesPositionAndScale()
{
  // Position of the axes is same as the center of rotation.
  double center[3];
  vtkSMPropertyHelper(this, "CenterOfRotation").Get(center, 3);
  vtkSMPropertyHelper(this->CenterAxesProxy, "Position").Set(center, 3);

  // FIXME: We may want to look at the 3D widget code to automatically scale the
  // axes instead of computing the bounds.
  // Reset size of the axes.
  double bounds[6];
  this->ComputeVisiblePropBounds(bounds);

  double widths[3];
  widths[0] = (bounds[1]-bounds[0]);
  widths[1] = (bounds[3]-bounds[2]);
  widths[2] = (bounds[5]-bounds[4]);

  // lets make some thickness in all directions
  double diameterOverTen = vtkMAX(widths[0], vtkMAX(widths[1], widths[2])) / 10.0;
  widths[0] = widths[0] < diameterOverTen ? diameterOverTen : widths[0];
  widths[1] = widths[1] < diameterOverTen ? diameterOverTen : widths[1];
  widths[2] = widths[2] < diameterOverTen ? diameterOverTen : widths[2];

  widths[0] *= 0.25;
  widths[1] *= 0.25;
  widths[2] *= 0.25;
  vtkSMPropertyHelper(this->CenterAxesProxy, "Scale").Set(widths, 3);
  this->CenterAxesProxy->UpdateVTKObjects();
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
  os << indent << "UseOffscreenRenderingForScreenshots: "
    << this->UseOffscreenRenderingForScreenshots << endl;
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
