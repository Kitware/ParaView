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

#include "vtkBoundingBox.h"
#include "vtkBSPCutsGenerator.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkInteractorStyleRubberBandZoom.h"
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkMath.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkProcessModule.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVHardwareSelector.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVOptions.h"
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
#include "vtkWeakPointer.h"

#include <assert.h>
#include <vtkstd/vector>
#include <vtkstd/set>

//----------------------------------------------------------------------------
// Statics
//----------------------------------------------------------------------------
bool vtkPVRenderView::RemoteRenderingAllowed = true;
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderView);
vtkInformationKeyMacro(vtkPVRenderView, GEOMETRY_SIZE, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DATA_DISTRIBUTION_MODE, Integer);
vtkInformationKeyMacro(vtkPVRenderView, USE_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DELIVER_OUTLINE_TO_CLIENT, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DELIVER_OUTLINE_TO_CLIENT_FOR_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DELIVER_LOD_TO_CLIENT, Integer);
vtkInformationKeyMacro(vtkPVRenderView, LOD_RESOLUTION, Double);
vtkInformationKeyMacro(vtkPVRenderView, NEED_ORDERED_COMPOSITING, Integer);
vtkInformationKeyMacro(vtkPVRenderView, REDISTRIBUTABLE_DATA_PRODUCER, ObjectBase);
vtkInformationKeyMacro(vtkPVRenderView, KD_TREE, ObjectBase);
vtkInformationKeyMacro(vtkPVRenderView, NEEDS_DELIVERY, Integer);

