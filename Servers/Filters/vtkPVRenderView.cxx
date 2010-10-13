/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

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
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkMath.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPHardwareSelector.h"
#include "vtkPKdTree.h"
#include "vtkProcessModule.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVGenericRenderWindowInteractor.h"
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
#include "vtkTimerLog.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <vtkstd/vector>
#include <vtkstd/set>

vtkStandardNewMacro(vtkPVRenderView);
vtkInformationKeyMacro(vtkPVRenderView, GEOMETRY_SIZE, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DATA_DISTRIBUTION_MODE, Integer);
vtkInformationKeyMacro(vtkPVRenderView, USE_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DELIVER_OUTLINE_TO_CLIENT, Integer);
vtkInformationKeyMacro(vtkPVRenderView, DELIVER_LOD_TO_CLIENT, Integer);
vtkInformationKeyMacro(vtkPVRenderView, LOD_RESOLUTION, Double);
vtkInformationKeyMacro(vtkPVRenderView, NEED_ORDERED_COMPOSITING, Integer);
vtkInformationKeyMacro(vtkPVRenderView, REDISTRIBUTABLE_DATA_PRODUCER, ObjectBase);
vtkInformationKeyMacro(vtkPVRenderView, KD_TREE, ObjectBase);
vtkCxxSetObjectMacro(vtkPVRenderView, LastSelection, vtkSelection);
//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();

  this->ForceRemoteRendering = false;
  this->StillRenderImageReductionFactor = 1;
  this->InteractiveRenderImageReductionFactor = 2;
  this->GeometrySize = 0;
  this->RemoteRenderingThreshold = 0;
  this->LODRenderingThreshold = 0;
  this->ClientOutlineThreshold = 100;
  this->LODResolution = 0.5;
  this->UseLightKit = false;
  this->Interactor = 0;
  this->InteractorStyle = 0;
  this->RubberBandStyle = 0;
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
  this->Selector = vtkPHardwareSelector::New();

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

    this->RubberBandStyle = vtkInteractorStyleRubberBand3D::New();
    this->RubberBandStyle->RenderOnMouseMoveOff();
    vtkCommand* observer = vtkMakeMemberFunctionCommand(*this,
      &vtkPVRenderView::OnSelectionChangedEvent);
    this->RubberBandStyle->AddObserver(vtkCommand::SelectionChangedEvent,
      observer);
    observer->Delete();

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
  this->SynchronizedRenderers->SetRenderer(this->RenderView->GetRenderer());

  this->Superclass::Initialize(id);
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
  this->SetLastSelection(NULL);

  this->Selector->SetRenderer(this->GetRenderer());
  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    this->Selector->SetProcessIsRoot(true);
    }
  else
    {
    this->Selector->SetProcessIsRoot(false);
    }
  this->Selector->SetArea(region[0], region[1], region[2], region[3]);
  this->Selector->SetFieldAssociation(fieldAssociation);
  // for now, we always do the process pass. In future, we can be smart about
  // disabling process pass when not needed.
  this->Selector->SetProcessID(
    vtkMultiProcessController::GetGlobalController()?
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() : 0);
  vtkSelection* sel = this->Selector->Select();
  if (sel)
    {
    // sel is only generated on the "driver" node. The driver node may not have
    // the actual data (except in built-in mode). So representations on this
    // process may not be able to handle ConvertSelection() if call it right here.
    // Hence we broadcast the selection to all data-server nodes.
    this->FinishSelection(sel);
    sel->Delete();
    }
  else
    {
    vtkMemberFunctionCommand<vtkPVRenderView>* observer =
      vtkMemberFunctionCommand<vtkPVRenderView>::New();
    observer->SetCallback(*this, &vtkPVRenderView::FinishSelection);
    this->Selector->AddObserver(vtkCommand::EndEvent, observer);
    observer->FastDelete();
    }

}

