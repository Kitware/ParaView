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
#include "vtkPVDataRepresentation.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVHardwareSelector.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVOptions.h"
#include "vtkPVSession.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkRenderer.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRepresentedDataStorage.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTrackballPan.h"
#include "vtkTrivialProducer.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <vector>
#include <set>
#include <map>

class vtkPVRenderView::vtkInternals
{
public:
  unsigned int UniqueId;
  vtkNew<vtkRepresentedDataStorage> GeometryStore;
};


//----------------------------------------------------------------------------
// Statics
//----------------------------------------------------------------------------
bool vtkPVRenderView::RemoteRenderingAllowed = true;
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderView);
vtkInformationKeyMacro(vtkPVRenderView, USE_LOD, Integer);
vtkInformationKeyMacro(vtkPVRenderView, LOD_RESOLUTION, Double);
vtkInformationKeyMacro(vtkPVRenderView, NEED_ORDERED_COMPOSITING, Integer);
vtkInformationKeyMacro(vtkPVRenderView, REPRESENTED_DATA_STORE, ObjectBase);
vtkCxxSetObjectMacro(vtkPVRenderView, LastSelection, vtkSelection);
//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  this->Internals = new vtkInternals();
  this->Internals->GeometryStore->SetView(this);

  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();

  this->RemoteRenderingAvailable = vtkPVRenderView::RemoteRenderingAllowed;

  this->StillRenderProcesses = vtkPVSession::NONE;
  this->InteractiveRenderProcesses = vtkPVSession::NONE;
  this->UsedLODForLastRender = false;
  this->UseLODForInteractiveRender = false;
  this->UseOutlineForInteractiveRender = false;
  this->UseDistributedRenderingForStillRender = false;
  this->UseDistributedRenderingForInteractiveRender = false;
  this->MakingSelection = false;
  this->StillRenderImageReductionFactor = 1;
  this->InteractiveRenderImageReductionFactor = 2;
  this->RemoteRenderingThreshold = 0;
  this->LODRenderingThreshold = 0;
  this->ClientOutlineThreshold = 5;
  this->LODResolution = 0.5;
  this->UseLightKit = false;
  this->Interactor = 0;
  this->InteractorStyle = 0;
  this->TwoDInteractorStyle = 0;
  this->ThreeDInteractorStyle = 0;
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
  this->SynchronizationCounter  = 0;
  this->PreviousParallelProjectionStatus = 0;
  this->NeedsOrderedCompositing = false;

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

  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
