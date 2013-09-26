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

#include "vtk3DWidgetRepresentation.h"
#include "vtkAlgorithmOutput.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformation.h"
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
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkProcessModule.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVConfig.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVHardwareSelector.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVOptions.h"
#include "vtkPVSession.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkRenderer.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTrackballPan.h"
#include "vtkTrivialProducer.h"
#include "vtkVector.h"
#include "vtkWeakPointer.h"

#ifdef PARAVIEW_USE_PISTON
#include "vtkPistonMapper.h"
#endif

#include <assert.h>
#include <vector>
#include <set>
#include <map>

class vtkPVRenderView::vtkInternals
{
  std::map<int, vtkWeakPointer<vtkPVDataRepresentation> > PropMap;

public:
  vtkNew<vtkPVDataDeliveryManager> DeliveryManager;

  void RegisterSelectionProp(
    int id, vtkProp*, vtkPVDataRepresentation* rep)
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
    std::map<int, vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter =
      this->PropMap.find(id);
    return (iter != this->PropMap.end()? iter->second : NULL);
    }

#ifdef PARAVIEW_USE_PISTON
  void PreRender(vtkRenderViewBase *renderView)
    {
    if(vtkPVOptions *options = vtkProcessModule::GetProcessModule()->GetOptions())
      {
      if(!vtkPistonMapper::IsEnabledCudaGL() && options->GetUseCudaInterop())
        {
        vtkPistonMapper::InitCudaGL(renderView->GetRenderWindow());
        }
      }
    }
#else
  void PreRender(vtkRenderViewBase *vtkNotUsed(renderView))
    {
    }
#endif // PARAVIEW_USE_PISTON

};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderView);
vtkInformationKeyMacro(vtkPVRenderView, USE_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, USE_OUTLINE_FOR_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, LOD_RESOLUTION, Double);
vtkInformationKeyMacro(vtkPVRenderView, NEED_ORDERED_COMPOSITING, Integer);
vtkInformationKeyMacro(vtkPVRenderView, RENDER_EMPTY_IMAGES, Integer);
vtkInformationKeyMacro(vtkPVRenderView, REQUEST_STREAMING_UPDATE, Request);
vtkInformationKeyMacro(vtkPVRenderView, REQUEST_PROCESS_STREAMED_PIECE, Request);
vtkInformationKeyRestrictedMacro(
  vtkPVRenderView, VIEW_PLANES, DoubleVector, 24);

