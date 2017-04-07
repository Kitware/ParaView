/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderView.h"

#include "vtkPVConfig.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractMapper.h"
#include "vtkAlgorithmOutput.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkCuller.h"
#include "vtkDataRepresentation.h"
#include "vtkFXAAOptions.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkInteractorStyleDrawPolygon.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkInteractorStyleRubberBandZoom.h"
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkMPIMoveData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVCameraCollection.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVGridAxes3DActor.h"
#include "vtkPVHardwareSelector.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVOptions.h"
#include "vtkPVSession.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVTrackballMultiRotate.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkPVTrackballZoomToMouse.h"
#include "vtkPartitionOrdering.h"
#include "vtkPartitionOrderingInterface.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTextRepresentation.h"
#include "vtkTimerLog.h"
#include "vtkTrackballPan.h"
#include "vtkTrivialProducer.h"
#include "vtkVector.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

#ifdef VTKGL2
#include "vtkLightingMapPass.h"
#include "vtkValuePass.h"
#else
#include "vtkValuePasses.h"
#endif

#ifdef PARAVIEW_USE_ICE_T
#include "vtkIceTSynchronizedRenderers.h"
#endif

#ifdef PARAVIEW_USE_OSPRAY
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#endif

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <vector>

class vtkPVRenderView::vtkInternals
{
  std::map<int, vtkWeakPointer<vtkPVDataRepresentation> > PropMap;

public:
#ifdef VTKGL2
  vtkNew<vtkValuePass> ValuePasses;
  vtkNew<vtkLightingMapPass> LightingMapPass;
#else
  vtkNew<vtkValuePasses> ValuePasses;
#endif
#ifdef PARAVIEW_USE_OSPRAY
  vtkNew<vtkOSPRayPass> OSPRayPass;
#endif
  vtkSmartPointer<vtkRenderPass> SavedRenderPass;
  int FieldAssociation;
  int FieldAttributeType;
  std::string FieldName;
  bool FieldNameSet;
  int Component;
  double ScalarRange[2];
  bool ScalarRangeSet;
  bool SavedOrientationState;
  bool SavedAnnotationState;
  bool IsInCapture;
  bool IsInOSPRay;
  bool OSPRayShadows;
  int OSPRayCount;
  vtkNew<vtkFloatArray> ArrayHolder;
  vtkNew<vtkWindowToImageFilter> ZGrabber;

  vtkNew<vtkPVDataDeliveryManager> DeliveryManager;

  void RegisterSelectionProp(int id, vtkProp*, vtkPVDataRepresentation* rep)
  {
    this->PropMap[id] = rep;
  }
  void UnRegisterSelectionProp(vtkProp* prop, vtkPVDataRepresentation* rep)
  {
    (void)prop;
    (void)rep;
  }

  vtkPVDataRepresentation* GetRepresentationForPropId(int id)
  {
    std::map<int, vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter = this->PropMap.find(id);
    return (iter != this->PropMap.end() ? iter->second : NULL);
  }

  void PreRender(vtkRenderViewBase* vtkNotUsed(renderView)) {}
};

namespace
{
// In multi-process rendering modes, data delivered to a set of ranks is not
// not cleared until the data pipeline updates. This avoids having to
// redeliver data when simply switching between remote and local rendering
// modes. Now consider this case. You're rendering a Sphere in client-server
// mode with remote rendering disabled; the geometry is delivered to the
// client. Next, you switch to force remote-rendering; the geometry delivered
// to the client is still there and hence will get rendered, even through
// we'll replace the pixels with those obtained from the server. Hence, we
// need to cull the props in the composited renderer on the local process when
// it's not participating in the compositing. This culler enables us to do
// that. We also add support to add props to the DoNotCullList since the
// vtkPVGridAxes3DActor is an exception to this rule.
class vtkPVRendererCuller : public vtkCuller
{
public:
  static vtkPVRendererCuller* New();
  vtkTypeMacro(vtkPVRendererCuller, vtkCuller);

  virtual double Cull(vtkRenderer* vtkNotUsed(ren), vtkProp** propList, int& listLength,
    int& initialized) VTK_OVERRIDE
  {
    double total_time = 0;
    if (listLength <= 0)
    {
      return total_time;
    }
    double* allocatedTimeList = new double[listLength];
    for (int propLoop = 0; propLoop < listLength; ++propLoop)
    {
      // Get the prop out of the list
      vtkProp* prop = propList[propLoop];
      double prop_time = initialized ? prop->GetRenderTimeMultiplier() : 1.0;
      if (this->RenderOnLocalProcess == false &&
        this->DoNotCullList.find(prop) == this->DoNotCullList.end())
      {
        prop_time = 0;
      }
      prop->SetRenderTimeMultiplier(prop_time);
      allocatedTimeList[propLoop] = prop_time;
      total_time += prop_time;
    }

    // ========================================================================
    // The following code is borrowed from vtkFrustumCoverageCuller.
    // ========================================================================
    // Now traverse the list from the beginning, swapping any zero entries back
    // in the list, while preserving the order of the non-zero entries. This
    // requires two indices for the two items we are comparing at any step.
    // The second index always moves back by one, but the first index moves back
    // by one only when it is pointing to something that has a non-zero value.
    int index1 = 0, index2 = 0;
    for (index2 = 1; index2 < listLength; index2++)
    {
      if (allocatedTimeList[index1] == 0.0)
      {
        if (allocatedTimeList[index2] != 0.0)
        {
          allocatedTimeList[index1] = allocatedTimeList[index2];
          propList[index1] = propList[index2];
          propList[index2] = NULL;
          allocatedTimeList[index2] = 0.0;
        }
        else
        {
          propList[index1] = propList[index2] = NULL;
          allocatedTimeList[index1] = allocatedTimeList[index2] = 0.0;
        }
      }
      if (allocatedTimeList[index1] != 0.0)
      {
        index1++;
      }
    }
    // Compute the new list length - index1 is always pointing to the
    // first 0.0 entry or the last entry if none were zero (in which case
    // we won't change the list length)
    listLength = (allocatedTimeList[index1] == 0.0) ? (index1) : listLength;
    delete[] allocatedTimeList;
    return total_time;
  }

  vtkSetMacro(RenderOnLocalProcess, bool);
  vtkGetMacro(RenderOnLocalProcess, bool);