vtkCxxSetObjectMacro(vtkPVRenderView, LastSelection, vtkSelection);
//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();

  this->RemoteRenderingAvailable = false;

  this->UsedLODForLastRender = false;
  this->MakingSelection = false;
  this->StillRenderImageReductionFactor = 1;
  this->InteractiveRenderImageReductionFactor = 2;
  this->GeometrySize = 0;
  this->RemoteRenderingThreshold = 0;
  this->LODRenderingThreshold = 0;
  this->ClientOutlineThreshold = 5;
  this->LODResolution = 0.5;
  this->UseLightKit = false;
  this->Interactor = 0;
  this->InteractorStyle = 0;
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

  this->LastComputedBounds[0] = this->LastComputedBounds[2] =
    this->LastComputedBounds[4] = -1.0;
  this->LastComputedBounds[1] = this->LastComputedBounds[3] =
    this->LastComputedBounds[5] = 1.0;

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

  this->OrderedCompositingBSPCutsSource = vtkBSPCutsGenerator::New();

  if (this->Interactor)
    {
    this->InteractorStyle = vtkPVInteractorStyle::New();
    this->Interactor->SetRenderer(this->GetRenderer());
    this->Interactor->SetRenderWindow(this->GetRenderWindow());
    this->Interactor->SetInteractorStyle(this->InteractorStyle);

    // Add some default manipulators. Applications can override them without
    // much ado.
    vtkPVTrackballRotate* manip = vtkPVTrackballRotate::New();
    manip->SetButton(1);
    this->InteractorStyle->AddManipulator(manip);
    manip->Delete();

    vtkPVTrackballZoom* manip2 = vtkPVTrackballZoom::New();
    manip2->SetButton(3);
    this->InteractorStyle->AddManipulator(manip2);
    manip2->Delete();

    vtkTrackballPan* manip3 = vtkTrackballPan::New();
    manip3->SetButton(2);
    this->InteractorStyle->AddManipulator(manip3);
    manip3->Delete();

    this->RubberBandStyle = vtkInteractorStyleRubberBand3D::New();
    this->RubberBandStyle->RenderOnMouseMoveOff();
    vtkCommand* observer2 = vtkMakeMemberFunctionCommand(*this,
      &vtkPVRenderView::OnSelectionChangedEvent);
    this->RubberBandStyle->AddObserver(vtkCommand::SelectionChangedEvent,
      observer2);
    observer2->Delete();

    this->RubberBandZoom = vtkInteractorStyleRubberBandZoom::New();
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
    this->InteractorStyle->Delete();
    this->InteractorStyle = 0;
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

  this->OrderedCompositingBSPCutsSource->Delete();
  this->OrderedCompositingBSPCutsSource = NULL;
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
  this->SynchronizedWindows->AddRenderWindow(id, this->RenderView->GetRenderWindow());
  this->SynchronizedWindows->AddRenderer(id, this->RenderView->GetRenderer());
  this->SynchronizedWindows->AddRenderer(id, this->GetNonCompositedRenderer());

  this->SynchronizedRenderers->Initialize();
  this->SynchronizedRenderers->SetRenderer(this->RenderView->GetRenderer());

  this->Superclass::Initialize(id);

  this->RemoteRenderingAvailable = vtkPVDisplayInformation::CanOpenDisplayLocally();
  // Synchronize this among all processes involved.
  unsigned int cannot_render = (this->RemoteRenderingAvailable &&
                                vtkPVRenderView::IsRemoteRenderingAllowed()) ? 0 : 1;
  this->SynchronizeSize(cannot_render);
  this->RemoteRenderingAvailable = cannot_render == 0;
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
    this->InteractionMode = mode;
    this->Modified();

    if (this->Interactor == NULL)
      {
      return;
      }

    switch (this->InteractionMode)
      {
    case INTERACTION_MODE_3D:
    case INTERACTION_MODE_2D:
      this->Interactor->SetInteractorStyle(this->InteractorStyle);
      break;

    case INTERACTION_MODE_SELECTION:
      this->Interactor->SetInteractorStyle(this->RubberBandStyle);
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
void vtkPVRenderView::Select(int fieldAssociation, int region[4])
{
  // NOTE: selection is only supported in builtin or client-server mode. Not
  // supported in tile-display or batch modes.

  if (this->MakingSelection)
    {
    vtkErrorMacro("Select was called while making another selection.");
    return;
    }

  if (!this->GetRemoteRenderingAvailable())
    {
    vtkErrorMacro("Cannot make selections since remote rendering is not available.");
    return;
    }

  this->MakingSelection = true;

  bool render_event_propagation =
    this->SynchronizedWindows->GetRenderEventPropagation();
  this->SynchronizedWindows->RenderEventPropagationOff();
  // Make sure that the representations are up-to-date. This is required since
  // due to delayed-swicth-back-from-lod, the most recent render maybe a LOD
  // render (or a nonremote render) in which case we need to update the
  // representation pipelines correctly.
  this->Render(/*interactive*/false, /*skip-rendering*/false);

  this->SetLastSelection(NULL);

  this->Selector->SetRenderer(this->GetRenderer());
  this->Selector->SetFieldAssociation(fieldAssociation);
  // for now, we always do the process pass. In future, we can be smart about
  // disabling process pass when not needed.
  this->Selector->SetProcessID(
    vtkMultiProcessController::GetGlobalController()?
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() : 0);

  vtkSelection* sel = this->Selector->Select(region);
  if (sel)
    {
    // A valid sel is only generated on the "driver" node. The driver node may not have
    // the actual data (except in built-in mode). So representations on this
    // process may not be able to handle ConvertSelection() if call it right here.
    // Hence we broadcast the selection to all data-server nodes.
    this->FinishSelection(sel);
    sel->Delete();
    }
  else
    {
    vtkErrorMacro("Failed to capture selection.");
    }
  this->SynchronizedWindows->SetRenderEventPropagation(render_event_propagation);

  this->MakingSelection = false;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::FinishSelection(vtkSelection* sel)
{
  assert(sel != NULL);
  this->SynchronizedWindows->BroadcastToDataServer(sel);

  // not sel has PROP_ID() set and not PROP() pointers. We setup the PROP()
  // pointers, since representations have know knowledge for that the PROP_ID()s
  // are.
  for (unsigned int cc=0; cc < sel->GetNumberOfNodes(); cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node->GetProperties()->Has(vtkSelectionNode::PROP_ID()))
      {
      int propid = node->GetProperties()->Get(vtkSelectionNode::PROP_ID());
      vtkProp* prop = this->Selector->GetPropFromID(propid);
      node->GetProperties()->Set(vtkSelectionNode::PROP(), prop);
      }
    }

  // Now all processes have the full selection. We can tell the representations
  // to convert the selections.

  vtkSelection* converted = vtkSelection::New();

  // Now, vtkPVRenderView is in no position to tell how many representations got
  // selected, and what nodes in the vtkSelection correspond to which
  // representations. So it simply passes the full vtkSelection to all
  // representations and asks them to "convert" it. A representation will return
  // the original selection if it was not selected at all or returns the
  // converted selection for the part that it can handle, ignoring the rest.
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkDataRepresentation* repr = this->GetRepresentation(i);
    vtkSelection* convertedSelection = repr->ConvertSelection(this, sel);
    if (convertedSelection == NULL|| convertedSelection == sel)
      {
      continue;
      }
    for (unsigned int cc=0; cc < convertedSelection->GetNumberOfNodes(); cc++)
      {
      vtkSelectionNode* node = convertedSelection->GetNode(cc);
      // update the SOURCE() for the node to be the selected representation.
      node->GetProperties()->Set(vtkSelectionNode::SOURCE_ID(), i);
      converted->AddNode(convertedSelection->GetNode(cc));
      }
    convertedSelection->Delete();
    }

  this->SetLastSelection(converted);
  converted->FastDelete();

}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  //cout << "Clipping Bounds: "
  //  << this->LastComputedBounds[0] << ", "
  //  << this->LastComputedBounds[1] << ", "
  //  << this->LastComputedBounds[2] << ", "
  //  << this->LastComputedBounds[3] << ", "
  //  << this->LastComputedBounds[4] << ", "
  //  << this->LastComputedBounds[5] << endl;

  this->GetRenderer()->ResetCameraClippingRange(this->LastComputedBounds);
  this->GetNonCompositedRenderer()->ResetCameraClippingRange(this->LastComputedBounds);
}