vtkRepresentedDataStorage* vtkPVRenderView::GetGeometryStore()
{
  return this->Internals->GeometryStore.GetPointer();
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
    // We only increase that counter when widget are not involved as in
    // collaboration mode only the master has the widget in its representation
    this->SynchronizationCounter++;
    unsigned int id = this->Internals->UniqueId++;
    this->Internals->GeometryStore->RegisterRepresentation(id, dataRep);
    }

  this->Superclass::AddRepresentationInternal(rep);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{
  vtkPVDataRepresentation* dataRep = vtkPVDataRepresentation::SafeDownCast(rep);
  if (dataRep != NULL)
    {
    this->Internals->GeometryStore->UnRegisterRepresentation(dataRep);

    // We only increase that counter when widget are not involved as in
    // collaboration mode only the master has the widget in its representation
    this->SynchronizationCounter++;
    }

  this->Superclass::RemoveRepresentationInternal(rep);
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

  // look at ::Render(..,..). We need to disable these once we are done with
  // rendering.
  this->SynchronizedWindows->SetEnabled(false);
  this->SynchronizedRenderers->SetEnabled(false);

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

  this->MakingSelection = false;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::FinishSelection(vtkSelection* sel)
{
  assert(sel != NULL);
  this->SynchronizedWindows->BroadcastToDataServer(sel);

  // now, sel has PROP_ID() set and not PROP() pointers. We setup the PROP()
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
  if (this->GeometryBounds.IsValid())
    {
    double bounds[6];
    this->GeometryBounds.GetBounds(bounds);
    this->GetRenderer()->ResetCameraClippingRange(bounds);
    this->GetNonCompositedRenderer()->ResetCameraClippingRange(bounds);
    }
}

#define PRINT_BOUNDS(bds)\
  bds[0] << "," << bds[1] << "," << bds[2] << "," << bds[3] << "," << bds[4] << "," << bds[5] << ","

//----------------------------------------------------------------------------
void vtkPVRenderView::SynchronizeGeometryBounds()
{
  double bounds[6];
  vtkMath::UninitializeBounds(bounds);
  if (this->GeometryBounds.IsValid())
    {
    this->GeometryBounds.GetBounds(bounds);
    }

  // sync up bounds across all processes when doing distributed rendering.
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
bool vtkPVRenderView::SynchronizeForCollaboration()
{
  bool counterSynchronizedSuccessfully = false;

  // Also, can we optimize this further? This happens on every render in
  // collaborative mode.
  vtkMultiProcessController* p_controller =
    this->SynchronizedWindows->GetParallelController();
  vtkMultiProcessController* d_controller = 
    this->SynchronizedWindows->GetClientDataServerController();
  vtkMultiProcessController* r_controller =
    this->SynchronizedWindows->GetClientServerController();
  if (d_controller != NULL)
    {
    vtkErrorMacro("RenderServer-DataServer configuration is not supported in "
      "collabortion mode.");
    abort();
    }

  if (this->SynchronizedWindows->GetMode() == vtkPVSynchronizedRenderWindows::CLIENT)
    {
    vtkMultiProcessStream stream;
    stream << this->SynchronizationCounter << this->RemoteRenderingThreshold;
    r_controller->Send(stream, 1, 41000);
    int server_sync_counter;
    r_controller->Receive(&server_sync_counter, 1, 1, 41001);
    counterSynchronizedSuccessfully =
      (server_sync_counter == this->SynchronizationCounter);
    }
  else
    {
    if (r_controller)
      {
      vtkMultiProcessStream stream;
      r_controller->Receive(stream, 1, 41000);
      int client_sync_counter;
      stream >> client_sync_counter >> this->RemoteRenderingThreshold;
      r_controller->Send(&this->SynchronizationCounter, 1, 1, 41001 );
      counterSynchronizedSuccessfully =
        (client_sync_counter == this->SynchronizationCounter);
      }

    if (p_controller)
      {
      p_controller->Broadcast(&this->RemoteRenderingThreshold, 1, 0);
      int temp = counterSynchronizedSuccessfully? 1 : 0;
      p_controller->Broadcast(&temp, 1, 0);
      counterSynchronizedSuccessfully = (temp == 1);
      }
    }
  return counterSynchronizedSuccessfully;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Update()
{
  vtkTimerLog::MarkStartEvent("RenderView::Update");

  // reset the bounds, so that representations can provide us with bounds
  // information during update.
  this->GeometryBounds.Reset();

  this->RequestInformation->Set(REPRESENTED_DATA_STORE(),
    this->Internals->GeometryStore.GetPointer());

  this->Superclass::Update();

  // After every update we can expect the representation geometries to change.
  // Thus we need to determine whether we are doing to remote-rendering or not,
  // use-lod or not, etc. All these decisions are made right here to avoid
  // making them during each render-call.

  // Check if any representation told us that it needed ordered compositing.
  this->NeedsOrderedCompositing = false;
  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* info =
      this->ReplyInformationVector->GetInformationObject(cc);
    if (info->Has(NEED_ORDERED_COMPOSITING()) &&
      info->Get(NEED_ORDERED_COMPOSITING()) != 0)
      {
      this->NeedsOrderedCompositing= true;
      break;
      }
    }

  // Gather information about geometry sizes from all representations.
  double local_size = this->GetGeometryStore()->GetVisibleDataSize(false) / 1024.0;
  this->SynchronizedWindows->SynchronizeSize(local_size);
  // cout << "Full Geometry size: " << local_size << endl;

  // Update decisions about lod-rendering and remote-rendering.
  this->UseLODForInteractiveRender = this->ShouldUseLODRendering(local_size);
  this->UseDistributedRenderingForStillRender = this->ShouldUseDistributedRendering(local_size);
  if (!this->UseLODForInteractiveRender)
    {
    this->UseDistributedRenderingForInteractiveRender =
      this->UseDistributedRenderingForStillRender;
    this->UseOutlineForInteractiveRender = (this->ClientOutlineThreshold <= local_size);
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
}

//----------------------------------------------------------------------------
void vtkPVRenderView::UpdateLOD()
{
  vtkTimerLog::MarkStartEvent("RenderView::UpdateLOD");
  this->RequestInformation->Set(REPRESENTED_DATA_STORE(),
    this->Internals->GeometryStore.GetPointer());

  // Update LOD geometry.
  this->CallProcessViewRequest(
    vtkPVView::REQUEST_UPDATE_LOD(),
    this->RequestInformation, this->ReplyInformationVector);

  double local_size = this->GetGeometryStore()->GetVisibleDataSize(true) / 1024.0;
  this->SynchronizedWindows->SynchronizeSize(local_size);
  // cout << "LOD Geometry size: " << local_size << endl;

  this->UseOutlineForInteractiveRender = (this->ClientOutlineThreshold <= local_size);
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

  // Render each representation with available geometry.
  // This is the pass where representations get an opportunity to get the
  // currently "available" represented data and try to render it.
  this->RequestInformation->Set(REPRESENTED_DATA_STORE(),
    this->Internals->GeometryStore.GetPointer());
  this->CallProcessViewRequest(
    vtkPVView::REQUEST_RENDER(),
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

  this->SynchronizedRenderers->SetKdTree(
    this->Internals->GeometryStore->GetKdTree());

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
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return;
    }
  storage->SetPiece(repr, data, false);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVRenderView::GetPieceProducer(vtkInformation* info,
    vtkPVDataRepresentation* repr)
{
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return NULL;
    }
  return storage->GetProducer(repr, false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPieceLOD(vtkInformation* info,
  vtkPVDataRepresentation* repr, vtkDataObject* data)
{
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return;
    } 
  storage->SetPiece(repr, data, true);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVRenderView::GetPieceProducerLOD(vtkInformation* info,
    vtkPVDataRepresentation* repr)
{
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return NULL;
    }

  return storage->GetProducer(repr, true);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::MarkAsRedistributable(
  vtkInformation* info, vtkPVDataRepresentation* repr)
{
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return;
    }
  storage->MarkAsRedistributable(repr);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDeliverToAllProcesses(vtkInformation* info,
  vtkPVDataRepresentation* repr, bool clone)
{
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return;
    }
  storage->SetDeliverToAllProcesses(repr, clone, false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetDeliverLODToAllProcesses(vtkInformation* info,
  vtkPVDataRepresentation* repr, bool clone)
{
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return;
    }
  storage->SetDeliverToAllProcesses(repr, clone, true);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetGeometryBounds(vtkInformation* info,
  double bounds[6], vtkMatrix4x4* matrix /*=NULL*/)
{
  // FIXME: I need a cleaner way for accessing the render view.
  vtkRepresentedDataStorage* storage =
    vtkRepresentedDataStorage::SafeDownCast(
      info->Get(REPRESENTED_DATA_STORE()));
  if (!storage)
    {
    vtkGenericWarningMacro("Missing REPRESENTED_DATA_STORE().");
    return;
    }

  vtkPVRenderView* self = vtkPVRenderView::SafeDownCast(storage->GetView());
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
bool vtkPVRenderView::ShouldUseDistributedRendering(double geometry_size)
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

  if (!this->NeedsOrderedCompositing)
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