vtkCxxSetObjectMacro(vtkPVRenderView, LastSelection, vtkSelection);
//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  this->Internals = new vtkInternals();
  // non-reference counted, so no worries about reference loops.
  this->Internals->DeliveryManager->SetRenderView(this);

  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();

  this->RemoteRenderingAvailable = true;

  this->StillRenderProcesses = vtkPVSession::NONE;
  this->InteractiveRenderProcesses = vtkPVSession::NONE;
  this->UsedLODForLastRender = false;
  this->UseLODForInteractiveRender = false;
  this->UseDistributedRenderingForStillRender = false;
  this->UseDistributedRenderingForInteractiveRender = false;
  this->MakingSelection = false;
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
  this->PolygonStyle = 0;
  this->RubberBandStyle = 0;
  this->RubberBandZoom = 0;
  this->CenterAxes = vtkPVCenterAxesActor::New();
  this->CenterAxes->SetComputeNormals(0);
  this->CenterAxes->SetPickable(0);
  this->CenterAxes->SetScale(0.25, 0.25, 0.25);
  this->OrientationWidget = vtkPVAxesWidget::New();
  this->InteractionMode = -1;
  this->LastSelection = NULL;
  this->UseOffscreenRenderingForScreenshots = false;
  this->UseInteractiveRenderingForSceenshots = false;
  this->UseOffscreenRendering = (options->GetUseOffscreenRendering() != 0);
  this->Selector = vtkPVHardwareSelector::New();
  this->PreviousParallelProjectionStatus = 0;
  this->NeedsOrderedCompositing = false;
  this->RenderEmptyImages = false;

  this->SynchronizedRenderers = vtkPVSynchronizedRenderer::New();

  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    this->Interactor = vtkPVGenericRenderWindowInteractor::New();
    // essential to call Initialize() otherwise first time the render is called
    // on  the render window, it initializes the interactor which in turn
    // results in a call to Render() which can cause uncanny side effects.
    this->Interactor->Initialize();
    }

  vtkRenderWindow* window = this->SynchronizedWindows->NewRenderWindow();
  window->SetMultiSamples(0);
  window->SetOffScreenRendering(this->UseOffscreenRendering? 1 : 0);
  window->SetInteractor(this->Interactor);
  this->RenderView = vtkRenderViewBase::New();
  this->RenderView->SetRenderWindow(window);
  window->Delete();

  this->NonCompositedRenderer = vtkRenderer::New();
  this->NonCompositedRenderer->EraseOff();
  this->NonCompositedRenderer->InteractiveOff();
  this->NonCompositedRenderer->SetLayer(2);
  this->NonCompositedRenderer->SetActiveCamera(
    this->RenderView->GetRenderer()->GetActiveCamera());
  window->AddRenderer(this->NonCompositedRenderer);
  window->SetNumberOfLayers(3);
  this->RenderView->GetRenderer()->GetActiveCamera()->ParallelProjectionOff();

  vtkMemberFunctionCommand<vtkPVRenderView>* observer =
    vtkMemberFunctionCommand<vtkPVRenderView>::New();
  observer->SetCallback(*this, &vtkPVRenderView::ResetCameraClippingRange);
  this->GetRenderer()->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,
    observer);
  observer->FastDelete();

  this->GetRenderer()->SetUseDepthPeeling(1);

  this->Light = vtkLight::New();
  this->Light->SetAmbientColor(1, 1, 1);
  this->Light->SetSpecularColor(1, 1, 1);
  this->Light->SetDiffuseColor(1, 1, 1);
  this->Light->SetIntensity(1.0);
  this->Light->SetLightType(2); // CameraLight
  this->LightKit = vtkLightKit::New();
  this->GetRenderer()->AddLight(this->Light);
  this->GetRenderer()->SetAutomaticLightCreation(0);

  if (this->Interactor)
    {
    this->InteractorStyle = // Default one will be the 3D
        this->ThreeDInteractorStyle = vtkPVInteractorStyle::New();
    this->TwoDInteractorStyle = vtkPVInteractorStyle::New();

    this->Interactor->SetRenderer(this->GetRenderer());
    this->Interactor->SetRenderWindow(this->GetRenderWindow());
    this->Interactor->SetInteractorStyle(this->ThreeDInteractorStyle);

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

    this->RubberBandStyle = vtkInteractorStyleRubberBand3D::New();
    this->RubberBandStyle->RenderOnMouseMoveOff();
    vtkCommand* observer2 = vtkMakeMemberFunctionCommand(*this,
      &vtkPVRenderView::OnSelectionChangedEvent);
    this->RubberBandStyle->AddObserver(vtkCommand::SelectionChangedEvent,
      observer2);
    observer2->Delete();

    this->RubberBandZoom = vtkInteractorStyleRubberBandZoom::New();
    this->PolygonStyle = vtkInteractorStyleDrawPolygon::New();
    vtkCommand* observer3 = vtkMakeMemberFunctionCommand(*this,
      &vtkPVRenderView::OnPolygonSelectionEvent);
    this->PolygonStyle->AddObserver(vtkCommand::SelectionChangedEvent,
      observer3);
    observer3->Delete();
    }

  this->OrientationWidget->SetParentRenderer(this->GetRenderer());
  this->OrientationWidget->SetViewport(0, 0, 0.25, 0.25);
  this->OrientationWidget->SetInteractor(this->Interactor);

  this->GetRenderer()->AddActor(this->CenterAxes);

  this->SetInteractionMode(INTERACTION_MODE_3D);
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
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

  if (this->Interactor)
    {
    this->Interactor->Delete();
    this->Interactor = 0;
    }
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

  this->SynchronizedRenderers->Initialize(
    this->SynchronizedWindows->GetSession(), id);
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
void vtkPVRenderView::RegisterPropForHardwareSelection(
  vtkPVDataRepresentation* repr, vtkProp* prop)
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
vtkRenderer* vtkPVRenderView::GetRenderer()
{
  return this->RenderView->GetRenderer();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetActiveCamera(vtkCamera* camera)
{
  this->GetRenderer()->SetActiveCamera(camera);
  this->GetNonCompositedRenderer()->SetActiveCamera(camera);
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
void vtkPVRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
    {
    if(this->InteractionMode == INTERACTION_MODE_3D)
      {
      this->PreviousParallelProjectionStatus = this->GetActiveCamera()->GetParallelProjection();
      }

    this->InteractionMode = mode;
    this->Modified();

    if (this->Interactor == NULL)
      {
      return;
      }

    switch (this->InteractionMode)
      {
    case INTERACTION_MODE_3D:
      this->Interactor->SetInteractorStyle(
            this->InteractorStyle = this->ThreeDInteractorStyle);
      // Get back to the previous state
      this->GetActiveCamera()->SetParallelProjection(this->PreviousParallelProjectionStatus);
      break;
    case INTERACTION_MODE_2D:
      this->Interactor->SetInteractorStyle(
            this->InteractorStyle = this->TwoDInteractorStyle);
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
  ordered_region[0] = region[0] < region[2]? region[0] : region[2];
  ordered_region[2] = region[0] > region[2]? region[0] : region[2];
  ordered_region[1] = region[1] < region[3]? region[1] : region[3];
  ordered_region[3] = region[1] > region[3]? region[1] : region[3];

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
  if(points.size() >= 3)
    {
    vtkNew<vtkIntArray> polygonPointsArray;
    polygonPointsArray->SetNumberOfComponents(2);
    polygonPointsArray->SetNumberOfTuples(points.size());
    for (unsigned int j = 0; j < points.size(); ++j)
      {
      const vtkVector2i &v = points[j];
      int pos[2] = {v[0], v[1]};
      polygonPointsArray->SetTupleValue(j, pos);
      }

    this->InvokeEvent(vtkCommand::SelectionChangedEvent,
      polygonPointsArray.GetPointer());
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

  // Make sure that the representations are up-to-date. This is required since
  // due to delayed-swicth-back-from-lod, the most recent render maybe a LOD
  // render (or a nonremote render) in which case we need to update the
  // representation pipelines correctly.
  this->Render(/*interactive*/false, /*skip-rendering*/false);

  this->SetLastSelection(NULL);

  this->Selector->SetRenderer(this->GetRenderer());
  this->Selector->SetFieldAssociation(fieldAssociation);
  this->Selector->SetSynchronizedWindows(this->SynchronizedWindows);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Select(int fieldAssociation, int region[4])
{
  if(!this->PrepareSelect(fieldAssociation))
    {
    return;
    }
  vtkSmartPointer<vtkSelection> sel;
  if (this->SynchronizedWindows->GetEnabled() ||
    this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    sel.TakeReference(this->Selector->Select(region));
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
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPolygonPoints(
  int* polygonPoints, vtkIdType arrayLen)
{
  this->SelectPolygon(vtkDataObject::FIELD_ASSOCIATION_POINTS,
    polygonPoints, arrayLen);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPolygonCells(
  int* polygonPoints, vtkIdType arrayLen)
{
  this->SelectPolygon(vtkDataObject::FIELD_ASSOCIATION_CELLS,
    polygonPoints, arrayLen);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SelectPolygon(
  int fieldAssociation, int* polygonPoints, vtkIdType arrayLen)
{
  if(!this->PrepareSelect(fieldAssociation))
    {
    return;
    }
  vtkSmartPointer<vtkSelection> sel;
  if (this->SynchronizedWindows->GetEnabled() ||
    this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    sel.TakeReference(this->Selector->PolygonSelect(
        polygonPoints, arrayLen));
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
  for (unsigned int cc=0; cc < sel->GetNumberOfNodes(); cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node->GetProperties()->Has(vtkSelectionNode::PROP_ID()))
      {
      int propid = node->GetProperties()->Get(vtkSelectionNode::PROP_ID());
      vtkPVDataRepresentation* repr =
        this->Internals->GetRepresentationForPropId(propid);
      if (repr)
        {
        node->GetProperties()->Set(vtkSelectionNode::SOURCE(), repr);
        }
      }
    }

  this->SetLastSelection(sel);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  if (this->GeometryBounds.IsValid())
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
    double prop_bounds[6];
    this->GetRenderer()->ComputeVisiblePropBounds(prop_bounds);
    this->CenterAxes->SetUseBounds(1);


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
    return using_distributed_rendering || 
      this->InTileDisplayMode() ||
      this->SynchronizedWindows->GetIsInCave();
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
  this->RenderView->GetRenderer()->ResetCamera(bounds);

  this->InvokeEvent(vtkCommand::ResetCameraEvent);
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVRenderView::ResetCamera(double bounds[6])
{
  // Remember, vtkRenderer::ResetCamera() calls
  // vtkRenderer::ResetCameraClippingPlanes() with the given bounds.
  this->RenderView->GetRenderer()->ResetCamera(bounds);
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

  vtkMultiProcessController* p_controller =
    this->SynchronizedWindows->GetParallelController();
  vtkMultiProcessController* d_controller =
    this->SynchronizedWindows->GetClientDataServerController();
  vtkMultiProcessController* r_controller =
    this->SynchronizedWindows->GetClientServerController();
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
      r_controller->Send(&magicNumber, 1, 1, 41001 );
      counterSynchronizedSuccessfully =
        (client_sync_counter == magicNumber);
      }

    if (p_controller)
      {
      p_controller->Broadcast(&this->RemoteRenderingThreshold, 1, 0);
      int temp = counterSynchronizedSuccessfully? 1 : 0;
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

  vtkMultiProcessController* p_controller =
    this->SynchronizedWindows->GetParallelController();
  vtkMultiProcessController* r_controller =
    this->SynchronizedWindows->GetClientServerController();

  if (this->SynchronizedWindows->GetMode() == vtkPVSynchronizedRenderWindows::CLIENT)
    {
    vtkMultiProcessStream stream;
    stream << (this->UseLODForInteractiveRender? 1 : 0)
           << (this->UseDistributedRenderingForStillRender? 1 : 0)
           << (this->UseDistributedRenderingForInteractiveRender? 1 :0)
           << this->StillRenderProcesses
           << this->InteractiveRenderProcesses;
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
    stream >> arg1
           >> arg2
           >> arg3
           >> this->StillRenderProcesses
           >> this->InteractiveRenderProcesses;
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

  this->Superclass::Update();

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
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* info =
      this->ReplyInformationVector->GetInformationObject(cc);
    if ( info->Has(NEED_ORDERED_COMPOSITING())
      && (info->Get(NEED_ORDERED_COMPOSITING()) != 0))
      {
      this->NeedsOrderedCompositing = true;
      }
    else
    if ( info->Has(RENDER_EMPTY_IMAGES())
      && (info->Get(RENDER_EMPTY_IMAGES()) != 0))
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
  this->UseDistributedRenderingForStillRender = this->ShouldUseDistributedRendering(local_size);
  if (!this->UseLODForInteractiveRender)
    {
    this->UseDistributedRenderingForInteractiveRender =
      this->UseDistributedRenderingForStillRender;
    }

  this->StillRenderProcesses = this->InteractiveRenderProcesses =
    vtkPVSession::CLIENT;
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->SynchronizedWindows->GetIsInCave();
  if (in_tile_display_mode || in_cave_mode ||
    this->UseDistributedRenderingForStillRender)
    {
    this->StillRenderProcesses = vtkPVSession::CLIENT_AND_SERVERS;
    }
  if (in_tile_display_mode || in_cave_mode ||
    this->UseDistributedRenderingForInteractiveRender)
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
  this->UseDistributedRenderingForInteractiveRender = otherView->UseDistributedRenderingForInteractiveRender;
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

  this->CallProcessViewRequest(
    vtkPVView::REQUEST_UPDATE_LOD(),
    this->RequestInformation, this->ReplyInformationVector);

  double local_size = this->GetDeliveryManager()->GetVisibleDataSize(true) / 1024.0;
  this->SynchronizedWindows->SynchronizeSize(local_size);
  // cout << "LOD Geometry size: " << local_size << endl;

  this->UseDistributedRenderingForInteractiveRender =
    this->ShouldUseDistributedRendering(local_size);

  this->InteractiveRenderProcesses = vtkPVSession::CLIENT;
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->SynchronizedWindows->GetIsInCave();
  if (in_tile_display_mode || in_cave_mode ||
    this->UseDistributedRenderingForInteractiveRender)
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

  this->Internals->PreRender(this->RenderView);

  this->Render(true, false);

  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Render(bool interactive, bool skip_rendering)
{
  if (this->SynchronizedWindows->GetMode() !=
    vtkPVSynchronizedRenderWindows::CLIENT ||
    (!interactive && this->UseDistributedRenderingForStillRender) ||
    (interactive && this->UseDistributedRenderingForInteractiveRender))
    {
    // in multi-client modes, Render() will be called on client always. Now the
    // client need to coordinate with server only when we are remote rendering.
    // if Render() is called on server side, then we are indeed remote
    // rendering, irrespective of what the local flags tell us and we need to
    // coordinate with the client to update the local flags correctly.
    this->SynchronizeForCollaboration();
    }

  // BUG #13534. Reset the clip planes on every render. Since this does not
  // involve any communication, doing this on every render is not a big deal.
  this->ResetCameraClippingRange();

  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->SynchronizedWindows->GetIsInCave();
  if (in_cave_mode && !this->RemoteRenderingAvailable)
    {
    static bool warned_once = false;
    if (!warned_once)
      {
      vtkErrorMacro(
        "In Cave mode and Display cannot be opened on server-side! "
        "Ensure the environment is set correctly in the pvx file.");
      in_cave_mode = false;
      }
    }

  // Use loss-less image compression for client-server for full-res renders.
  this->SynchronizedRenderers->SetLossLessCompression(!interactive);

  bool use_lod_rendering = interactive? this->GetUseLODForInteractiveRender() : false;
  if (use_lod_rendering)
    {
    this->RequestInformation->Set(USE_LOD(), 1);
    }

  // cout << "Using remote rendering: " << use_distributed_rendering << endl;

  // Decide if we are doing remote rendering or local rendering.
  bool use_distributed_rendering = interactive?
    this->GetUseDistributedRenderingForInteractiveRender():
    this->GetUseDistributedRenderingForStillRender();

  if (this->GetUseOrderedCompositing())
    {
    this->Internals->DeliveryManager->RedistributeDataForOrderedCompositing(
      use_lod_rendering);
    this->SynchronizedRenderers->SetKdTree(
      this->Internals->DeliveryManager->GetKdTree());
    }
  else
    {
    this->SynchronizedRenderers->SetKdTree(NULL);
    }

  // enable render empty images if it was requested
  this->SynchronizedRenderers->SetRenderEmptyImages(this->GetRenderEmptyImages());

  // Render each representation with available geometry.
  // This is the pass where representations get an opportunity to get the
  // currently "available" represented data and try to render it.
  this->CallProcessViewRequest(vtkPVView::REQUEST_RENDER(),
    this->RequestInformation, this->ReplyInformationVector);

  // set the image reduction factor.
  this->SynchronizedRenderers->SetImageReductionFactor(
    (interactive?
     this->InteractiveRenderImageReductionFactor :
     this->StillRenderImageReductionFactor));

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
    in_cave_mode ||
    (!use_distributed_rendering && in_tile_display_mode));


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
  if (
    (this->SynchronizedWindows->GetLocalProcessIsDriver() ||
     (!this->SynchronizedWindows->GetRenderEventPropagation() && use_distributed_rendering) ||
     in_tile_display_mode || in_cave_mode) &&
    vtkProcessModule::GetProcessType() != vtkProcessModule::PROCESS_DATA_SERVER)
    {
    this->GetRenderWindow()->Render();
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
void vtkPVRenderView::Deliver(int use_lod,
    unsigned int size, unsigned int *representation_ids)
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
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->SynchronizedWindows->GetIsInCave();
  if (in_cave_mode)
    {
    return vtkMPIMoveData::CLONE;
    }

  if (use_remote_rendering)
    {
    return in_tile_display_mode?
      vtkMPIMoveData::COLLECT_AND_PASS_THROUGH:
      vtkMPIMoveData::PASS_THROUGH;
    }

  return in_tile_display_mode? vtkMPIMoveData::CLONE: vtkMPIMoveData::COLLECT;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPiece(vtkInformation* info,
  vtkPVDataRepresentation* repr, vtkDataObject* data)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
    }

  view->GetDeliveryManager()->SetPiece(repr, data, false);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVRenderView::GetPieceProducer(vtkInformation* info,
    vtkPVDataRepresentation* repr)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
    }

  return view->GetDeliveryManager()->GetProducer(repr, false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPieceLOD(vtkInformation* info,
  vtkPVDataRepresentation* repr, vtkDataObject* data)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
    }

  view->GetDeliveryManager()->SetPiece(repr, data, true);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVRenderView::GetPieceProducerLOD(vtkInformation* info,
    vtkPVDataRepresentation* repr)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
    }

  return view->GetDeliveryManager()->GetProducer(repr, true);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::MarkAsRedistributable(
  vtkInformation* info, vtkPVDataRepresentation* repr, bool value/*=true*/)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
    }

  view->GetDeliveryManager()->MarkAsRedistributable(repr, value);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetStreamable(
  vtkInformation* info, vtkPVDataRepresentation* repr, bool val)
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
void vtkPVRenderView::SetOrderedCompositingInformation(
  vtkInformation* info, vtkPVDataRepresentation* repr,
  vtkExtentTranslator* translator,
  const int whole_extents[6], const double origin[3], const double spacing[3])
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
void vtkPVRenderView::SetDeliverToAllProcesses(vtkInformation* info,
  vtkPVDataRepresentation* repr, bool clone)
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
void vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(
  vtkInformation* info, vtkPVDataRepresentation* repr,
  bool deliver_to_client, bool gather_before_delivery)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!view)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
    }

  view->GetDeliveryManager()->SetDeliverToClientAndRenderingProcesses(
    repr, deliver_to_client, gather_before_delivery, false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetGeometryBounds(vtkInformation* info,
  double bounds[6], vtkMatrix4x4* matrix /*=NULL*/)
{
  vtkPVRenderView* self= vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
    }

  if (self)
    {
    if (matrix && vtkMath::AreBoundsInitialized(bounds))
      {
      double min_point[4] = {bounds[0], bounds[2], bounds[4], 1};
      double max_point[4] = {bounds[1], bounds[3], bounds[5], 1};
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
  vtkPVRenderView* self= vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
    }
  vtkStreamingStatusMacro(
    << repr << " streamed piece of size: " << (piece? piece->GetActualMemorySize() : 0))
  self->GetDeliveryManager()->SetNextStreamedPiece(repr, piece);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVRenderView::GetCurrentStreamedPiece(
  vtkInformation* info, vtkPVDataRepresentation* repr)
{
  vtkPVRenderView* self= vtkPVRenderView::SafeDownCast(info->Get(VIEW()));
  if (!self)
    {
    vtkGenericWarningMacro("Missing VIEW().");
    return NULL;
    }

  return self->GetDeliveryManager()->GetCurrentStreamedPiece(repr);
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::ShouldUseDistributedRendering(double geometry_size)
{
  if (this->GetRemoteRenderingAvailable() == false)
    {
    return false;
    }

  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_BATCH)
    {
    // currently, we only support parallel rendering in batch mode.
    return true;
    }

  return this->RemoteRenderingThreshold <= geometry_size;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::ShouldUseLODRendering(double geometry_size)
{
  // return false;
  return this->LODRenderingThreshold <= geometry_size;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseOrderedCompositing()
{
  if (this->SynchronizedWindows->GetIsInCave())
    {
    return false;
    }

  if (!this->NeedsOrderedCompositing || this->MakingSelection)
    {
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
  if ( this->RenderEmptyImages
    && ((ptype == vtkProcessModule::PROCESS_SERVER)
    || (ptype == vtkProcessModule::PROCESS_BATCH)
    || (ptype == vtkProcessModule::PROCESS_RENDER_SERVER))
    && (vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions() > 1) )
    {
    return true;
    }
  return false;
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
  this->Light->SetSwitch(enable? 1 : 0);
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
  double diameterOverTen = bbox.GetMaxLength() > 0?
    bbox.GetMaxLength() / 10.0 : 1.0;
  widths[0] = widths[0] < diameterOverTen ? diameterOverTen : widths[0];
  widths[1] = widths[1] < diameterOverTen ? diameterOverTen : widths[1];
  widths[2] = widths[2] < diameterOverTen ? diameterOverTen : widths[2];

  widths[0] *= 0.25;
  widths[1] *= 0.25;
  widths[2] *= 0.25;
  this->CenterAxes->SetScale(widths);
}

//----------------------------------------------------------------------------
double vtkPVRenderView::GetZbufferDataAtPoint(int x, int y)
{
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->SynchronizedWindows->GetIsInCave();
  if (in_tile_display_mode || in_cave_mode)
    {
    return this->GetRenderWindow()->GetZbufferDataAtPoint(x, y);
    }

  // Note, this relies on the fact that the most-recent render must have updated
  // the enabled state on  the vtkPVSynchronizedRenderWindows correctly based on
  // whether remote rendering was needed or not.
  return this->SynchronizedWindows->GetZbufferDataAtPoint(x, y,
    this->GetIdentifier());
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StreamingUpdate(const double view_planes[24])
{
  vtkTimerLog::MarkStartEvent("vtkPVRenderView::StreamingUpdate");

  // Provide information about the view planes to the representations.
  // Representations are free to ignore them.
  this->RequestInformation->Set(vtkPVRenderView::VIEW_PLANES(),
    const_cast<double*>(view_planes), 24);

  // Now call REQUEST_STREAMING_UPDATE() on all representations. Most representations
  // simply ignore it, but those that support streaming update the pipeline to
  // get the "next phase" of the data from the input pipeline.
  this->CallProcessViewRequest(vtkPVRenderView::REQUEST_STREAMING_UPDATE(),
    this->RequestInformation, this->ReplyInformationVector);

  vtkTimerLog::MarkEndEvent("vtkPVRenderView::StreamingUpdate");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::DeliverStreamedPieces(
  unsigned int size, unsigned int *representation_ids)
{
  // the plan now is to fetch the piece and then simply give it to the
  // representation as "next piece". Representation can decide what to do with
  // it, including adding to the existing datastructure.
  vtkTimerLog::MarkStartEvent("vtkPVRenderView::DeliverStreamedPieces");
  this->Internals->DeliveryManager->DeliverStreamedPieces(
    size, representation_ids);

  if (this->GetLocalProcessDoesRendering(
      this->GetUseDistributedRenderingForStillRender()))
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

//----------------------------------------------------------------------------
void vtkPVRenderView::PrepareForScreenshot()
{
  if (this->Interactor && this->GetRenderWindow())
    {
    this->GetRenderWindow()->SetInteractor(this->Interactor);
    }
  this->Superclass::PrepareForScreenshot();
}

//*****************************************************************
// Forwarded to orientation axes widget.

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrientationAxesInteractivity(bool v)
{
  this->OrientationWidget->SetInteractive(v);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetOrientationAxesVisibility(bool v)
{
  this->OrientationWidget->SetEnabled(v);
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
// Forward to vtkPVGenericRenderWindowInteractor.
//----------------------------------------------------------------------------
void vtkPVRenderView::SetCenterOfRotation(double x, double y, double z)
{
  this->CenterAxes->SetPosition(x, y, z);
  if (this->Interactor)
    {
    this->Interactor->SetCenterOfRotation(x, y, z);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetNonInteractiveRenderDelay(double seconds)
{
  if (this->Interactor)
    {
    if(seconds > 0)
      {
      this->Interactor->SetNonInteractiveRenderDelay(static_cast<unsigned long>(seconds*1000));
      }
    else
      {
      this->Interactor->SetNonInteractiveRenderDelay(0);
      }
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
  this->GetRenderer()->SetGradientBackground(val? true : false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetTexturedBackground(int val)
{
  this->GetRenderer()->SetTexturedBackground(val? true : false);
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
void vtkPVRenderView::SetStereoType(int val)
{
  this->GetRenderWindow()->SetStereoType(val);
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
// Forwarded to vtkPVInteractorStyle if present on local processes.
//----------------------------------------------------------------------------
void vtkPVRenderView::Add2DManipulator(vtkCameraManipulator* val)
{
  if (this->TwoDInteractorStyle)
    {
    this->TwoDInteractorStyle->AddManipulator(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveAll2DManipulators()
{
  if (this->TwoDInteractorStyle)
    {
    this->TwoDInteractorStyle->RemoveAllManipulators();
    }
}
//----------------------------------------------------------------------------
void vtkPVRenderView::Add3DManipulator(vtkCameraManipulator* val)
{
  if (this->ThreeDInteractorStyle)
    {
    this->ThreeDInteractorStyle->AddManipulator(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveAll3DManipulators()
{
  if (this->ThreeDInteractorStyle)
    {
    this->ThreeDInteractorStyle->RemoveAllManipulators();
    }
}