//----------------------------------------------------------------------------
void vtkPVRenderView::FinishSelection()
{
  this->FinishSelection(NULL);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::FinishSelection(vtkSelection* sel)
{
  this->Selector->RemoveObservers(vtkCommand::EndEvent);

  if (sel==NULL)
    {
    sel = vtkSelection::New();
    }
  else
    {
    sel->Register(this);
    }
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
      vtkProp* prop = this->Selector->GetProp(propid);
      node->SetSelectedProp(prop);
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
  sel->Delete();

  this->SetLastSelection(converted);
  converted->FastDelete();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  this->GetRenderer()->ResetCameraClippingRange(this->LastComputedBounds);
  this->GetNonCompositedRenderer()->ResetCameraClippingRange(this->LastComputedBounds);
}

#define PRINT_BOUNDS(bds)\
  bds[0] << "," << bds[1] << "," << bds[2] << "," << bds[3] << "," << bds[4] << "," << bds[5] << ","

//----------------------------------------------------------------------------
void vtkPVRenderView::GatherBoundsInformation()
{
  // FIXME: when doing client-only render, we are wasting our energy computing
  // universal bounds. How can we fix that?
  vtkMath::UninitializeBounds(this->LastComputedBounds);

  if (this->GetRenderWindow()->GetActualSize()[0] > 0 &&
    this->GetRenderWindow()->GetActualSize()[1] > 0)
    {
    // if ComputeVisiblePropBounds is called when there's no real window on the
    // local process, all vtkWidgetRepresentations return wacky Z bounds which
    // screws up the renderer and we don't see any images.
    this->GetRenderer()->ComputeVisiblePropBounds(this->LastComputedBounds);
    }

  this->SynchronizedWindows->SynchronizeBounds(this->LastComputedBounds);
  if (!vtkMath::AreBoundsInitialized(this->LastComputedBounds))
    {
    this->LastComputedBounds[0] = this->LastComputedBounds[2] =
      this->LastComputedBounds[4] = -1.0;
    this->LastComputedBounds[1] = this->LastComputedBounds[3] =
      this->LastComputedBounds[5] = 1.0;
    }
  this->ResetCameraClippingRange();
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVRenderView::ResetCamera()
{
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
  if (!interactive)
    {
    // Update all representations.
    // This should update mostly just the inputs to the representations, and maybe
    // the internal geometry filter.
    this->Update();

    // Do the vtkView::REQUEST_INFORMATION() pass.
    this->GatherRepresentationInformation();
    }

  // Gather information about geometry sizes from all representations.
  this->GatherGeometrySizeInformation();

  bool use_lod_rendering = interactive? this->GetUseLODRendering() : false;
  this->SetRequestLODRendering(use_lod_rendering);

  // Decide if we are doing remote rendering or local rendering.
  bool use_distributed_rendering = this->GetUseDistributedRendering();
  // cout << "Using remote rendering: " << use_distributed_rendering << endl;
  bool in_tile_display_mode = this->InTileDisplayMode();

  // When in tile-display mode, we are always doing shared rendering. However
  // when use_distributed_rendering we tell IceT that geometry is duplicated on
  // all processes.
  this->SynchronizedWindows->SetEnabled(
    use_distributed_rendering || in_tile_display_mode);
  this->SynchronizedRenderers->SetEnabled(
    use_distributed_rendering || in_tile_display_mode);
  this->SynchronizedRenderers->SetDataReplicatedOnAllProcesses(
    !use_distributed_rendering && in_tile_display_mode);

  // Build the request for REQUEST_PREPARE_FOR_RENDER().
  this->SetRequestDistributedRendering(use_distributed_rendering);

  if (in_tile_display_mode)
    {
    if (this->GetDeliverOutlineToClient())
      {
      this->RequestInformation->Remove(DELIVER_LOD_TO_CLIENT());
      this->RequestInformation->Set(DELIVER_OUTLINE_TO_CLIENT(), 1);
      }
    else
      {
      this->RequestInformation->Remove(DELIVER_OUTLINE_TO_CLIENT());
      this->RequestInformation->Set(DELIVER_LOD_TO_CLIENT(), 1);
      }
    }
  else
    {
    this->RequestInformation->Remove(DELIVER_LOD_TO_CLIENT());
    this->RequestInformation->Remove(DELIVER_OUTLINE_TO_CLIENT());
    }

  // In REQUEST_PREPARE_FOR_RENDER, this view expects all representations to
  // know the data-delivery mode.
  this->CallProcessViewRequest(
    vtkPVView::REQUEST_PREPARE_FOR_RENDER(),
    this->RequestInformation, this->ReplyInformationVector);

  if (use_distributed_rendering &&
    this->OrderedCompositingBSPCutsSource->GetNumberOfInputConnections(0) > 0)
    {
    this->OrderedCompositingBSPCutsSource->Update();
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
    // FIXME: How can be make this so that we don't have to do parallel
    // communication each time.
    this->GatherBoundsInformation();
    this->UpdateCenterAxes(this->LastComputedBounds);
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
void vtkPVRenderView::SetRequestDistributedRendering(bool enable)
{
  bool in_tile_display_mode = this->InTileDisplayMode();
  if (enable)
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
      this->LocalGeometrySize += info->Get(GEOMETRY_SIZE());
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
  if (this->ForceRemoteRendering)
    {
    // force remote rendering when doing a surface selection.
    return true;
    }

  if (vtkProcessModule::GetProcessModule()->GetOptions()->GetProcessType()
    == vtkPVOptions::PVBATCH)
    {
    // currently, we only support parallel rendering in batch mode.
    return true;
    }

  return (this->RemoteRenderingThreshold*1024) <= this->GeometrySize;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseLODRendering()
{
  // return false;
  return (this->LODRenderingThreshold*1024) <= this->GeometrySize;
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
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  switch (options->GetProcessType())
    {
  case vtkPVOptions::PVSERVER:
  case vtkPVOptions::PVBATCH:
  case vtkPVOptions::PVRENDER_SERVER:
    if (vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions() > 1)
      {
      return true;
      }
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
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseLightKit: " << this->UseLightKit << endl;
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
  this->GetRenderer()->SetGradientBackground(val);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetTexturedBackground(int val)
{
  this->GetRenderer()->SetTexturedBackground(val);
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