#define PRINT_BOUNDS(bds)\
  bds[0] << "," << bds[1] << "," << bds[2] << "," << bds[3] << "," << bds[4] << "," << bds[5] << ","

//----------------------------------------------------------------------------
void vtkPVRenderView::GatherBoundsInformation(
  bool using_distributed_rendering)
{
  vtkMath::UninitializeBounds(this->LastComputedBounds);

  if (this->GetLocalProcessDoesRendering(using_distributed_rendering))
    {
    this->CenterAxes->SetUseBounds(0);
    // if ComputeVisiblePropBounds is called when there's no real window on the
    // local process, all vtkWidgetRepresentations return wacky Z bounds which
    // screws up the renderer and we don't see any images.
    this->GetRenderer()->ComputeVisiblePropBounds(this->LastComputedBounds);
    this->CenterAxes->SetUseBounds(1);
    }

  if (using_distributed_rendering)
    {
    // sync up bounds across all processes when doing distributed rendering.
    this->SynchronizedWindows->SynchronizeBounds(this->LastComputedBounds);
    }

  if (!vtkMath::AreBoundsInitialized(this->LastComputedBounds))
    {
    this->LastComputedBounds[0] = this->LastComputedBounds[2] =
      this->LastComputedBounds[4] = -1.0;
    this->LastComputedBounds[1] = this->LastComputedBounds[3] =
      this->LastComputedBounds[5] = 1.0;
    }
  //cout << "Bounds: "
  //  << this->LastComputedBounds[0] << ", "
  //  << this->LastComputedBounds[1] << ", "
  //  << this->LastComputedBounds[2] << ", "
  //  << this->LastComputedBounds[3] << ", "
  //  << this->LastComputedBounds[4] << ", "
  //  << this->LastComputedBounds[5] << endl;

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
    return using_distributed_rendering;
    }
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVRenderView::ResetCamera()
{
  // FIXME: Call update only when needed. That can be done at some point in the
  // future.
  this->Update();

  // Do all passes needed for rendering so that the geometry in the renderer is
  // updated. This is essential since the bounds are determined by using the
  // geometry bounds know to the renders on all processes. If they are not
  // updated, we will get wrong bounds.
  this->Render(false, true);

  // Remember, vtkRenderer::ResetCamera() calls
  // vtkRenderer::ResetCameraClippingPlanes() with the given bounds.
  this->RenderView->GetRenderer()->ResetCamera(this->LastComputedBounds);

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
void vtkPVRenderView::Update()
{
  vtkTimerLog::MarkStartEvent("RenderView::Update");
  this->Superclass::Update();

  // Do the vtkView::REQUEST_INFORMATION() pass.
  this->GatherRepresentationInformation();

  // Gather information about geometry sizes from all representations.
  this->GatherGeometrySizeInformation();

  // UpdateTime is used to determine if we need to redeliver the geometries.
  // Hence the more accurate we can be about when to update this time, the
  // better. Right now, we are relying on the client (vtkSMViewProxy) to ensure
  // that Update is called only when some representation is modified.
  this->UpdateTime.Modified();
  vtkTimerLog::MarkEndEvent("RenderView::Update");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StillRender()
{
  vtkTimerLog::MarkStartEvent("Still Render");
  this->GetRenderWindow()->SetDesiredUpdateRate(0.002);
  this->Render(false, false);
  vtkTimerLog::MarkEndEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::InteractiveRender()
{
  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->GetRenderWindow()->SetDesiredUpdateRate(5.0);
  this->Render(true, false);
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Render(bool interactive, bool skip_rendering)
{
  // Use loss-less image compression for client-server for full-res renders.
  this->SynchronizedRenderers->SetLossLessCompression(!interactive);

  bool use_lod_rendering = interactive? this->GetUseLODRendering() : false;
  this->SetRequestLODRendering(use_lod_rendering);

  // cout << "Using remote rendering: " << use_distributed_rendering << endl;
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
      in_cave_mode = true;
      }
    }

  // Decide if we are doing remote rendering or local rendering.
  bool use_distributed_rendering = in_cave_mode || this->GetUseDistributedRendering();

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

  // Build the request for REQUEST_PREPARE_FOR_RENDER().
  this->SetRequestDistributedRendering(use_distributed_rendering);

  if (in_tile_display_mode && this->GetDeliverOutlineToClient())
    {
    this->RequestInformation->Remove(DELIVER_LOD_TO_CLIENT());
    this->RequestInformation->Set(DELIVER_OUTLINE_TO_CLIENT(), 1);
    }
  else if (!in_tile_display_mode && this->GetDeliverOutlineToClient())
    {
    this->RequestInformation->Set(DELIVER_OUTLINE_TO_CLIENT_FOR_LOD(), 1);
    if (interactive && !use_distributed_rendering)
      {
      // also force LOD mode. Otherwise the outline is delivered only when LOD
      // mode is kicked in, even when the user requested that the LOD be
      // delivered to client for a lower threshold that the LOD threshold.
      use_lod_rendering = true;
      this->SetRequestLODRendering(use_lod_rendering);
      }
    }
  else
    {
    this->RequestInformation->Remove(DELIVER_OUTLINE_TO_CLIENT());
    this->RequestInformation->Set(DELIVER_LOD_TO_CLIENT(), 1);
    }

  if (in_cave_mode)
    {
    // FIXME: This isn't supported currently.
    this->RequestInformation->Set(DELIVER_LOD_TO_CLIENT(), 1);
    }
  else
    {
    this->RequestInformation->Remove(DELIVER_LOD_TO_CLIENT());
    }

  // In REQUEST_PREPARE_FOR_RENDER, this view expects all representations to
  // know the data-delivery mode. No representation should do any delivery here.
  // Only inform the view whether they will do delivery. Then the "driver" will
  // dictate what representations need to do delivery.
  this->CallProcessViewRequest(
    vtkPVView::REQUEST_PREPARE_FOR_RENDER(),
    this->RequestInformation, this->ReplyInformationVector);

  this->DoDataDelivery(use_lod_rendering, use_distributed_rendering);

  if (use_distributed_rendering &&
    this->OrderedCompositingBSPCutsSource->GetNumberOfInputConnections(0) > 0)
    {
    vtkMultiProcessController* controller =
      vtkMultiProcessController::GetGlobalController();
    if (controller && controller->GetNumberOfProcesses() > 1)
      {
      vtkStreamingDemandDrivenPipeline *sddp = vtkStreamingDemandDrivenPipeline::
        SafeDownCast(this->OrderedCompositingBSPCutsSource->GetExecutive());
      sddp->SetUpdateExtent
        (0,controller->GetLocalProcessId(),controller->GetNumberOfProcesses(),0);
      sddp->Update(0);
      }
    else
      {
      this->OrderedCompositingBSPCutsSource->Update();
      }
    this->SynchronizedRenderers->SetKdTree(
      this->OrderedCompositingBSPCutsSource->GetPKdTree());
    this->RequestInformation->Set(KD_TREE(),
      this->OrderedCompositingBSPCutsSource->GetPKdTree());
    }
  else
    {
    this->SynchronizedRenderers->SetKdTree(NULL);
    }

  this->CallProcessViewRequest(
    vtkPVView::REQUEST_RENDER(),
    this->RequestInformation, this->ReplyInformationVector);

  // set the image reduction factor.
  this->SynchronizedRenderers->SetImageReductionFactor(
    (interactive?
     this->InteractiveRenderImageReductionFactor :
     this->StillRenderImageReductionFactor));

  if (!interactive)
    {
    // Keep bounds information up-to-date.

    // GatherBoundsInformation will not do communication unless using
    // distributed rendering.
    this->GatherBoundsInformation(use_distributed_rendering);

    this->UpdateCenterAxes(this->LastComputedBounds);
    }

  this->UsedLODForLastRender = use_lod_rendering;

  if (interactive)
    {
    this->InteractiveRenderTime.Modified();
    }
  else
    {
    this->StillRenderTime.Modified();
    }

  if (skip_rendering)
    {
    return;
    }

  // When in batch mode, we are using the same render window for all views. That
  // makes it impossible for vtkPVSynchronizedRenderWindows to identify which
  // view is being rendered. We explicitly mark the view being rendered using
  // this HACK.
  this->SynchronizedWindows->BeginRender(this->GetIdentifier());

  // Call Render() on local render window only if
  // 1: Local process is the driver OR
  // 2: RenderEventPropagation is Off and we are doing distributed rendering.
  if (this->SynchronizedWindows->GetLocalProcessIsDriver() ||
    (!this->SynchronizedWindows->GetRenderEventPropagation() &&
     use_distributed_rendering))
    {
    this->GetRenderWindow()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::DoDataDelivery(
  bool using_lod_rendering, 
  bool vtkNotUsed(using_remote_rendering)
  )
{
  if ((using_lod_rendering &&
    this->InteractiveRenderTime > this->UpdateTime) ||
    (!using_lod_rendering &&
     this->StillRenderTime > this->UpdateTime))
    {
    //cout << "skipping delivery" << endl;
    return;
    }

  vtkMultiProcessController* s_controller =
    this->SynchronizedWindows->GetClientServerController();
  vtkMultiProcessController* d_controller =
    this->SynchronizedWindows->GetClientDataServerController();
  vtkMultiProcessController* p_controller =
    vtkMultiProcessController::GetGlobalController();

  vtkMultiProcessStream stream;
  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    // Tell everyone the representations that this process thinks are need to
    // delivery data.
    int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
    vtkstd::vector<int> need_delivery;
    for (int cc=0; cc < num_reprs; cc++)
      {
      vtkInformation* info =
        this->ReplyInformationVector->GetInformationObject(cc);
      if (info->Has(NEEDS_DELIVERY()) && info->Get(NEEDS_DELIVERY()) == 1)
        {
        need_delivery.push_back(cc);
        }
      }

    stream << static_cast<int>(need_delivery.size());
    for (size_t cc=0; cc < need_delivery.size(); cc++)
      {
      stream << need_delivery[cc];
      }

    if (s_controller)
      {
      s_controller->Send(stream, 1, 9998877);
      }
    if (d_controller)
      {
      d_controller->Send(stream, 1, 9998877);
      }
    if (p_controller)
      {
      p_controller->Broadcast(stream, 0);
      }
    }
  else
    {
    if (s_controller)
      {
      s_controller->Receive(stream, 1, 9998877);
      }
    if (d_controller)
      {
      d_controller->Receive(stream, 1, 9998877);
      }
    if (p_controller)
      {
      p_controller->Broadcast(stream, 0);
      }
    }
  int size;
  stream >> size;
  for (int cc=0; cc < size; cc++)
    {
    int index;
    stream >> index;
    vtkPVDataRepresentation* repr = vtkPVDataRepresentation::SafeDownCast(
      this->GetRepresentation(index));
    if (repr)
      {
      //cout << "Requesting Delivery: " << index << ": " << repr->GetClassName()
      //  << endl;
      repr->ProcessViewRequest(REQUEST_DELIVERY(), NULL, NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetRequestDistributedRendering(bool enable)
{
  bool in_tile_display_mode = this->InTileDisplayMode();
  bool in_cave_mode = this->SynchronizedWindows->GetIsInCave();
  if (in_cave_mode)
    {
    this->RequestInformation->Set(DATA_DISTRIBUTION_MODE(),
      vtkMPIMoveData::CLONE);
    }
  else if (enable)
    {
    this->RequestInformation->Set(DATA_DISTRIBUTION_MODE(),
      in_tile_display_mode?
      vtkMPIMoveData::COLLECT_AND_PASS_THROUGH:
      vtkMPIMoveData::PASS_THROUGH);
    }
  else
    {
    this->RequestInformation->Set(DATA_DISTRIBUTION_MODE(),
      in_tile_display_mode?
      vtkMPIMoveData::CLONE:
      vtkMPIMoveData::COLLECT);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetRequestLODRendering(bool enable)
{
  if (enable)
    {
    this->RequestInformation->Set(USE_LOD(), 1);
    this->RequestInformation->Set(LOD_RESOLUTION(), this->LODResolution);
    }
  else
    {
    this->RequestInformation->Remove(USE_LOD());
    this->RequestInformation->Remove(LOD_RESOLUTION());
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::GatherRepresentationInformation()
{
  this->CallProcessViewRequest(vtkPVView::REQUEST_INFORMATION(),
    this->RequestInformation, this->ReplyInformationVector);

  // REQUEST_UPDATE() pass may result in REDISTRIBUTABLE_DATA_PRODUCER() being
  // specified. If so, we update the OrderedCompositingBSPCutsSource to use
  // those producers as inputs, if ordered compositing maybe needed.
  static vtkstd::set<void*> previous_producers;

  this->LocalGeometrySize = 0;

  bool need_ordered_compositing = false;
  vtkstd::set<void*> current_producers;
  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* info =
      this->ReplyInformationVector->GetInformationObject(cc);
    if (info->Has(REDISTRIBUTABLE_DATA_PRODUCER()))
      {
      current_producers.insert(info->Get(REDISTRIBUTABLE_DATA_PRODUCER()));
      }
    if (info->Has(NEED_ORDERED_COMPOSITING()))
      {
      need_ordered_compositing = true;
      }
    if (info->Has(GEOMETRY_SIZE()))
      {
      this->LocalGeometrySize += (info->Get(GEOMETRY_SIZE())/1024.0);
      }
    }

  if (!this->GetUseOrderedCompositing() || !need_ordered_compositing)
    {
    this->OrderedCompositingBSPCutsSource->RemoveAllInputs();
    previous_producers.clear();
    return;
    }

  if (current_producers != previous_producers)
    {
    this->OrderedCompositingBSPCutsSource->RemoveAllInputs();
    vtkstd::set<void*>::iterator iter;
    for (iter = current_producers.begin(); iter != current_producers.end();
      ++iter)
      {
      this->OrderedCompositingBSPCutsSource->AddInputConnection(0,
        reinterpret_cast<vtkAlgorithm*>(*iter)->GetOutputPort(0));
      }
    previous_producers = current_producers;
    }

}

//----------------------------------------------------------------------------
void vtkPVRenderView::GatherGeometrySizeInformation()
{
  this->GeometrySize = this->LocalGeometrySize;
  this->SynchronizedWindows->SynchronizeSize(this->GeometrySize);
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseDistributedRendering()
{
  if (this->GetRemoteRenderingAvailable() == false)
    {
    return false;
    }

  if (this->MakingSelection)
    {
    // force remote rendering when doing a surface selection.
    return true;
    }

  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_BATCH)
    {
    // currently, we only support parallel rendering in batch mode.
    return true;
    }

  return this->RemoteRenderingThreshold <= this->GeometrySize;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseLODRendering()
{
  // return false;
  return this->LODRenderingThreshold <= this->GeometrySize;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetDeliverOutlineToClient()
{
//  return false;
  return this->ClientOutlineThreshold <= this->GeometrySize;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseOrderedCompositing()
{
  if (this->SynchronizedWindows->GetIsInCave())
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
void vtkPVRenderView::AddPropToNonCompositedRenderer(vtkProp* prop)
{
  this->GetNonCompositedRenderer()->AddActor(prop);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemovePropFromNonCompositedRenderer(vtkProp* prop)
{
  this->GetNonCompositedRenderer()->RemoveActor(prop);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AddPropToRenderer(vtkProp* prop)
{
  this->GetRenderer()->AddActor(prop);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemovePropFromRenderer(vtkProp* prop)
{
  this->GetRenderer()->RemoveActor(prop);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateCenterAxes(double bounds[6])
{
  vtkBoundingBox bbox(bounds);
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
void vtkPVRenderView::SetNonInteractiveRenderDelay(unsigned int seconds)
{
  if (this->Interactor)
    {
    this->Interactor->SetNonInteractiveRenderDelay(seconds*1000);
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
void vtkPVRenderView::AddManipulator(vtkCameraManipulator* val)
{
  if (this->InteractorStyle)
    {
    this->InteractorStyle->AddManipulator(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveAllManipulators()
{
  if (this->InteractorStyle)
    {
    this->InteractorStyle->RemoveAllManipulators();
    }
}
//----------------------------------------------------------------------------
bool vtkPVRenderView::IsRemoteRenderingAllowed()
{
  return vtkPVRenderView::RemoteRenderingAllowed;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AllowRemoteRendering(bool allowRemoteRendering)
{
  vtkPVRenderView::RemoteRenderingAllowed = allowRemoteRendering;
}