  // List of props to never cull.
  std::set<void*> DoNotCullList;

private:
  vtkPVRendererCuller()
    : RenderOnLocalProcess(false)
  {
  }
  ~vtkPVRendererCuller() {}
  bool RenderOnLocalProcess;
};
vtkStandardNewMacro(vtkPVRendererCuller);

#if defined(VTKGL2) && defined(PARAVIEW_USE_ICE_T)
//------------------------------------------------------------------------------
// vtkIceTCompositePass needs to know the set RenderPass will read from its
// internal float FBO in order to setup an adequate context.
void IceTPassEnableFloatPass(bool enable, vtkPVSynchronizedRenderer* sr)
{
  vtkIceTSynchronizedRenderers* iceTRen =
    vtkIceTSynchronizedRenderers::SafeDownCast(sr->GetParallelSynchronizer());

  if (iceTRen)
  {
    vtkIceTCompositePass* iceTPass = iceTRen->GetIceTCompositePass();
    if (iceTPass)
    {
      iceTPass->SetEnableFloatValuePass(enable);
    }
  }
};
#endif

//----------------------------------------------------------------------------
// This is used in vtkPVRenderView::Update() to change all zoom manipulators to
// zoom (and not dolly) when a discrete set of cameras are provided. This is
// essential since dolly changes camera position.
void vtkUpdateTrackballZoomManipulators(
  vtkPVInteractorStyle* style, bool useDollyForPerspectiveProjection)
{
  if (style == NULL)
  {
    return;
  }

  vtkCollection* manips = style->GetCameraManipulators();
  for (int cc = 0, max = manips->GetNumberOfItems(); cc < max; ++cc)
  {
    vtkPVTrackballZoom* zoom = vtkPVTrackballZoom::SafeDownCast(manips->GetItemAsObject(cc));
    if (zoom)
    {
      zoom->SetUseDollyForPerspectiveProjection(useDollyForPerspectiveProjection);
    }
  }
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderView);
vtkInformationKeyMacro(vtkPVRenderView, USE_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, USE_OUTLINE_FOR_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, LOD_RESOLUTION, Double);
vtkInformationKeyMacro(vtkPVRenderView, NEED_ORDERED_COMPOSITING, Integer);
vtkInformationKeyMacro(vtkPVRenderView, RENDER_EMPTY_IMAGES, Integer);
vtkInformationKeyMacro(vtkPVRenderView, REQUEST_STREAMING_UPDATE, Request);
vtkInformationKeyMacro(vtkPVRenderView, REQUEST_PROCESS_STREAMED_PIECE, Request);
vtkInformationKeyRestrictedMacro(vtkPVRenderView, VIEW_PLANES, DoubleVector, 24);

vtkCxxSetObjectMacro(vtkPVRenderView, LastSelection, vtkSelection);

#define VTK_STEREOTYPE_SAME_AS_CLIENT 0

//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
  : Annotation()
  , StereoType(VTK_STEREO_RED_BLUE)
  , ServerStereoType(VTK_STEREOTYPE_SAME_AS_CLIENT)
{
  this->Internals = new vtkInternals();
  this->Internals->FieldAssociation = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
  this->Internals->FieldNameSet = false;
  this->Internals->FieldAttributeType = 0;
  this->Internals->Component = 0;
  this->Internals->ScalarRangeSet = false;
  this->Internals->ScalarRange[0] = 0.0;
  this->Internals->ScalarRange[1] = -1.0;
  this->Internals->IsInCapture = false;
  this->Internals->IsInOSPRay = false;
  this->Internals->OSPRayShadows = false;
  this->Internals->OSPRayCount = 0;

  // non-reference counted, so no worries about reference loops.
  this->Internals->DeliveryManager->SetRenderView(this);

  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();

  this->RemoteRenderingAvailable = true;

  this->LockBounds = false;

  this->StillRenderProcesses = vtkPVSession::NONE;
  this->InteractiveRenderProcesses = vtkPVSession::NONE;
  this->UsedLODForLastRender = false;
  this->UseLODForInteractiveRender = false;
  this->UseDistributedRenderingForStillRender = false;
  this->UseDistributedRenderingForInteractiveRender = false;
  this->MakingSelection = false;
  this->PreviousSwapBuffers = 0;
  this->StillRenderImageReductionFactor = 1;
  this->InteractiveRenderImageReductionFactor = 2;
  this->RemoteRenderingThreshold = 0;
  this->LODRenderingThreshold = 0;
  this->LODResolution = 0.5;
  this->UseOutlineForLODRendering = false;
  this->UseLightKit = false;
  this->Interactor = 0;
  this->InteractorStyle = 0;
  this->TwoDInteractorStyle = 0;
  this->ThreeDInteractorStyle = 0;
  this->DiscreteCameras = 0;
  this->PolygonStyle = 0;
  this->RubberBandStyle = 0;
  this->RubberBandZoom = 0;
  this->CenterAxes = vtkPVCenterAxesActor::New();
  this->CenterAxes->SetComputeNormals(0);
  this->CenterAxes->SetPickable(0);
  this->CenterAxes->SetScale(0.25, 0.25, 0.25);
  this->OrientationWidget = vtkPVAxesWidget::New();
  this->InteractionMode = INTERACTION_MODE_UNINTIALIZED;
  this->LastSelection = NULL;
  this->UseOffscreenRenderingForScreenshots = false;
  this->UseInteractiveRenderingForScreenshots = false;
  this->UseOffscreenRendering = (options->GetUseOffscreenRendering() != 0);
  this->EGLDeviceIndex = options->GetEGLDeviceIndex();
  this->Selector = vtkPVHardwareSelector::New();
  this->NeedsOrderedCompositing = false;
  this->RenderEmptyImages = false;
  this->UseFXAA = false;
  this->DistributedRenderingRequired = false;
  this->NonDistributedRenderingRequired = false;
  this->DistributedRenderingRequiredLOD = false;
  this->NonDistributedRenderingRequiredLOD = false;
  this->ParallelProjection = 0;
  this->Culler = vtkSmartPointer<vtkPVRendererCuller>::New();
  this->ForceDataDistributionMode = -1;
  this->PreviousDiscreteCameraIndex = -1;

  this->SynchronizedRenderers = vtkPVSynchronizedRenderer::New();

  vtkRenderWindow* window = this->SynchronizedWindows->NewRenderWindow();
  window->SetMultiSamples(0);
  window->SetOffScreenRendering(this->UseOffscreenRendering ? 1 : 0);
  window->SetDeviceIndex(this->EGLDeviceIndex);

  this->RenderView = vtkRenderViewBase::New();
  this->RenderView->SetRenderWindow(window);
  window->Delete();

  this->NonCompositedRenderer = vtkRenderer::New();
  this->NonCompositedRenderer->EraseOff();
  this->NonCompositedRenderer->InteractiveOff();
  this->NonCompositedRenderer->SetLayer(2);
  this->NonCompositedRenderer->SetActiveCamera(this->RenderView->GetRenderer()->GetActiveCamera());
  window->AddRenderer(this->NonCompositedRenderer);
  window->SetNumberOfLayers(3);
  this->RenderView->GetRenderer()->GetActiveCamera()->ParallelProjectionOff();

  vtkMemberFunctionCommand<vtkPVRenderView>* observer =
    vtkMemberFunctionCommand<vtkPVRenderView>::New();
  observer->SetCallback(*this, &vtkPVRenderView::ResetCameraClippingRange);
  this->GetRenderer()->AddObserver(vtkCommand::ResetCameraClippingRangeEvent, observer);
  observer->FastDelete();

  this->UseHiddenLineRemoval = false;
  this->GetRenderer()->SetUseDepthPeeling(1);
  this->GetRenderer()->AddCuller(this->Culler);

  this->Light = vtkLight::New();
  this->Light->SetAmbientColor(1, 1, 1);
  this->Light->SetSpecularColor(1, 1, 1);
  this->Light->SetDiffuseColor(1, 1, 1);
  this->Light->SetIntensity(1.0);
  this->Light->SetLightType(2); // CameraLight
  this->LightKit = vtkLightKit::New();
  this->GetRenderer()->AddLight(this->Light);
  this->GetRenderer()->SetAutomaticLightCreation(0);

  // Setup interactor styles. Since these are only needed on the process that
  // the users interact with, we only create it on the "driver" process.
  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
  {
    this->InteractorStyle = // Default one will be the 3D
      this->ThreeDInteractorStyle = vtkPVInteractorStyle::New();
    this->TwoDInteractorStyle = vtkPVInteractorStyle::New();

    // Add some default manipulators. Applications can override them without
    // much ado.
    vtkPVTrackballRotate* manip = vtkPVTrackballRotate::New();
    manip->SetButton(1);
    this->ThreeDInteractorStyle->AddManipulator(manip);
    manip->Delete();

    vtkPVTrackballZoom* manip2 = vtkPVTrackballZoom::New();
    manip2->SetButton(3);
    this->ThreeDInteractorStyle->AddManipulator(manip2);
    manip2->Delete();

    vtkTrackballPan* manip3 = vtkTrackballPan::New();
    manip3->SetButton(2);
    this->ThreeDInteractorStyle->AddManipulator(manip3);
    manip3->Delete();

    vtkTrackballPan* manip4 = vtkTrackballPan::New();
    manip4->SetButton(1);
    this->TwoDInteractorStyle->AddManipulator(manip4);
    manip4->Delete();

    vtkPVTrackballZoom* manip5 = vtkPVTrackballZoom::New();
    manip5->SetButton(3);
    this->TwoDInteractorStyle->AddManipulator(manip5);
    manip5->Delete();

    this->RubberBandStyle = vtkInteractorStyleRubberBand3D::New();
    this->RubberBandStyle->RenderOnMouseMoveOff();
    vtkCommand* observer2 =
      vtkMakeMemberFunctionCommand(*this, &vtkPVRenderView::OnSelectionChangedEvent);
    this->RubberBandStyle->AddObserver(vtkCommand::SelectionChangedEvent, observer2);
    observer2->Delete();

    this->RubberBandZoom = vtkInteractorStyleRubberBandZoom::New();
    this->PolygonStyle = vtkInteractorStyleDrawPolygon::New();
    vtkCommand* observer3 =
      vtkMakeMemberFunctionCommand(*this, &vtkPVRenderView::OnPolygonSelectionEvent);
    this->PolygonStyle->AddObserver(vtkCommand::SelectionChangedEvent, observer3);
    observer3->Delete();
  }

  this->OrientationWidget->SetParentRenderer(this->GetRenderer());
  this->OrientationWidget->SetViewport(0, 0, 0.25, 0.25);

  this->GetRenderer()->AddActor(this->CenterAxes);

  this->SetInteractionMode(INTERACTION_MODE_3D);

  this->NonCompositedRenderer->AddActor(this->Annotation.GetPointer());
  this->Annotation->SetRenderer(this->NonCompositedRenderer);
  this->Annotation->SetText("Annotation");
  this->Annotation->BuildRepresentation();
  this->Annotation->SetWindowLocation(vtkTextRepresentation::UpperLeftCorner);
  this->Annotation->GetTextActor()->SetTextScaleModeToNone();
  this->Annotation->GetTextActor()->GetTextProperty()->SetJustificationToLeft();
  this->Annotation->SetVisibility(0);
  this->ShowAnnotation = false;
  this->UpdateAnnotation = true;

  // We update the annotation text before the 2D renderer renders.
  this->NonCompositedRenderer->AddObserver(
    vtkCommand::StartEvent, this, &vtkPVRenderView::UpdateAnnotationText);
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
#if defined(VTKGL2)
  vtkRenderWindow* win = this->RenderView->GetRenderWindow();
  if (win)
  {
    this->Internals->ValuePasses->ReleaseGraphicsResources(win);
  }
#endif

  // this ensure that the renderer releases graphics resources before the window
  // is destroyed.
  this->GetRenderWindow()->RemoveRenderer(this->NonCompositedRenderer);
  this->GetRenderWindow()->RemoveRenderer(this->GetRenderer());
  this->GetNonCompositedRenderer()->SetRenderWindow(0);
  this->GetRenderer()->SetRenderWindow(0);

  this->SetLastSelection(NULL);
  this->Selector->Delete();
  this->SynchronizedRenderers->Delete();
  this->NonCompositedRenderer->Delete();
  this->RenderView->Delete();
  this->LightKit->Delete();
  this->Light->Delete();
  this->CenterAxes->Delete();
  this->OrientationWidget->Delete();
  this->Interactor = 0;

  if (this->InteractorStyle)
  {
    // Don't want to delete it as it is only pointing to either
    // [TwoDInteractorStyle, ThreeDInteractorStyle]
    this->InteractorStyle = 0;
  }
  if (this->TwoDInteractorStyle)
  {
    this->TwoDInteractorStyle->Delete();
    this->TwoDInteractorStyle = 0;
  }
  if (this->ThreeDInteractorStyle)
  {
    this->ThreeDInteractorStyle->Delete();
    this->ThreeDInteractorStyle = 0;
  }
  if (this->RubberBandStyle)
  {
    this->RubberBandStyle->Delete();
    this->RubberBandStyle = 0;
  }
  if (this->RubberBandZoom)
  {
    this->RubberBandZoom->Delete();
    this->RubberBandZoom = 0;
  }
  if (this->PolygonStyle)
  {
    this->PolygonStyle->Delete();
    this->PolygonStyle = 0;
  }

  this->Internals->SavedRenderPass = NULL;

  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
vtkPVDataDeliveryManager* vtkPVRenderView::GetDeliveryManager()
{
  return this->Internals->DeliveryManager.GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetUseOffscreenRendering(bool use_offscreen)
{
  if (this->UseOffscreenRendering == use_offscreen)
  {
    return;
  }

  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  bool process_use_offscreen = options->GetUseOffscreenRendering() != 0;

  this->UseOffscreenRendering = use_offscreen || process_use_offscreen;
  this->GetRenderWindow()->SetOffScreenRendering(this->UseOffscreenRendering);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetEGLDeviceIndex(int deviceIndex)
{
  this->EGLDeviceIndex = deviceIndex;
  this->GetRenderWindow()->SetDeviceIndex(deviceIndex);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Initialize(unsigned int id)
{
  if (this->Identifier == id)
  {
    // already initialized
    return;
  }
  this->SynchronizedWindows->AddRenderWindow(id, this->RenderView->GetRenderWindow());
  this->SynchronizedWindows->AddRenderer(id, this->RenderView->GetRenderer());
  this->SynchronizedWindows->AddRenderer(id, this->GetNonCompositedRenderer());
  this->SynchronizedWindows->AddRenderer(id, this->OrientationWidget->GetRenderer());

  this->SynchronizedRenderers->Initialize(this->SynchronizedWindows->GetSession(), id);
  this->SynchronizedRenderers->SetRenderer(this->RenderView->GetRenderer());

  this->Superclass::Initialize(id);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AddRepresentationInternal(vtkDataRepresentation* rep)
{
  vtkPVDataRepresentation* dataRep = vtkPVDataRepresentation::SafeDownCast(rep);
  if (dataRep != NULL)
  {
    this->Internals->DeliveryManager->RegisterRepresentation(dataRep);
  }

  this->Superclass::AddRepresentationInternal(rep);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{
  vtkPVDataRepresentation* dataRep = vtkPVDataRepresentation::SafeDownCast(rep);
  if (dataRep != NULL)
  {
    this->Internals->DeliveryManager->UnRegisterRepresentation(dataRep);
  }

  this->Superclass::RemoveRepresentationInternal(rep);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RegisterPropForHardwareSelection(vtkPVDataRepresentation* repr, vtkProp* prop)
{
  int id = this->Selector->AssignUniqueId(prop);
  this->Internals->RegisterSelectionProp(id, prop, repr);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UnRegisterPropForHardwareSelection(
  vtkPVDataRepresentation* repr, vtkProp* prop)
{
  this->Internals->UnRegisterSelectionProp(prop, repr);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AddPropToRenderer(vtkProp* prop)
{
  this->GetRenderer()->AddViewProp(prop);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemovePropFromRenderer(vtkProp* prop)
{
  this->GetRenderer()->RemoveViewProp(prop);
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPVRenderView::GetRenderer(int rendererType)
{
  switch (rendererType)
  {
    case NON_COMPOSITED_RENDERER:
      return this->GetNonCompositedRenderer();
    case DEFAULT_RENDERER:
      return this->RenderView->GetRenderer();
    default:
      return NULL;
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetActiveCamera(vtkCamera* camera)
{
  this->GetRenderer()->SetActiveCamera(camera);
  this->GetNonCompositedRenderer()->SetActiveCamera(camera);
  if (camera)
  {
    camera->SetParallelProjection(this->ParallelProjection);
  }
}

//----------------------------------------------------------------------------
vtkCamera* vtkPVRenderView::GetActiveCamera()
{
  return this->RenderView->GetRenderer()->GetActiveCamera();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVRenderView::GetRenderWindow()
{
  return this->RenderView->GetRenderWindow();
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkPVRenderView::GetInteractor()
{
  return this->Interactor;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  if (this->GetLocalProcessSupportsInteraction() == false)
  {
    // We don't setup interactor on non-driver processes.
    return;
  }

  if (this->Interactor != iren)
  {
    this->Interactor = iren;
    this->OrientationWidget->SetInteractor(this->Interactor);
    if (this->Interactor)
    {
      this->Interactor->SetRenderWindow(this->GetRenderWindow());

      // this will set the interactor style.
      int mode = this->InteractionMode;
      this->InteractionMode = INTERACTION_MODE_UNINTIALIZED;
      this->SetInteractionMode(mode);
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
  {
    this->InteractionMode = mode;
    this->Modified();

    if (this->Interactor == NULL)
    {
      return;
    }

    switch (this->InteractionMode)
    {
      case INTERACTION_MODE_3D:
        this->Interactor->SetInteractorStyle(this->InteractorStyle = this->ThreeDInteractorStyle);
        // Get back to the previous state
        this->GetActiveCamera()->SetParallelProjection(this->ParallelProjection);
        break;
      case INTERACTION_MODE_2D:
        this->Interactor->SetInteractorStyle(this->InteractorStyle = this->TwoDInteractorStyle);
        this->GetActiveCamera()->SetParallelProjection(1);
        break;

      case INTERACTION_MODE_SELECTION:
        this->Interactor->SetInteractorStyle(this->RubberBandStyle);
        break;

      case INTERACTION_MODE_POLYGON:
        this->Interactor->SetInteractorStyle(this->PolygonStyle);
        break;

      case INTERACTION_MODE_ZOOM:
        this->Interactor->SetInteractorStyle(this->RubberBandZoom);
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetGridAxes3DActor(vtkPVGridAxes3DActor* gridActor)
{
  if (this->GridAxes3DActor != gridActor)
  {
    vtkPVRendererCuller* culler = vtkPVRendererCuller::SafeDownCast(this->Culler.GetPointer());
    // we currently don't support grid axes in tile-display mode.
    const bool in_tile_display_mode = this->InTileDisplayMode();
    if (this->GridAxes3DActor)
    {
      this->GetNonCompositedRenderer()->RemoveViewProp(this->GridAxes3DActor);
      this->GetRenderer()->RemoveViewProp(this->GridAxes3DActor);
      culler->DoNotCullList.erase(this->GridAxes3DActor);
    }
    this->GridAxes3DActor = gridActor;
    if (this->GridAxes3DActor && !in_tile_display_mode)
    {
      this->GetNonCompositedRenderer()->AddViewProp(this->GridAxes3DActor);
      this->GetRenderer()->AddViewProp(this->GridAxes3DActor);

      this->GridAxes3DActor->SetEnableLayerSupport(true);
      this->GridAxes3DActor->SetBackgroundLayer(this->GetRenderer()->GetLayer());
      this->GridAxes3DActor->SetGeometryLayer(this->GetRenderer()->GetLayer());
      this->GridAxes3DActor->SetForegroundLayer(this->GetNonCompositedRenderer()->GetLayer());
      culler->DoNotCullList.insert(this->GridAxes3DActor);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::OnSelectionChangedEvent()
{
  int region[4];
  this->RubberBandStyle->GetStartPosition(&region[0]);
  this->RubberBandStyle->GetEndPosition(&region[2]);

  // NOTE: This gets called on the driver i.e. client or root-node in batch mode.
  // That's not necessarily the node on which the selection can be made, since
  // data may not be on this process.

  // selection is a data-selection (not geometry selection).
  int ordered_region[4];
  ordered_region[0] = region[0] < region[2] ? region[0] : region[2];
  ordered_region[2] = region[0] > region[2] ? region[0] : region[2];
  ordered_region[1] = region[1] < region[3] ? region[1] : region[3];
  ordered_region[3] = region[1] > region[3] ? region[1] : region[3];

  this->InvokeEvent(vtkCommand::SelectionChangedEvent, ordered_region);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::OnPolygonSelectionEvent()
{
  // NOTE: This gets called on the driver i.e. client or root-node in batch mode.
  // That's not necessarily the node on which the selection can be made, since
  // data may not be on this process.
  // selection is a data-selection (not geometry selection).
  std::vector<vtkVector2i> points = this->PolygonStyle->GetPolygonPoints();
  if (points.size() >= 3)
  {
    vtkNew<vtkIntArray> polygonPointsArray;
    polygonPointsArray->SetNumberOfComponents(2);
    polygonPointsArray->SetNumberOfTuples(points.size());
    for (unsigned int j = 0; j < points.size(); ++j)
    {
      const vtkVector2i& v = points[j];
      int pos[2] = { v[0], v[1] };
      polygonPointsArray->SetTypedTuple(j, pos);
    }

    this->InvokeEvent(vtkCommand::SelectionChangedEvent, polygonPointsArray.GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPoints(int region[4])
{
  this->Select(vtkDataObject::FIELD_ASSOCIATION_POINTS, region);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectCells(int region[4])
{
  this->Select(vtkDataObject::FIELD_ASSOCIATION_CELLS, region);
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::PrepareSelect(int fieldAssociation)
{
  // NOTE: selection is only supported in builtin or client-server mode. Not
  // supported in tile-display or batch modes.

  if (this->MakingSelection)
  {
    vtkErrorMacro("Select was called while making another selection.");
    return false;
  }

  this->MakingSelection = true;
  // disable buffer swapping during selection to avoid clobbering the users
  // view (BUG #16042).
  this->PreviousSwapBuffers = this->GetRenderWindow()->GetSwapBuffers();
  this->GetRenderWindow()->SwapBuffersOff();

  // Make sure that the representations are up-to-date. This is required since
  // due to delayed-swicth-back-from-lod, the most recent render maybe a LOD
  // render (or a nonremote render) in which case we need to update the
  // representation pipelines correctly.
  this->Render(/*interactive*/ false, /*skip-rendering*/ false);

  this->SetLastSelection(NULL);

  this->Selector->SetRenderer(this->GetRenderer());
  this->Selector->SetFieldAssociation(fieldAssociation);
  this->Selector->SetSynchronizedWindows(this->SynchronizedWindows);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Select(int fieldAssociation, int region[4])
{
  if (!this->PrepareSelect(fieldAssociation))
  {
    return;
  }
  vtkSmartPointer<vtkSelection> sel;
  if (this->SynchronizedWindows->GetEnabled() ||
    this->SynchronizedWindows->GetLocalProcessIsDriver())
  {
    // we don't render labels for hardware selection
    this->NonCompositedRenderer->SetDraw(false);
    sel.TakeReference(this->Selector->Select(region));
    this->NonCompositedRenderer->SetDraw(true);
  }
  this->PostSelect(sel);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::PostSelect(vtkSelection* sel)
{
  if (this->SynchronizedWindows->GetLocalProcessIsDriver() && sel)
  {
    // valid selection is only generated on the driver process. Other's are
    // merely rendering the passes so that the result is composited correctly.
    this->FinishSelection(sel);
  }
  // look at ::Render(..,..). We need to disable these once we are done with
  // rendering.
  this->SynchronizedWindows->SetEnabled(false);
  this->SynchronizedRenderers->SetEnabled(false);

  this->MakingSelection = false;
  this->GetRenderWindow()->SetSwapBuffers(this->PreviousSwapBuffers);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPolygonPoints(int* polygonPoints, vtkIdType arrayLen)
{
  this->SelectPolygon(vtkDataObject::FIELD_ASSOCIATION_POINTS, polygonPoints, arrayLen);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPolygonCells(int* polygonPoints, vtkIdType arrayLen)
{
  this->SelectPolygon(vtkDataObject::FIELD_ASSOCIATION_CELLS, polygonPoints, arrayLen);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPolygon(int fieldAssociation, int* polygonPoints, vtkIdType arrayLen)
{
  if (!this->PrepareSelect(fieldAssociation))
  {
    return;
  }
  vtkSmartPointer<vtkSelection> sel;
  if (this->SynchronizedWindows->GetEnabled() ||
    this->SynchronizedWindows->GetLocalProcessIsDriver())
  {
    sel.TakeReference(this->Selector->PolygonSelect(polygonPoints, arrayLen));
  }
  this->PostSelect(sel);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::FinishSelection(vtkSelection* sel)
{
  assert(sel != NULL);

  // first, convert local-prop ids to ids that the data-server can understand if
  // the selection was done without compositing, i.e. rendering happened on the
  // client side.
  for (unsigned int cc = 0; cc < sel->GetNumberOfNodes(); cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node->GetProperties()->Has(vtkSelectionNode::PROP_ID()))
    {
      int propid = node->GetProperties()->Get(vtkSelectionNode::PROP_ID());
      vtkPVDataRepresentation* repr = this->Internals->GetRepresentationForPropId(propid);
      if (repr)
      {
        node->GetProperties()->Set(vtkSelectionNode::SOURCE(), repr);
      }
    }
  }

  this->SetLastSelection(sel);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLockBounds(bool nv)
{
  if (this->LockBounds == nv)
  {
    return;
  }
  this->LockBounds = nv;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetMaxClipBounds(double* bounds)
{
  this->GeometryBounds.SetBounds(bounds);
  this->GetRenderer()->ResetCameraClippingRange(bounds);
  this->GetNonCompositedRenderer()->ResetCameraClippingRange(bounds);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  if (this->GeometryBounds.IsValid() && !this->LockBounds && this->DiscreteCameras == NULL)
  {
    double bounds[6];
    this->GeometryBounds.GetBounds(bounds);
    this->GetRenderer()->ResetCameraClippingRange(bounds);
    this->GetNonCompositedRenderer()->ResetCameraClippingRange(bounds);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SynchronizeGeometryBounds()
{
  vtkBoundingBox bbox;
  bbox.AddBox(this->GeometryBounds);

  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
  {
    // get local bounds to consider 3D widgets correctly.
    // if ComputeVisiblePropBounds is called when there's no real window on the
    // local process, all vtkWidgetRepresentations return wacky Z bounds which
    // screws up the renderer and we don't see any images. Hence we only do this
    // on the driver nodes. There will always be a render window on the driver
    // nodes.

    this->CenterAxes->SetUseBounds(0);
    if (this->GridAxes3DActor)
    {
      this->GridAxes3DActor->SetUseBounds(0);
    }
    double prop_bounds[6];
    this->GetRenderer()->ComputeVisiblePropBounds(prop_bounds);
    this->CenterAxes->SetUseBounds(1);
    if (this->GridAxes3DActor)
    {
      this->GridAxes3DActor->SetUseBounds(1);
    }

    bbox.AddBounds(prop_bounds);
  }

  // sync up bounds across all processes when doing distributed rendering.
  double bounds[6];
  bbox.GetBounds(bounds);
  this->SynchronizedWindows->SynchronizeBounds(bounds);

  if (!vtkMath::AreBoundsInitialized(bounds))
  {
    this->GeometryBounds.SetBounds(-1, 1, -1, 1, -1, 1);
  }
  else
  {
    this->GeometryBounds.SetBounds(bounds);
  }

  this->UpdateCenterAxes();
  this->ResetCameraClippingRange();
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetLocalProcessDoesRendering(bool using_distributed_rendering)
{

  switch (vtkProcessModule::GetProcessType())
  {
    case vtkProcessModule::PROCESS_DATA_SERVER:
      return false;

    case vtkProcessModule::PROCESS_CLIENT:
      return true;

    default:
      return using_distributed_rendering || this->InTileDisplayMode() || this->InCaveDisplayMode();
  }
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVRenderView::ResetCamera()
{
  // Since ResetCamera() is accessible via a property on the view proxy, this
  // method gets called directly (and on on the vtkSMRenderViewProxy). Hence
  // we need to ensure things are updated explicitly and cannot rely on the View
  // proxy to take care of updating the view.
  this->Update();

  // Remember, vtkRenderer::ResetCamera() calls
  // vtkRenderer::ResetCameraClippingPlanes() with the given bounds.
  double bounds[6];
  this->GeometryBounds.GetBounds(bounds);
  if (!this->LockBounds)
  {
    this->RenderView->GetRenderer()->ResetCamera(bounds);
  }

  this->InvokeEvent(vtkCommand::ResetCameraEvent);
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVRenderView::ResetCamera(double bounds[6])
{
  // Remember, vtkRenderer::ResetCamera() calls
  // vtkRenderer::ResetCameraClippingPlanes() with the given bounds.
  if (!this->LockBounds)
  {
    this->RenderView->GetRenderer()->ResetCamera(bounds);
  }
  this->InvokeEvent(vtkCommand::ResetCameraEvent);
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::TestCollaborationCounter()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSession* activeSession = vtkPVSession::SafeDownCast(pm->GetActiveSession());
  if (!activeSession || !activeSession->IsMultiClients())
  {
    return true;
  }

  vtkMultiProcessController* p_controller = this->SynchronizedWindows->GetParallelController();
  vtkMultiProcessController* d_controller =
    this->SynchronizedWindows->GetClientDataServerController();
  vtkMultiProcessController* r_controller = this->SynchronizedWindows->GetClientServerController();
  if (d_controller != NULL)
  {
    vtkErrorMacro("RenderServer-DataServer configuration is not supported in "
                  "multi-clients mode. Please restart ParaView in the right mode. "
                  "Aborting since this could cause deadlocks and other issues.");
    abort();
  }

  if (this->SynchronizedWindows->GetMode() == vtkPVSynchronizedRenderWindows::CLIENT)
  {
    int magicNumber = this->GetDeliveryManager()->GetSynchronizationMagicNumber();
    r_controller->Send(&magicNumber, 1, 1, 41000);
    int server_sync_counter;
    r_controller->Receive(&server_sync_counter, 1, 1, 41001);
    return (server_sync_counter == magicNumber);
  }
  else
  {
    bool counterSynchronizedSuccessfully = false;
    if (r_controller)
    {
      int client_sync_counter;
      int magicNumber = this->GetDeliveryManager()->GetSynchronizationMagicNumber();
      r_controller->Receive(&client_sync_counter, 1, 1, 41000);
      r_controller->Send(&magicNumber, 1, 1, 41001);
      counterSynchronizedSuccessfully = (client_sync_counter == magicNumber);
    }

    if (p_controller)
    {
      p_controller->Broadcast(&this->RemoteRenderingThreshold, 1, 0);
      int temp = counterSynchronizedSuccessfully ? 1 : 0;
      p_controller->Broadcast(&temp, 1, 0);
      counterSynchronizedSuccessfully = (temp == 1);
    }
    return counterSynchronizedSuccessfully;
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SynchronizeForCollaboration()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSession* activeSession = vtkPVSession::SafeDownCast(pm->GetActiveSession());
  if (!activeSession || !activeSession->IsMultiClients())
  {
    return;
  }

  // Update decisions about lod-rendering and remote-rendering.

  vtkMultiProcessController* p_controller = this->SynchronizedWindows->GetParallelController();
  vtkMultiProcessController* r_controller = this->SynchronizedWindows->GetClientServerController();

  if (this->SynchronizedWindows->GetMode() == vtkPVSynchronizedRenderWindows::CLIENT)
  {
    vtkMultiProcessStream stream;
    stream << (this->UseLODForInteractiveRender ? 1 : 0)
           << (this->UseDistributedRenderingForStillRender ? 1 : 0)
           << (this->UseDistributedRenderingForInteractiveRender ? 1 : 0)
           << this->StillRenderProcesses << this->InteractiveRenderProcesses;
    r_controller->Send(stream, 1, 42000);
  }
  else
  {
    vtkMultiProcessStream stream;
    if (r_controller)
    {
      r_controller->Receive(stream, 1, 42000);
    }
    if (p_controller)
    {
      p_controller->Broadcast(stream, 0);
    }
    int arg1, arg2, arg3;
    stream >> arg1 >> arg2 >> arg3 >> this->StillRenderProcesses >>
      this->InteractiveRenderProcesses;
    this->UseLODForInteractiveRender = (arg1 == 1);
    this->UseDistributedRenderingForStillRender = (arg2 == 1);
    this->UseDistributedRenderingForInteractiveRender = (arg3 == 1);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Update()
{
  vtkTimerLog::MarkStartEvent("RenderView::Update");

  // reset the bounds, so that representations can provide us with bounds
  // information during update.
  this->GeometryBounds.Reset();

  // reset flags that representations set in REQUEST_UPDATE() pass.
  this->DistributedRenderingRequired = false;
  this->NonDistributedRenderingRequired = false;
  this->ForceDataDistributionMode = -1;

  this->PartitionOrdering->SetImplementation(NULL);

  // clear discrete interaction style state.
  this->DiscreteCameras = NULL;
  this->PreviousDiscreteCameraIndex = -1;

  this->Superclass::Update();

  // Update camera zoom manipulators based on whether we have discrete position.
  vtkUpdateTrackballZoomManipulators(this->TwoDInteractorStyle, this->DiscreteCameras == NULL);
  vtkUpdateTrackballZoomManipulators(this->ThreeDInteractorStyle, this->DiscreteCameras == NULL);

  // After every update we can expect the representation geometries to change.
  // Thus we need to determine whether we are doing to remote-rendering or not,
  // use-lod or not, etc. All these decisions are made right here to avoid
  // making them during each render-call.

  // Check if any representation told us:
  // 1 needed ordered compositing
  // 2 needed render empty images
  this->NeedsOrderedCompositing = false;
  this->RenderEmptyImages = false;
  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  for (int cc = 0; cc < num_reprs; cc++)
  {
    vtkInformation* info = this->ReplyInformationVector->GetInformationObject(cc);
    if (info->Has(NEED_ORDERED_COMPOSITING()) && (info->Get(NEED_ORDERED_COMPOSITING()) != 0))
    {
      this->NeedsOrderedCompositing = true;
    }
    else if (info->Has(RENDER_EMPTY_IMAGES()) && (info->Get(RENDER_EMPTY_IMAGES()) != 0))
    {
      this->RenderEmptyImages = true;
    }
  }

  // Gather information about geometry sizes from all representations.
  double local_size = this->GetDeliveryManager()->GetVisibleDataSize(false) / 1024.0;
  this->SynchronizedWindows->SynchronizeSize(local_size);
  // cout << "Full Geometry size: " << local_size << endl;

  // Update decisions about lod-rendering and remote-rendering.
  this->UseLODForInteractiveRender = this->ShouldUseLODRendering(local_size);
  this->UseDistributedRenderingForStillRender =
    this->ShouldUseDistributedRendering(local_size, /*using_lod=*/false);
  if (!this->UseLODForInteractiveRender)
  {
    this->UseDistributedRenderingForInteractiveRender = this->UseDistributedRenderingForStillRender;
  }

  this->StillRenderProcesses = this->InteractiveRenderProcesses = vtkPVSession::CLIENT;
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->InCaveDisplayMode();
  if (in_tile_display_mode || in_cave_mode || this->UseDistributedRenderingForStillRender)
  {
    this->StillRenderProcesses = vtkPVSession::CLIENT_AND_SERVERS;
  }
  if (in_tile_display_mode || in_cave_mode || this->UseDistributedRenderingForInteractiveRender)
  {
    this->InteractiveRenderProcesses = vtkPVSession::CLIENT_AND_SERVERS;
  }

  // Synchronize data bounds.
  this->SynchronizeGeometryBounds();

  vtkTimerLog::MarkEndEvent("RenderView::Update");

  this->UpdateTimeStamp.Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CopyViewUpdateOptions(vtkPVRenderView* otherView)
{
  this->NeedsOrderedCompositing = otherView->NeedsOrderedCompositing;
  this->RenderEmptyImages = otherView->RenderEmptyImages;
  this->UseLODForInteractiveRender = otherView->UseLODForInteractiveRender;
  this->UseDistributedRenderingForStillRender = otherView->UseDistributedRenderingForStillRender;
  this->StillRenderProcesses = otherView->StillRenderProcesses;
  this->InteractiveRenderProcesses = otherView->InteractiveRenderProcesses;
  this->UseDistributedRenderingForInteractiveRender =
    otherView->UseDistributedRenderingForInteractiveRender;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateLOD()
{
  vtkTimerLog::MarkStartEvent("RenderView::UpdateLOD");

  // Update LOD geometry.

  this->RequestInformation->Set(LOD_RESOLUTION(), this->LODResolution);
  if (this->UseOutlineForLODRendering)
  {
    this->RequestInformation->Set(USE_OUTLINE_FOR_LOD(), 1);
  }

  // reset flags that representations set in REQUEST_UPDATE_LOD() pass.
  this->DistributedRenderingRequiredLOD = false;
  this->NonDistributedRenderingRequiredLOD = false;

  this->CallProcessViewRequest(
    vtkPVView::REQUEST_UPDATE_LOD(), this->RequestInformation, this->ReplyInformationVector);

  double local_size = this->GetDeliveryManager()->GetVisibleDataSize(true) / 1024.0;
  this->SynchronizedWindows->SynchronizeSize(local_size);
  // cout << "LOD Geometry size: " << local_size << endl;

  this->UseDistributedRenderingForInteractiveRender =
    this->ShouldUseDistributedRendering(local_size, /*using_lod=*/true);

  this->InteractiveRenderProcesses = vtkPVSession::CLIENT;
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->InCaveDisplayMode();
  if (in_tile_display_mode || in_cave_mode || this->UseDistributedRenderingForInteractiveRender)
  {
    this->InteractiveRenderProcesses = vtkPVSession::CLIENT_AND_SERVERS;
  }

  vtkTimerLog::MarkEndEvent("RenderView::UpdateLOD");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StillRender()
{
  vtkTimerLog::MarkStartEvent("Still Render");
  this->GetRenderWindow()->SetDesiredUpdateRate(0.002);

  this->Internals->PreRender(this->RenderView);

  this->Render(false, false);

  vtkTimerLog::MarkEndEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::InteractiveRender()
{
  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->GetRenderWindow()->SetDesiredUpdateRate(5.0);

  this->Internals->OSPRayCount = 0;
  this->Internals->PreRender(this->RenderView);

  this->Render(true, false);

  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Render(bool interactive, bool skip_rendering)
{
  this->UpdateStereoProperties();

  if (this->SynchronizedWindows->GetMode() != vtkPVSynchronizedRenderWindows::CLIENT ||
    (!interactive && this->UseDistributedRenderingForStillRender) ||
    (interactive && this->UseDistributedRenderingForInteractiveRender))
  {
    // in multi-client modes, Render() will be called on client always. Now the
    // client need to coordinate with server only when we are remote rendering.
    // if Render() is called on server side, then we are indeed remote
    // rendering, irrespective of what the local flags tell us and we need to
    // coordinate with the client to update the local flags correctly.

    // Although Selection do trigger a Render on the server side and in such
    // case we MUST NOT execute that collaboration synchronization
    if (!this->MakingSelection)
    {
      this->SynchronizeForCollaboration();
    }
  }

  // BUG #13534. Reset the clip planes on every render. Since this does not
  // involve any communication, doing this on every render is not a big deal.
  this->ResetCameraClippingRange();

  if (this->DiscreteCameras != NULL)
  {
    vtkCamera* camera = this->GetActiveCamera();
    int index = this->DiscreteCameras->FindClosestCamera(camera);
    if (interactive == false || this->PreviousDiscreteCameraIndex != index)
    {
      this->PreviousDiscreteCameraIndex = index;
      this->DiscreteCameras->UpdateCamera(index, camera);
    }
  }
  else
  {
    this->PreviousDiscreteCameraIndex = -1;
  }

  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->InCaveDisplayMode();
  if (in_cave_mode && !this->RemoteRenderingAvailable)
  {
    static bool warned_once = false;
    if (!warned_once)
    {
      vtkErrorMacro("In Cave mode and Display cannot be opened on server-side! "
                    "Ensure the environment is set correctly in the pvx file.");
      in_cave_mode = false;
    }
  }

  // Use loss-less image compression for client-server for full-res renders.
  this->SynchronizedRenderers->SetLossLessCompression(!interactive);

  bool use_lod_rendering = interactive ? this->GetUseLODForInteractiveRender() : false;
  if (use_lod_rendering)
  {
    this->RequestInformation->Set(USE_LOD(), 1);
  }

  // cout << "Using remote rendering: " << use_distributed_rendering << endl;

  // Decide if we are doing remote rendering or local rendering.
  bool use_distributed_rendering = interactive
    ? this->GetUseDistributedRenderingForInteractiveRender()
    : this->GetUseDistributedRenderingForStillRender();

  if (this->GetUseOrderedCompositing())
  {
    if (this->PartitionOrdering->GetImplementation() == NULL ||
      this->PartitionOrdering->GetImplementation()->IsA("vtkPKdTree"))
    {
      this->Internals->DeliveryManager->RedistributeDataForOrderedCompositing(use_lod_rendering);
      this->PartitionOrdering->SetImplementation(this->Internals->DeliveryManager->GetKdTree());
    }
    else
    {
      this->Internals->DeliveryManager->ClearRedistributedData(use_lod_rendering);
    }
    this->SynchronizedRenderers->SetPartitionOrdering(this->PartitionOrdering.GetPointer());
  }
  else
  {
    this->SynchronizedRenderers->SetPartitionOrdering(NULL);
  }

  // enable render empty images if it was requested
  this->SynchronizedRenderers->SetRenderEmptyImages(this->GetRenderEmptyImages());

  // Render each representation with available geometry.
  // This is the pass where representations get an opportunity to get the
  // currently "available" represented data and try to render it.
  this->CallProcessViewRequest(
    vtkPVView::REQUEST_RENDER(), this->RequestInformation, this->ReplyInformationVector);

  // set the image reduction factor.
  this->SynchronizedRenderers->SetImageReductionFactor(
    (interactive ? this->InteractiveRenderImageReductionFactor
                 : this->StillRenderImageReductionFactor));

  this->UsedLODForLastRender = use_lod_rendering;

  if (skip_rendering)
  {
    // essential to restore state.
    return;
  }

  // When in tile-display mode, we are always doing shared rendering. However
  // when use_distributed_rendering we tell IceT that geometry is duplicated on
  // all processes.
  this->SynchronizedWindows->SetEnabled(
    use_distributed_rendering || in_tile_display_mode || in_cave_mode);
  this->SynchronizedRenderers->SetEnabled(
    use_distributed_rendering || in_tile_display_mode || in_cave_mode);
  this->SynchronizedRenderers->SetDataReplicatedOnAllProcesses(
    in_cave_mode || (!use_distributed_rendering && in_tile_display_mode));

  if (this->UseHiddenLineRemoval && !use_distributed_rendering)
  {
    this->GetRenderer()->SetUseHiddenLineRemoval(true);
  }
  else
  { // Ignore for distributed rendering.
    this->GetRenderer()->SetUseHiddenLineRemoval(false);
  }

  // Configure FXAA. Disable for picking, as it mucks up the selection buffers.
  bool use_fxaa = this->UseFXAA && !this->MakingSelection;
  if (this->SynchronizedRenderers->GetEnabled())
  {
    this->SynchronizedRenderers->SetUseFXAA(use_fxaa);
    this->SynchronizedRenderers->SetFXAAOptions(this->FXAAOptions.Get());
    // Disable the renderer's FXAA implementation when rendering remotely. We
    // need to run it on the composed image to avoid seam artifacts.
    this->RenderView->GetRenderer()->SetUseFXAA(false);
  }
  else
  {
    this->RenderView->GetRenderer()->SetUseFXAA(use_fxaa);
    this->RenderView->GetRenderer()->SetFXAAOptions(this->FXAAOptions.Get());
  }
  this->OrientationWidget->GetRenderer()->SetUseFXAA(use_fxaa);
  this->OrientationWidget->GetRenderer()->SetFXAAOptions(this->FXAAOptions.Get());

  if (this->ShowAnnotation)
  {
    std::ostringstream stream;
    stream << "Mode: " << (interactive ? "interactive" : "still") << "\n"
           << "Level-of-detail: " << (use_lod_rendering ? "yes" : "no") << "\n"
           << "Remote/parallel rendering: " << (use_distributed_rendering ? "yes" : "no") << "\n";
    this->Annotation->SetText(stream.str().c_str());
  }

  // Determine if local process is rendering data for the composited renderer.
  vtkPVRendererCuller* culler = vtkPVRendererCuller::SafeDownCast(this->Culler.GetPointer());
  culler->SetRenderOnLocalProcess(
    this->IsProcessRenderingGeometriesForCompositing(use_distributed_rendering));

  // When in batch mode, we are using the same render window for all views. That
  // makes it impossible for vtkPVSynchronizedRenderWindows to identify which
  // view is being rendered. We explicitly mark the view being rendered using
  // this HACK.
  this->SynchronizedWindows->BeginRender(this->GetIdentifier());

  // Call Render() on local render window only if
  // 1: Local process is the driver OR
  // 2: RenderEventPropagation is Off and we are doing distributed rendering.
  // 3: In tile-display mode or cave-mode.
  // Note, ParaView no longer has RenderEventPropagation ON. It's set to off
  // always.
  if ((this->SynchronizedWindows->GetLocalProcessIsDriver() ||
        (!this->SynchronizedWindows->GetRenderEventPropagation() && use_distributed_rendering) ||
        in_tile_display_mode || in_cave_mode) &&
    vtkProcessModule::GetProcessType() != vtkProcessModule::PROCESS_DATA_SERVER)
  {
    this->AboutToRenderOnLocalProcess(interactive);
    if (!this->MakingSelection)
    {
      this->Timer->StartTimer();
    }
    this->GetRenderWindow()->Render();
    if (!this->MakingSelection)
    {
      this->Timer->StopTimer();
    }
  }

  if (!this->MakingSelection)
  {
    // If we are making selection, then it's a multi-step render process and we
    // need to leave the SynchronizedWindows/SynchronizedRenderers enabled for
    // that entire process.
    this->SynchronizedWindows->SetEnabled(false);
    this->SynchronizedRenderers->SetEnabled(false);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Deliver(int use_lod, unsigned int size, unsigned int* representation_ids)
{
  // if in multi-clients mode, ensure that processes are in the same "state"
  // before doing the data delivery or we may end up with dead-locks due to
  // mismatched representations.
  if (!this->TestCollaborationCounter())
  {
    return;
  }

  // in multi-clients mode, for every Deliver() and Render() call, we obtain the
  // remote-rendering related ivars from the client.
  this->SynchronizeForCollaboration();

  this->GetDeliveryManager()->Deliver(use_lod, size, representation_ids);
}

//----------------------------------------------------------------------------
int vtkPVRenderView::GetDataDistributionMode(bool use_remote_rendering)
{
  if (this->ForceDataDistributionMode >= 0)
  {
    // data-distribution mode is being overridden (experimental)
    return this->ForceDataDistributionMode;
  }

  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->InCaveDisplayMode();
  if (in_cave_mode)
  {
    return vtkMPIMoveData::CLONE;
  }

  if (use_remote_rendering)
  {
    return in_tile_display_mode ? vtkMPIMoveData::COLLECT_AND_PASS_THROUGH
                                : vtkMPIMoveData::PASS_THROUGH;
  }

  return in_tile_display_mode ? vtkMPIMoveData::CLONE : vtkMPIMoveData::COLLECT;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPiece(vtkInformation* info, vtkPVDataRepresentation* repr,
  vtkDataObject* data, unsigned long trueSize /*=0*/, int port)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  view->GetDeliveryManager()->SetPiece(repr, data, false, trueSize, port);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVRenderView::GetPieceProducer(
  vtkInformation* info, vtkPVDataRepresentation* repr, int port)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
  }

  return view->GetDeliveryManager()->GetProducer(repr, false, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPieceLOD(
  vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* data, int port)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  view->GetDeliveryManager()->SetPiece(repr, data, true, port);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVRenderView::GetPieceProducerLOD(
  vtkInformation* info, vtkPVDataRepresentation* repr, int port)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
  }

  return view->GetDeliveryManager()->GetProducer(repr, true, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::MarkAsRedistributable(
  vtkInformation* info, vtkPVDataRepresentation* repr, bool value /*=true*/, int port)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  view->GetDeliveryManager()->MarkAsRedistributable(repr, value, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetStreamable(vtkInformation* info, vtkPVDataRepresentation* repr, bool val)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  view->GetDeliveryManager()->SetStreamable(repr, val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrderedCompositingInformation(vtkInformation* info,
  vtkPVDataRepresentation* repr, vtkExtentTranslator* translator, const int whole_extents[6],
  const double origin[3], const double spacing[3])
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }
  view->GetDeliveryManager()->SetOrderedCompositingInformation(
    repr, translator, whole_extents, origin, spacing);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrderedCompositingInformation(vtkInformation* info, const double bounds[6])
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }
  vtkNew<vtkPartitionOrdering> partitionOrdering;
  partitionOrdering->Construct(bounds);
  view->PartitionOrdering->SetImplementation(partitionOrdering.GetPointer());
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ClearOrderedCompositingInformation()
{
  this->PartitionOrdering->SetImplementation(NULL);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDeliverToAllProcesses(
  vtkInformation* info, vtkPVDataRepresentation* repr, bool clone)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  view->GetDeliveryManager()->SetDeliverToAllProcesses(repr, clone, false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(vtkInformation* info,
  vtkPVDataRepresentation* repr, bool deliver_to_client, bool gather_before_delivery, int port)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  view->GetDeliveryManager()->SetDeliverToClientAndRenderingProcesses(
    repr, deliver_to_client, gather_before_delivery, false, port);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetGeometryBounds(
  vtkInformation* info, double bounds[6], vtkMatrix4x4* matrix /*=NULL*/)
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  if (self)
  {
    if (matrix && vtkMath::AreBoundsInitialized(bounds))
    {
      double min_point[4] = { bounds[0], bounds[2], bounds[4], 1 };
      double max_point[4] = { bounds[1], bounds[3], bounds[5], 1 };
      matrix->MultiplyPoint(min_point, min_point);
      matrix->MultiplyPoint(max_point, max_point);
      double transformed_bounds[6];
      transformed_bounds[0] = min_point[0] / min_point[3];
      transformed_bounds[2] = min_point[1] / min_point[3];
      transformed_bounds[4] = min_point[2] / min_point[3];
      transformed_bounds[1] = max_point[0] / max_point[3];
      transformed_bounds[3] = max_point[1] / max_point[3];
      transformed_bounds[5] = max_point[2] / max_point[3];
      self->GeometryBounds.AddBounds(transformed_bounds);
    }
    else
    {
      self->GeometryBounds.AddBounds(bounds);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetNextStreamedPiece(
  vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* piece)
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }
  vtkStreamingStatusMacro(<< repr << " streamed piece of size: "
                          << (piece ? piece->GetActualMemorySize() : 0)) self->GetDeliveryManager()
    ->SetNextStreamedPiece(repr, piece);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVRenderView::GetCurrentStreamedPiece(
  vtkInformation* info, vtkPVDataRepresentation* repr)
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
  }

  return self->GetDeliveryManager()->GetCurrentStreamedPiece(repr);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetRequiresDistributedRendering(
  vtkInformation* info, vtkPVDataRepresentation* vtkNotUsed(repr), bool value, bool for_lod)
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  bool& nonDistributedRenderingRequired =
    for_lod ? self->NonDistributedRenderingRequiredLOD : self->NonDistributedRenderingRequired;

  bool& distributedRenderingRequired =
    for_lod ? self->DistributedRenderingRequiredLOD : self->DistributedRenderingRequired;

  if ((nonDistributedRenderingRequired == true && value == true) ||
    (distributedRenderingRequired == true && value == false))
  {
    vtkGenericWarningMacro("Conflicting distributed rendering requests. "
                           "Rendering may not work as expected.");
    return;
  }

  if (value)
  {
    distributedRenderingRequired = true;
  }
  else
  {
    nonDistributedRenderingRequired = true;
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetForceDataDistributionMode(vtkInformation* info, int flag)
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }
  self->ForceDataDistributionMode = flag < 0 ? -1 : flag;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::ShouldUseDistributedRendering(double geometry_size, bool using_lod)
{
  // both can never be true.
  assert((this->DistributedRenderingRequired && this->NonDistributedRenderingRequired) == false);
  assert(
    (this->DistributedRenderingRequiredLOD && this->NonDistributedRenderingRequiredLOD) == false);

  bool remote_rendering_available = this->GetRemoteRenderingAvailable();

  // First check if any representations explicitly told us to use either remote
  // or local render. In that case, we pick accordingly without taking
  // remote-rendering thresholds into consideration (BUG #0013749).
  const bool& distributedRenderingRequired =
    using_lod ? this->DistributedRenderingRequiredLOD : this->DistributedRenderingRequired;
  const bool& nonDistributedRenderingRequired =
    using_lod ? this->NonDistributedRenderingRequiredLOD : this->NonDistributedRenderingRequired;

  try
  {
    if (distributedRenderingRequired == true && remote_rendering_available == false)
    {
      vtkErrorMacro("Some of the representations in this view require remote rendering "
                    "which, however, is not available. Rendering may not work as expected.");
    }
    else if (distributedRenderingRequired || nonDistributedRenderingRequired)
    {
      // implies that at least a representation requested a specific mode.
      throw(distributedRenderingRequired ? true : false);
    }

    if (remote_rendering_available == false)
    {
      throw false;
    }

    if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_BATCH)
    {
      // currently, we only support parallel rendering in batch mode.
      throw true;
    }

    throw(this->RemoteRenderingThreshold <= geometry_size);
  }
  catch (bool val)
  {
    //----------------------------------------------------------------------------
    // This helps us further condition the "ShouldUseDistributedRendering"
    // response based on whether distributed rendering makes sense in the current
    // configuration e.g. it doesn't make sense in builtin mode, or in batch mode
    // with 1 process.
    //----------------------------------------------------------------------------
    if (val)
    {
      // distributed rendering is requested. ensure that we're running in a mode
      // where distributed rendering has any effect i.e client-server or parallel
      // batch.
      switch (this->SynchronizedWindows->GetMode())
      {
        case vtkPVSynchronizedRenderWindows::BUILTIN:
          return false;
        case vtkPVSynchronizedRenderWindows::BATCH:
          return (this->SynchronizedWindows->GetParallelController()->GetNumberOfProcesses() > 1);
        default:
          break;
      }
    }
    return val;
  }
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::ShouldUseLODRendering(double geometry_size)
{
  // return false;
  return this->LODRenderingThreshold <= geometry_size;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::IsProcessRenderingGeometriesForCompositing(bool using_distributed_rendering)
{
  if (this->InTileDisplayMode() || this->InCaveDisplayMode())
  {
    return true;
  }

  vtkProcessModule::ProcessTypes processType = vtkProcessModule::GetProcessType();

  if (using_distributed_rendering)
  {
    // using distributed rendering.
    return (processType != vtkProcessModule::PROCESS_CLIENT);
  }
  else
  {
    // **not** using distributed rendering.
    if ((processType == vtkProcessModule::PROCESS_CLIENT) ||
      (processType == vtkProcessModule::PROCESS_BATCH &&
          this->SynchronizedWindows->GetParallelController()->GetLocalProcessId() == 0))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseOrderedCompositing()
{
  if (this->InCaveDisplayMode())
  {
    return false;
  }

  if (!this->NeedsOrderedCompositing)
  {
    return false;
  }

  if (this->ForceDataDistributionMode >= 0 &&
    this->ForceDataDistributionMode != vtkMPIMoveData::PASS_THROUGH)
  {
    // for any mode where data is getting collected or cloned, we assume
    // ordered compositing is not needed.
    return false;
  }

  switch (vtkProcessModule::GetProcessType())
  {
    case vtkProcessModule::PROCESS_SERVER:
    case vtkProcessModule::PROCESS_BATCH:
    case vtkProcessModule::PROCESS_RENDER_SERVER:
      if (vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions() > 1)
      {
        return true;
      }
    default:
      return false;
  }
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetRenderEmptyImages()
{
  int ptype = vtkProcessModule::GetProcessType();
  if (this->RenderEmptyImages &&
    ((ptype == vtkProcessModule::PROCESS_SERVER) || (ptype == vtkProcessModule::PROCESS_BATCH) ||
        (ptype == vtkProcessModule::PROCESS_RENDER_SERVER)) &&
    (vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions() > 1))
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFXAARelativeContrastThreshold(double val)
{
  this->FXAAOptions->SetRelativeContrastThreshold(static_cast<float>(val));
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFXAAHardContrastThreshold(double val)
{
  this->FXAAOptions->SetHardContrastThreshold(static_cast<float>(val));
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFXAASubpixelBlendLimit(double val)
{
  this->FXAAOptions->SetSubpixelBlendLimit(static_cast<float>(val));
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFXAASubpixelContrastThreshold(double val)
{
  this->FXAAOptions->SetSubpixelContrastThreshold(static_cast<float>(val));
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFXAAUseHighQualityEndpoints(bool val)
{
  this->FXAAOptions->SetUseHighQualityEndpoints(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFXAAEndpointSearchIterations(int val)
{
  this->FXAAOptions->SetEndpointSearchIterations(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetUseLightKit(bool use)
{
  if (this->UseLightKit != use)
  {
    if (use)
    {
      this->LightKit->AddLightsToRenderer(this->GetRenderer());
    }
    else
    {
      this->LightKit->RemoveLightsFromRenderer(this->GetRenderer());
    }
    this->UseLightKit = use;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLightSwitch(bool enable)
{
  this->Light->SetSwitch(enable ? 1 : 0);
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetLightSwitch()
{
  return this->Light->GetSwitch() != 0;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateCenterAxes()
{
  vtkBoundingBox bbox(this->GeometryBounds);

  // include the center of rotation in the axes size determination.
  bbox.AddPoint(this->CenterAxes->GetPosition());

  double widths[3];
  bbox.GetLengths(widths);

  // lets make some thickness in all directions
  double diameterOverTen = bbox.GetMaxLength() > 0 ? bbox.GetMaxLength() / 10.0 : 1.0;
  widths[0] = widths[0] < diameterOverTen ? diameterOverTen : widths[0];
  widths[1] = widths[1] < diameterOverTen ? diameterOverTen : widths[1];
  widths[2] = widths[2] < diameterOverTen ? diameterOverTen : widths[2];

  widths[0] *= 0.25;
  widths[1] *= 0.25;
  widths[2] *= 0.25;
  this->CenterAxes->SetScale(widths);

  double bounds[6];
  this->GeometryBounds.GetBounds(bounds);
  if (this->GridAxes3DActor)
  {
    this->GridAxes3DActor->SetTransformedBounds(bounds);
  }
}

//----------------------------------------------------------------------------
double vtkPVRenderView::GetZbufferDataAtPoint(int x, int y)
{
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->InCaveDisplayMode();
  if (in_tile_display_mode || in_cave_mode)
  {
    return this->GetRenderWindow()->GetZbufferDataAtPoint(x, y);
  }

  // Note, this relies on the fact that the most-recent render must have updated
  // the enabled state on  the vtkPVSynchronizedRenderWindows correctly based on
  // whether remote rendering was needed or not.
  return this->SynchronizedWindows->GetZbufferDataAtPoint(x, y, this->GetIdentifier());
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StreamingUpdate(const double view_planes[24])
{
  vtkTimerLog::MarkStartEvent("vtkPVRenderView::StreamingUpdate");

  // Provide information about the view planes to the representations.
  // Representations are free to ignore them.
  this->RequestInformation->Set(
    vtkPVRenderView::VIEW_PLANES(), const_cast<double*>(view_planes), 24);

  // Now call REQUEST_STREAMING_UPDATE() on all representations. Most representations
  // simply ignore it, but those that support streaming update the pipeline to
  // get the "next phase" of the data from the input pipeline.
  this->CallProcessViewRequest(vtkPVRenderView::REQUEST_STREAMING_UPDATE(),
    this->RequestInformation, this->ReplyInformationVector);

  vtkTimerLog::MarkEndEvent("vtkPVRenderView::StreamingUpdate");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::DeliverStreamedPieces(unsigned int size, unsigned int* representation_ids)
{
  // the plan now is to fetch the piece and then simply give it to the
  // representation as "next piece". Representation can decide what to do with
  // it, including adding to the existing datastructure.
  vtkTimerLog::MarkStartEvent("vtkPVRenderView::DeliverStreamedPieces");
  this->Internals->DeliveryManager->DeliverStreamedPieces(size, representation_ids);

  if (this->GetLocalProcessDoesRendering(this->GetUseDistributedRenderingForStillRender()))
  {
    // tell representations to "deal with" the newly streamed piece on the
    // rendering nodes.
    this->CallProcessViewRequest(vtkPVRenderView::REQUEST_PROCESS_STREAMED_PIECE(),
      this->RequestInformation, this->ReplyInformationVector);
  }

  this->Internals->DeliveryManager->ClearStreamedPieces();
  //                                  ^--- the most dubious part of this code.

  vtkTimerLog::MarkEndEvent("vtkPVRenderView::DeliverStreamedPieces");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseLightKit: " << this->UseLightKit << endl;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ConfigureCompressor(const char* configuration)
{
  this->SynchronizedRenderers->ConfigureCompressor(configuration);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::InvalidateCachedSelection()
{
  this->Selector->InvalidateCachedSelection();
}

//*****************************************************************
// Forwarded to orientation axes widget.

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrientationAxesInteractivity(bool v)
{
  this->OrientationWidget->SetEnabled(v);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrientationAxesVisibility(bool v)
{
  this->OrientationWidget->SetVisibility(v);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrientationAxesLabelColor(double r, double g, double b)
{
  this->OrientationWidget->SetAxisLabelColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrientationAxesOutlineColor(double r, double g, double b)
{
  this->OrientationWidget->SetOutlineColor(r, g, b);
}

//*****************************************************************
// Forwarded to center axes.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetCenterAxesVisibility(bool v)
{
  this->CenterAxes->SetVisibility(v);
}

//*****************************************************************
// Forward to vtkPVInteractorStyle instances.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetCenterOfRotation(double x, double y, double z)
{
  this->CenterAxes->SetPosition(x, y, z);
  if (this->TwoDInteractorStyle)
  {
    this->TwoDInteractorStyle->SetCenterOfRotation(x, y, z);
  }
  if (this->ThreeDInteractorStyle)
  {
    this->ThreeDInteractorStyle->SetCenterOfRotation(x, y, z);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetRotationFactor(double factor)
{
  if (this->TwoDInteractorStyle)
  {
    this->TwoDInteractorStyle->SetRotationFactor(factor);
  }
  if (this->ThreeDInteractorStyle)
  {
    this->ThreeDInteractorStyle->SetRotationFactor(factor);
  }
}

//*****************************************************************
// Forward to vtkLightKit.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyLightWarmth(double val)
{
  this->LightKit->SetKeyLightWarmth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyLightIntensity(double val)
{
  this->LightKit->SetKeyLightIntensity(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyLightElevation(double val)
{
  this->LightKit->SetKeyLightElevation(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyLightAzimuth(double val)
{
  this->LightKit->SetKeyLightAzimuth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFillLightWarmth(double val)
{
  this->LightKit->SetFillLightWarmth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyToFillRatio(double val)
{
  this->LightKit->SetKeyToFillRatio(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFillLightElevation(double val)
{
  this->LightKit->SetFillLightElevation(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetFillLightAzimuth(double val)
{
  this->LightKit->SetFillLightAzimuth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackLightWarmth(double val)
{
  this->LightKit->SetBackLightWarmth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyToBackRatio(double val)
{
  this->LightKit->SetKeyToBackRatio(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackLightElevation(double val)
{
  this->LightKit->SetBackLightElevation(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackLightAzimuth(double val)
{
  this->LightKit->SetBackLightAzimuth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetHeadLightWarmth(double val)
{
  this->LightKit->SetHeadLightWarmth(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetKeyToHeadRatio(double val)
{
  this->LightKit->SetKeyToHeadRatio(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetMaintainLuminance(int val)
{
  this->LightKit->SetMaintainLuminance(val);
}

//*****************************************************************
// Forward to 3D renderer.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetUseDepthPeeling(int val)
{
  this->GetRenderer()->SetUseDepthPeeling(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetMaximumNumberOfPeels(int val)
{
  this->GetRenderer()->SetMaximumNumberOfPeels(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackground(double r, double g, double b)
{
  this->GetRenderer()->SetBackground(r, g, b);
}
//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackground2(double r, double g, double b)
{
  this->GetRenderer()->SetBackground2(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundTexture(vtkTexture* val)
{
  this->GetRenderer()->SetBackgroundTexture(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetGradientBackground(int val)
{
  this->GetRenderer()->SetGradientBackground(val ? true : false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetTexturedBackground(int val)
{
  this->GetRenderer()->SetTexturedBackground(val ? true : false);
}

//*****************************************************************
// Forward to vtkLight.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetAmbientColor(double r, double g, double b)
{
  this->Light->SetAmbientColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetSpecularColor(double r, double g, double b)
{
  this->Light->SetSpecularColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDiffuseColor(double r, double g, double b)
{
  this->Light->SetDiffuseColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetIntensity(double val)
{
  this->Light->SetIntensity(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLightType(int val)
{
  this->Light->SetLightType(val);
}

//*****************************************************************
// Forward to vtkRenderWindow.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetStereoCapableWindow(int val)
{
  if (this->GetRenderWindow()->GetStereoCapableWindow() != val)
  {
    this->GetRenderWindow()->SetStereoCapableWindow(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetStereoRender(int val)
{
  this->GetRenderWindow()->SetStereoRender(val);
}

//----------------------------------------------------------------------------
inline int vtkGetNumberOfRendersPerFrame(int stereoMode)
{
  switch (stereoMode)
  {
    case VTK_STEREO_CRYSTAL_EYES:
    case VTK_STEREO_RED_BLUE:
    case VTK_STEREO_INTERLACED:
    case VTK_STEREO_DRESDEN:
    case VTK_STEREO_ANAGLYPH:
    case VTK_STEREO_CHECKERBOARD:
    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
    case VTK_STEREO_FAKE:
      return 2;

    case VTK_STEREO_LEFT:
    case VTK_STEREO_RIGHT:
    default:
      return 1;
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateStereoProperties()
{
  if (!this->GetRenderWindow()->GetStereoRender())
  {
    return;
  }

  if (this->ServerStereoType != 0 &&
    vtkGetNumberOfRendersPerFrame(this->ServerStereoType) !=
      vtkGetNumberOfRendersPerFrame(this->StereoType))
  {
    vtkWarningMacro("Incompatible stereo types for client and server ranks. "
                    "Forcing the server use the same type as the client.");
    this->ServerStereoType = 0;
  }

  switch (this->SynchronizedWindows->GetMode())
  {
    case vtkPVSynchronizedRenderWindows::RENDER_SERVER:
      if (this->ServerStereoType == VTK_STEREOTYPE_SAME_AS_CLIENT)
      {
        this->GetRenderWindow()->SetStereoType(this->StereoType);
      }
      else
      {
        this->GetRenderWindow()->SetStereoType(this->ServerStereoType);
      }
      break;

    default:
      this->GetRenderWindow()->SetStereoType(this->StereoType);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetMultiSamples(int val)
{
  this->GetRenderWindow()->SetMultiSamples(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetAlphaBitPlanes(int val)
{
  this->GetRenderWindow()->SetAlphaBitPlanes(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetStencilCapable(int val)
{
  this->GetRenderWindow()->SetStencilCapable(val);
}

//*****************************************************************
// Forwarded to vtkCamera if present on local processes.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetParallelProjection(int mode)
{
  if (this->ParallelProjection != mode)
  {
    this->ParallelProjection = mode;
    this->GetActiveCamera()->SetParallelProjection(mode);
    this->Modified();
  }
}

//*****************************************************************
// Forwarded to vtkPVInteractorStyle if present on local processes.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetCamera2DManipulators(const int manipulators[9])
{
  this->SetCameraManipulators(this->TwoDInteractorStyle, manipulators);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCamera3DManipulators(const int manipulators[9])
{
  this->SetCameraManipulators(this->ThreeDInteractorStyle, manipulators);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCameraManipulators(vtkPVInteractorStyle* style, const int manipulators[9])
{
  if (!style)
  {
    return;
  }

  style->RemoveAllManipulators();
  enum
  {
    NONE = 0,
    SHIFT = 1,
    CTRL = 2
  };

  enum
  {
    PAN = 1,
    ZOOM = 2,
    ROLL = 3,
    ROTATE = 4,
    MULTI_ROTATE = 5,
    ZOOM_TO_MOUSE = 6
  };

  for (int manip = NONE; manip <= CTRL; manip++)
  {
    for (int button = 0; button < 3; button++)
    {
      int manipType = manipulators[3 * manip + button];
      vtkSmartPointer<vtkCameraManipulator> cameraManipulator;
      switch (manipType)
      {
        case PAN:
          cameraManipulator = vtkSmartPointer<vtkTrackballPan>::New();
          break;
        case ZOOM:
          cameraManipulator = vtkSmartPointer<vtkPVTrackballZoom>::New();
          break;
        case ROLL:
          cameraManipulator = vtkSmartPointer<vtkPVTrackballRoll>::New();
          break;
        case ROTATE:
          cameraManipulator = vtkSmartPointer<vtkPVTrackballRotate>::New();
          break;
        case MULTI_ROTATE:
          cameraManipulator = vtkSmartPointer<vtkPVTrackballMultiRotate>::New();
          break;
        case ZOOM_TO_MOUSE:
          cameraManipulator = vtkSmartPointer<vtkPVTrackballZoomToMouse>::New();
          break;
      }
      if (cameraManipulator)
      {
        cameraManipulator->SetButton(button + 1); // since button starts with 1.
        cameraManipulator->SetControl(manip == CTRL ? 1 : 0);
        cameraManipulator->SetShift(manip == SHIFT ? 1 : 0);
        style->AddManipulator(cameraManipulator);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCamera2DMouseWheelMotionFactor(double factor)
{
  if (this->TwoDInteractorStyle)
  {
    this->TwoDInteractorStyle->SetMouseWheelMotionFactor(factor);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetCamera3DMouseWheelMotionFactor(double factor)
{
  if (this->ThreeDInteractorStyle)
  {
    this->ThreeDInteractorStyle->SetMouseWheelMotionFactor(factor);
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetShowAnnotation(bool val)
{
  this->ShowAnnotation = val;
  this->Annotation->SetVisibility(val ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetAnnotationColor(double r, double g, double b)
{
  this->Annotation->GetTextActor()->GetTextProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateAnnotationText()
{
  if (this->UpdateAnnotation && this->ShowAnnotation && !this->MakingSelection)
  {
    std::ostringstream stream;
    stream << this->Annotation->GetText();
    this->BuildAnnotationText(stream);
    this->Annotation->SetText(stream.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetSize(int dx, int dy)
{
  if (this->Size[0] != dx || this->Size[1] != dy)
  {
    this->InvalidateCachedSelection();
  }
  this->Superclass::SetSize(dx, dy);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPosition(int x, int y)
{
  if (this->Position[0] != x || this->Position[1] != y)
  {
    this->InvalidateCachedSelection();
  }
  this->Superclass::SetPosition(x, y);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::BuildAnnotationText(ostream& str)
{
  this->Timer->StopTimer();
  double time = this->Timer->GetElapsedTime();
  str << "Frame rate (approx): " << (time > 0.0 ? 1.0 / time : 100000.0) << " fps\n";
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDrawCells(bool choice)
{
  bool mod = false;
  if (choice)
  {
    if (this->Internals->FieldAssociation != VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
      this->Internals->FieldAssociation = VTK_SCALAR_MODE_USE_CELL_FIELD_DATA;
      mod = true;
    }
  }
  else
  {
    if (this->Internals->FieldAssociation != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
      this->Internals->FieldAssociation = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
      mod = true;
    }
  }

  if (mod)
  {
    if (this->Internals->FieldNameSet)
    {
      this->Internals->ValuePasses->SetInputArrayToProcess(
        this->Internals->FieldAssociation, this->Internals->FieldName.c_str());
    }
    else
    {
      this->Internals->ValuePasses->SetInputArrayToProcess(
        this->Internals->FieldAssociation, this->Internals->FieldAttributeType);
    }
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkPVRenderView::SetArrayNameToDraw(const char* name)
{
  if (!this->Internals->FieldNameSet || (this->Internals->FieldName != name))
  {
    this->Internals->FieldName = name;
    this->Internals->FieldNameSet = true;
    this->Internals->ValuePasses->SetInputArrayToProcess(
      this->Internals->FieldAssociation, this->Internals->FieldName.c_str());
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkPVRenderView::SetArrayNumberToDraw(int fieldAttributeType)
{
  if (this->Internals->FieldNameSet || (this->Internals->FieldAttributeType != fieldAttributeType))
  {
    this->Internals->FieldAttributeType = fieldAttributeType;
    this->Internals->FieldNameSet = false;
    this->Internals->ValuePasses->SetInputArrayToProcess(
      this->Internals->FieldAssociation, this->Internals->FieldAttributeType);
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkPVRenderView::SetValueRenderingModeCommand(int mode)
{
#ifdef VTKGL2
  // Fixes issue with the background (black) when comming back from FLOATING_POINT
  // mode. FLOATING_POINT mode is only supported in BATCH mode and single process
  // CLIENT.
  if (this->GetUseDistributedRenderingForStillRender() &&
    vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_CLIENT)
  {
    vtkWarningMacro("vtkValuePass::FLOATING_POINT mode is only supported in BATCH"
                    " mode. The result is only available in the root node.");
    return;
  }

  // Rendering mode can only be changed while capturing. TODO while in client mode?
  if (!this->Internals->IsInCapture)
  {
    return;
  }

  switch (mode)
  {
    case vtkValuePass::FLOATING_POINT:
    {
#ifdef PARAVIEW_USE_ICE_T
      IceTPassEnableFloatPass(true, this->SynchronizedRenderers);
      this->Internals->ValuePasses->SetRenderingMode(mode);
#else
      vtkWarningMacro("vtkValuePass::FLOATING_POINT mode is only supported in IceT"
                      " enabled builds. Falling back to INVERTIBLE_LUT.");

      this->Internals->ValuePasses->SetRenderingMode(vtkValuePass::INVERTIBLE_LUT);
#endif
    }
    break;

    case vtkValuePass::INVERTIBLE_LUT:
    default:
    {
#ifdef PARAVIEW_USE_ICE_T
      IceTPassEnableFloatPass(false, this->SynchronizedRenderers);
#endif
      this->Internals->ValuePasses->SetRenderingMode(mode);
    }
    break;
  }

  this->Modified();
#else
  (void)mode;
#endif
}

//-----------------------------------------------------------------------------
int vtkPVRenderView::GetValueRenderingModeCommand()
{
#ifdef VTKGL2
#ifdef PARAVIEW_USE_ICE_T
  return this->Internals->ValuePasses->GetRenderingMode();
#else
  return vtkValuePass::INVERTIBLE_LUT;
#endif
#else
  return 1; // vtkValuePass::INVERTIBLE_LUT
#endif
}

// ----------------------------------------------------------------------------
void vtkPVRenderView::SetArrayComponentToDraw(int comp)
{
  if (this->Internals->Component != comp)
  {
    this->Internals->Component = comp;
    this->Internals->ValuePasses->SetInputComponentToProcess(comp);
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkPVRenderView::SetScalarRange(double min, double max)
{
  if (this->Internals->ScalarRange[0] != min || this->Internals->ScalarRange[1] != max)
  {
    this->Internals->ScalarRange[0] = min;
    this->Internals->ScalarRange[1] = max;
    this->Internals->ValuePasses->SetScalarRange(min, max);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::BeginValueCapture()
{
  if (!this->Internals->IsInCapture)
  {
#if defined(VTKGL2) && defined(PARAVIEW_USE_ICE_T)
    if (vtkValuePass::FLOATING_POINT == this->Internals->ValuePasses->GetRenderingMode())
    {
      // Let the IceTPass know FLOATING_POINT is already enabled.
      IceTPassEnableFloatPass(true, this->SynchronizedRenderers);
    }
#endif

    this->Internals->SavedRenderPass = this->SynchronizedRenderers->GetRenderPass();
    this->Internals->SavedOrientationState = (this->OrientationWidget->GetEnabled() != 0);
    this->Internals->SavedAnnotationState = this->ShowAnnotation;
    this->SetOrientationAxesVisibility(false);
    this->SetShowAnnotation(false);
    this->Internals->IsInCapture = true;
  }

  if (this->Internals->FieldNameSet)
  {
    this->Internals->ValuePasses->SetInputArrayToProcess(
      this->Internals->FieldAssociation, this->Internals->FieldName.c_str());
  }
  else
  {
    this->Internals->ValuePasses->SetInputArrayToProcess(
      this->Internals->FieldAssociation, this->Internals->FieldAttributeType);
  }

  this->SynchronizedRenderers->SetRenderPass(this->Internals->ValuePasses.GetPointer());
}

//----------------------------------------------------------------------------
void vtkPVRenderView::EndValueCapture()
{
#if defined(VTKGL2) && defined(PARAVIEW_USE_ICE_T)
  if (vtkValuePass::FLOATING_POINT == this->Internals->ValuePasses->GetRenderingMode())
  {
    // Let the IceTPass know vtkValuePass will be removed.
    IceTPassEnableFloatPass(false, this->SynchronizedRenderers);
  }
#endif

  this->Internals->IsInCapture = false;
  this->SynchronizedRenderers->SetRenderPass(this->Internals->SavedRenderPass);
  this->Internals->SavedRenderPass = NULL;
  this->SetOrientationAxesVisibility(this->Internals->SavedOrientationState);
  this->SetShowAnnotation(this->Internals->SavedAnnotationState);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StartCaptureLuminance()
{
  if (!this->Internals->IsInCapture)
  {
    this->Internals->SavedRenderPass = this->SynchronizedRenderers->GetRenderPass();
    this->Internals->SavedOrientationState = (this->OrientationWidget->GetEnabled() != 0);
    this->Internals->SavedAnnotationState = this->ShowAnnotation;
    this->SetOrientationAxesVisibility(false);
    this->SetShowAnnotation(false);
    this->Internals->IsInCapture = true;
  }
#ifdef VTKGL2
  this->SynchronizedRenderers->SetRenderPass(this->Internals->LightingMapPass.GetPointer());
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StopCaptureLuminance()
{
  this->Internals->IsInCapture = false;
  this->SynchronizedRenderers->SetRenderPass(this->Internals->SavedRenderPass);
  this->Internals->SavedRenderPass = NULL;
  this->SetOrientationAxesVisibility(this->Internals->SavedOrientationState);
  this->SetShowAnnotation(this->Internals->SavedAnnotationState);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CaptureZBuffer()
{
#ifdef PARAVIEW_USE_ICE_T
  vtkIceTSynchronizedRenderers* IceTSynchronizedRenderers =
    vtkIceTSynchronizedRenderers::SafeDownCast(
      this->SynchronizedRenderers->GetParallelSynchronizer());
  if (IceTSynchronizedRenderers)
  {
    vtkIceTCompositePass* iceTPass = IceTSynchronizedRenderers->GetIceTCompositePass();
    if (iceTPass && iceTPass->GetLastRenderedDepths())
    {
      this->Internals->ArrayHolder->DeepCopy(iceTPass->GetLastRenderedDepths());
    }
  }
  else
#endif
  {
    this->Internals->ZGrabber->SetInput(this->GetRenderWindow());
    this->Internals->ZGrabber->ReadFrontBufferOff();
    this->Internals->ZGrabber->FixBoundaryOff();
    this->Internals->ZGrabber->ShouldRerenderOn();
    this->Internals->ZGrabber->SetMagnification(1);
    this->Internals->ZGrabber->SetInputBufferTypeToZBuffer();
    this->Internals->ZGrabber->Modified();
    this->Internals->ZGrabber->Update();
    this->Internals->ArrayHolder->DeepCopy(
      this->Internals->ZGrabber->GetOutput()->GetPointData()->GetScalars());
  }
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkPVRenderView::GetCapturedZBuffer()
{
  return this->Internals->ArrayHolder.GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CaptureValuesFloat()
{
#ifdef VTKGL2
#ifdef PARAVIEW_USE_ICE_T
  vtkIceTSynchronizedRenderers* IceTSynchronizedRenderers =
    vtkIceTSynchronizedRenderers::SafeDownCast(
      this->SynchronizedRenderers->GetParallelSynchronizer());

  vtkFloatArray* values = NULL;
  if (IceTSynchronizedRenderers)
  {
    vtkIceTCompositePass* iceTPass = IceTSynchronizedRenderers->GetIceTCompositePass();
    if (iceTPass && iceTPass->GetLastRenderedRGBA32F())
    {
      values = iceTPass->GetLastRenderedRGBA32F();
    }
  }
  else
  {
    if (this->GetUseDistributedRenderingForStillRender() &&
      vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_CLIENT)
    {
      vtkWarningMacro("vtkValuePass::FLOATING_POINT result is only available in the root"
                      " node.");
      return;
    }

    // Non-distributed case
    values = this->Internals->ValuePasses->GetFloatImageDataArray(this->RenderView->GetRenderer());
  }

  if (values)
  {
    // IceT requires the image format to be RGBA (R32F not supported). Component 0 is
    // enough from here on so a single component is exposed (components 1-3 hold the same
    // data).
    this->Internals->ArrayHolder->SetNumberOfComponents(1);
    this->Internals->ArrayHolder->SetNumberOfTuples(values->GetNumberOfTuples());
    this->Internals->ArrayHolder->CopyComponent(0, values, 0);
  }
#else
  vtkErrorMacro("vtkValuePass::FLOATING_POINT mode is only supported in IceT enabled"
                " builds.");
#endif
#endif
}

//-----------------------------------------------------------------------------
vtkFloatArray* vtkPVRenderView::GetCapturedValuesFloat()
{
#if defined(VTKGL2) && defined(PARAVIEW_USE_ICE_T)
  return this->Internals->ArrayHolder.GetPointer();
#else
  return NULL;
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetEnableOSPRay(bool v)
{
#ifdef PARAVIEW_USE_OSPRAY
  if (this->Internals->IsInOSPRay == v)
  {
    return;
  }
  this->Internals->IsInOSPRay = v;
  vtkRenderer* ren = this->GetRenderer();
  if (this->Internals->IsInOSPRay)
  {
    ren->SetUseShadows(this->Internals->OSPRayShadows);
    this->Internals->SavedRenderPass = this->SynchronizedRenderers->GetRenderPass();
    this->SynchronizedRenderers->SetRenderPass(this->Internals->OSPRayPass.GetPointer());
  }
  else
  {
    ren->SetUseShadows(false);
    this->SynchronizedRenderers->SetRenderPass(this->Internals->SavedRenderPass);
  }
  this->Modified();
#else
  if (v)
  {
    vtkWarningMacro(
      "Refusing to switch to OSPRay since it is not built into this copy of ParaView");
  }
#endif
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetEnableOSPRay()
{
  return this->Internals->IsInOSPRay;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetShadows(bool v)
{
#ifdef PARAVIEW_USE_OSPRAY
  this->Internals->OSPRayShadows = v;
  vtkRenderer* ren = this->GetRenderer();
  ren->SetUseShadows(v);
#else
  (void)v;
#endif
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetShadows()
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  if (ren->GetUseShadows() == 1)
  {
    return true;
  }
  else
  {
    return false;
  }
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetAmbientOcclusionSamples(int v)
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  vtkOSPRayRendererNode::SetAmbientSamples(v, ren);
#else
  (void)v;
#endif
}

//----------------------------------------------------------------------------
int vtkPVRenderView::GetAmbientOcclusionSamples()
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  return vtkOSPRayRendererNode::GetAmbientSamples(ren);
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetSamplesPerPixel(int v)
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  vtkOSPRayRendererNode::SetSamplesPerPixel(v, ren);
#else
  (void)v;
#endif
}

//----------------------------------------------------------------------------
int vtkPVRenderView::GetSamplesPerPixel()
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  return vtkOSPRayRendererNode::GetSamplesPerPixel(ren);
#else
  return 1;
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetMaxFrames(int v)
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  vtkOSPRayRendererNode::SetMaxFrames(v, ren);
#else
  (void)v;
#endif
}

//----------------------------------------------------------------------------
int vtkPVRenderView::GetMaxFrames()
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkRenderer* ren = this->GetRenderer();
  return vtkOSPRayRendererNode::GetMaxFrames(ren);
#else
  return 1;
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetLightScale(double v)
{
#ifdef PARAVIEW_USE_OSPRAY
  vtkOSPRayLightNode::SetLightScale(v);
#else
  (void)v;
#endif
}

//----------------------------------------------------------------------------
double vtkPVRenderView::GetLightScale()
{
#ifdef PARAVIEW_USE_OSPRAY
  return vtkOSPRayLightNode::GetLightScale();
#else
  return 0.5;
#endif
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetOSPRayContinueStreaming()
{
#ifdef PARAVIEW_USE_OSPRAY
  if (!this->Internals->IsInOSPRay)
  {
    return false;
  }
  this->Internals->OSPRayCount++;
  bool keep_going = this->Internals->OSPRayCount < this->GetMaxFrames();
  if (!keep_going)
  {
    this->Internals->OSPRayCount = 0;
  }
  return keep_going;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
vtkPVCameraCollection* vtkPVRenderView::GetDiscreteCameras(
  vtkInformation* info, vtkPVDataRepresentation* vtkNotUsed(repr))
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
  }

  return self->DiscreteCameras;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDiscreteCameras(
  vtkInformation* info, vtkPVDataRepresentation*, vtkPVCameraCollection* style)
{
  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }

  self->DiscreteCameras = style;
}
