/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRenderViewProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVLastSelectionInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMDataDeliveryManager.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMaterialLibraryProxy.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMViewProxyInteractorHelper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <map>

vtkStandardNewMacro(vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
  : InteractorHelper()
{
  this->IsSelectionCached = false;
  this->NewMasterObserverId = 0;
  this->DeliveryManager = NULL;
  this->NeedsUpdateLOD = true;
  this->InteractorHelper->SetViewProxy(this);
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy()
{
  this->InteractorHelper->SetViewProxy(NULL);
  this->InteractorHelper->CleanupInteractor();

  if (this->NewMasterObserverId != 0 && this->Session && this->Session->GetCollaborationManager())
  {
    this->Session->GetCollaborationManager()->RemoveObserver(this->NewMasterObserverId);
    this->NewMasterObserverId = 0;
  }

  if (this->DeliveryManager)
  {
    this->DeliveryManager->Delete();
    this->DeliveryManager = NULL;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::LastRenderWasInteractive()
{
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetUsedLODForLastRender() : false;
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::IsSelectionAvailable()
{
  const char* msg = this->IsSelectVisibleCellsAvailable();
  if (msg)
  {
    // vtkErrorMacro(<< msg);
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisibleCellsAvailable()
{
  vtkSMSession* session = this->GetSession();

  if (session->IsMultiClients() && !session->GetCollaborationManager()->IsMaster())
  {
    return "Cannot support selection in collaboration mode when not MASTER";
  }

  if (session->GetController(vtkPVSession::DATA_SERVER_ROOT) !=
    session->GetController(vtkPVSession::RENDER_SERVER_ROOT))
  {
    // when the two controller are different, we have a separate render-server
    // and data-server session.
    return "Cannot support selection in render-server mode";
  }

  vtkPVServerInformation* server_info = session->GetServerInformation();
  if (server_info && server_info->GetNumberOfMachines() > 0)
  {
    return "Cannot support selection in CAVE mode.";
  }

  // check if we don't have enough color depth to do color buffer selection
  // if we don't then disallow selection
  int rgba[4];
  vtkRenderWindow* rwin = this->GetRenderWindow();
  if (!rwin)
  {
    return "No render window available";
  }

  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
  {
    return "Selection not supported due to insufficient color depth.";
  }

  return NULL;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisiblePointsAvailable()
{
  return this->IsSelectVisibleCellsAvailable();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::Update()
{
  this->NeedsUpdateLOD |= this->NeedsUpdate;
  this->Superclass::Update();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::UpdateLOD()
{
  if (this->ObjectsCreated && this->NeedsUpdateLOD)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "UpdateLOD"
           << vtkClientServerStream::End;
    this->GetSession()->PrepareProgress();
    this->ExecuteStream(stream);
    this->GetSession()->CleanupPendingProgress();

    this->NeedsUpdateLOD = false;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::GetNeedsUpdate()
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  if (view->GetUseInteractiveRenderingForScreenshots() && view->GetUseLODForInteractiveRender())
  {
    return this->NeedsUpdateLOD;
  }
  else
  {
    return this->NeedsUpdate;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::StreamingUpdate(bool render_if_needed)
{
  // FIXME: add a check to not do anything when in multi-client mode. We don't
  // support streaming in multi-client mode.
  this->GetSession()->PrepareProgress();

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  double planes[24];
  vtkRenderer* ren = view->GetRenderer();
  ren->GetActiveCamera()->GetFrustumPlanes(ren->GetTiledAspectRatio(), planes);

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "StreamingUpdate"
         << vtkClientServerStream::InsertArray(planes, 24) << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  // Now fetch any pieces that the server streamed back to the client.
  bool something_delivered = this->DeliveryManager->DeliverStreamedPieces();
  bool OSPRayNotDone = view->GetOSPRayContinueStreaming();
  if (render_if_needed && (something_delivered || OSPRayNotDone))
  {
    this->StillRender();
  }

  this->GetSession()->CleanupPendingProgress();
  return something_delivered || OSPRayNotDone;
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMRenderViewProxy::PreRender(bool interactive)
{
  this->Superclass::PreRender(interactive);

  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  assert(rv != NULL);

  if (interactive && rv->GetUseLODForInteractiveRender())
  {
    // for interactive renders, we need to determine if we are going to use LOD.
    // If so, we may need to update the LOD geometries.
    this->UpdateLOD();
  }
  this->DeliveryManager->Deliver(interactive);
  return interactive ? rv->GetInteractiveRenderProcesses() : rv->GetStillRenderProcesses();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PostRender(bool interactive)
{
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  this->SynchronizeCameraProperties();
  this->Superclass::PostRender(interactive);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SynchronizeCameraProperties()
{
  if (!this->ObjectsCreated)
  {
    return;
  }

  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  vtkSMPropertyIterator* iter = cameraProxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProperty* cur_property = iter->GetProperty();
    vtkSMProperty* info_property = cur_property->GetInformationProperty();
    if (!info_property)
    {
      continue;
    }
    cur_property->Copy(info_property);
    // cur_property->UpdateLastPushedValues();
  }
  iter->Delete();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkSMRenderViewProxy::GetRenderer()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetRenderer() : NULL;
}

//----------------------------------------------------------------------------
vtkCamera* vtkSMRenderViewProxy::GetActiveCamera()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetActiveCamera() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  if (this->GetLocalProcessSupportsInteraction())
  {
    this->CreateVTKObjects();
    vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());

    // Remember, these calls end up changing ivars on iren.
    rv->SetupInteractor(iren);
    this->InteractorHelper->SetupInteractor(rv->GetInteractor());
  }
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkSMRenderViewProxy::GetInteractor()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetInteractor() : NULL;
}

//----------------------------------------------------------------------------
vtkSMViewProxyInteractorHelper* vtkSMRenderViewProxy::GetInteractorHelper()
{
  return this->InteractorHelper.GetPointer();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMRenderViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetRenderWindow() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go thurther...
  if (this->Location == 0)
  {
    return;
  }

  if (!this->ObjectsCreated)
  {
    return;
  }

  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());

  vtkCamera* camera =
    vtkCamera::SafeDownCast(this->GetSubProxy("ActiveCamera")->GetClientSideObject());
  rv->SetActiveCamera(camera);

  vtkEventForwarderCommand* forwarder = vtkEventForwarderCommand::New();
  forwarder->SetTarget(this);
  rv->AddObserver(vtkCommand::SelectionChangedEvent, forwarder);
  rv->AddObserver(vtkCommand::ResetCameraEvent, forwarder);
  forwarder->Delete();

  // We'll do this for now. But we need to not do this here. I am leaning
  // towards not making stereo a command line option as mentioned by a very
  // not-too-pleased user on the mailing list a while ago.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* pvoptions = pm->GetOptions();
  if (pvoptions->GetUseStereoRendering())
  {
    vtkSMPropertyHelper(this, "StereoCapableWindow").Set(1);
    vtkSMPropertyHelper(this, "StereoRender").Set(1);
    auto domain = this->GetProperty("StereoType")->FindDomain<vtkSMEnumerationDomain>();
    if (domain && domain->HasEntryText(pvoptions->GetStereoType()))
    {
      vtkSMPropertyHelper(this, "StereoType")
        .Set(domain->GetEntryValueForText(pvoptions->GetStereoType()));
    }
  }

  bool remote_rendering_available = true;
  if (this->GetSession()->GetIsAutoMPI())
  {
    // When the session is an auto-mpi session, we don't support remote
    // rendering.
    remote_rendering_available = false;
  }

  if (remote_rendering_available)
  {
    // Update whether render servers can open display i.e. remote rendering is
    // possible on all processes.
    vtkNew<vtkPVRenderingCapabilitiesInformation> info;
    this->GetSession()->GatherInformation(vtkPVSession::RENDER_SERVER, info.Get(), 0);
    if (!info->Supports(vtkPVRenderingCapabilitiesInformation::OPENGL))
    {
      remote_rendering_available = false;
    }
  }

  // Disable remote rendering on all processes, if not available.
  if (remote_rendering_available == false)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "RemoteRenderingAvailableOff"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
  }

  const bool enable_nvpipe = this->GetSession()->GetServerInformation()->GetNVPipeSupport();
  {
    vtkClientServerStream strm;
    strm << vtkClientServerStream::Invoke << VTKOBJECT(this);
    if (enable_nvpipe)
    {
      strm << "NVPipeAvailableOn";
    }
    else
    {
      strm << "NVPipeAvailableOff";
    }
    strm << vtkClientServerStream::End;
    this->ExecuteStream(strm);
  }

  // Attach to the collaborative session a callback to clear the selection cache
  // on the server side when we became master
  if (this->Session->IsMultiClients())
  {
    this->NewMasterObserverId = this->Session->GetCollaborationManager()->AddObserver(
      vtkSMCollaborationManager::UpdateMasterUser, this, &vtkSMRenderViewProxy::NewMasterCallback);
  }

  // Setup data-delivery manager.
  this->DeliveryManager = vtkSMDataDeliveryManager::New();
  this->DeliveryManager->SetViewProxy(this);
}

//----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer);

  if (const char* reprName = this->Superclass::GetRepresentationType(producer, outputPort))
  {
    return reprName;
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  const char* representationsToTry[] = { "UnstructuredGridRepresentation",
    "StructuredGridRepresentation", "UniformGridRepresentation", "AMRRepresentation",
    "PVMoleculeRepresentation", "GeometryRepresentation", NULL };
  for (int cc = 0; representationsToTry[cc] != NULL; ++cc)
  {
    vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", representationsToTry[cc]);
    if (prototype)
    {
      vtkSMProperty* inputProp = prototype->GetProperty("Input");
      vtkSMUncheckedPropertyHelper helper(inputProp);
      helper.Set(producer, outputPort);
      bool acceptable = (inputProp->IsInDomains() > 0);
      helper.SetNumberOfElements(0);
      if (acceptable)
      {
        return representationsToTry[cc];
      }
    }
  }

  // check if the data type is a vtkTable with a single row and column with
  // a vtkStringArray named "Text". If it is, we render this in a render view
  // with the value shown in the view.
  if (vtkSMOutputPort* port = producer->GetOutputPort(outputPort))
  {
    if (vtkPVDataInformation* dataInformation = port->GetDataInformation())
    {
      if (dataInformation->GetDataSetType() == VTK_TABLE)
      {
        if (vtkPVArrayInformation* ai =
              dataInformation->GetArrayInformation("Text", vtkDataObject::ROW))
        {
          if (ai->GetNumberOfComponents() == 1 && ai->GetNumberOfTuples() == 1)
          {
            return "TextSourceRepresentation";
          }
        }
      }
    }
  }

  if (vtkPVXMLElement* hints = producer->GetHints())
  {
    // If the source has an hint as follows, then it's a text producer and must
    // be display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>
    for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
    {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      const char* childName = child->GetName();
      if (childName && strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) && index == outputPort &&
        child->GetAttribute("type"))
      {
        if (strcmp(child->GetAttribute("type"), "text") == 0)
        {
          return "TextSourceRepresentation";
        }
        else if (strcmp(child->GetAttribute("type"), "progress") == 0)
        {
          return "ProgressBarSourceRepresentation";
        }
      }
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ZoomTo(vtkSMProxy* representation)
{
  vtkSMPropertyHelper helper(representation, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy());
  int port = helper.GetOutputPort();
  if (!input)
  {
    return;
  }

  vtkPVDataInformation* info = input->GetDataInformation(port);
  double bounds[6];
  info->GetBounds(bounds);
  if (!vtkMath::AreBoundsInitialized(bounds))
  {
    return;
  }

  if (representation->GetProperty("Position") && representation->GetProperty("Orientation") &&
    representation->GetProperty("Scale"))
  {
    double position[3], rotation[3], scale[3];
    vtkSMPropertyHelper(representation, "Position").Get(position, 3);
    vtkSMPropertyHelper(representation, "Orientation").Get(rotation, 3);
    vtkSMPropertyHelper(representation, "Scale").Get(scale, 3);

    if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 || position[0] != 0.0 ||
      position[1] != 0.0 || position[2] != 0.0 || rotation[0] != 0.0 || rotation[1] != 0.0 ||
      rotation[2] != 0.0)
    {
      vtkTransform* transform = vtkTransform::New();
      transform->Translate(position);
      transform->RotateZ(rotation[2]);
      transform->RotateX(rotation[0]);
      transform->RotateY(rotation[1]);
      transform->Scale(scale);

      int i, j, k;
      double origX[3], x[3];
      vtkBoundingBox bbox;
      for (i = 0; i < 2; i++)
      {
        origX[0] = bounds[i];
        for (j = 0; j < 2; j++)
        {
          origX[1] = bounds[2 + j];
          for (k = 0; k < 2; k++)
          {
            origX[2] = bounds[4 + k];
            transform->TransformPoint(origX, x);
            bbox.AddPoint(x);
          }
        }
      }
      bbox.GetBounds(bounds);
      transform->Delete();
    }
  }
  this->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetCamera").arg("comment", "reset view to fit data");

  this->GetSession()->PrepareProgress();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ResetCamera"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  double bds[6] = { xmin, xmax, ymin, ymax, zmin, zmax };
  this->ResetCamera(bds);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(double bounds[6])
{
  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("ResetCamera")
    .arg(bounds[0])
    .arg(bounds[1])
    .arg(bounds[2])
    .arg(bounds[3])
    .arg(bounds[4])
    .arg(bounds[5])
    .arg("comment", "reset view to fit data bounds");
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ResetCamera"
         << vtkClientServerStream::InsertArray(bounds, 6) << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");

  // If modified proxy is the camera, we must clear the cache even if we're
  // currently in selection mode.
  this->ClearSelectionCache(/*force=*/modifiedProxy == cameraProxy);

  // skip modified properties on camera subproxy.
  if (modifiedProxy != cameraProxy)
  {
    this->Superclass::MarkDirty(modifiedProxy);
  }
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::Pick(int x, int y)
{
  // 1) Create surface selection.
  //   Will returns a surface selection in terms of cells selected on the
  //   visible props from all representations.
  vtkSMRepresentationProxy* repr = NULL;
  vtkCollection* reprs = vtkCollection::New();
  vtkCollection* sources = vtkCollection::New();
  int region[4] = { x, y, x, y };
  if (this->SelectSurfaceCells(region, reprs, sources, false))
  {
    if (reprs->GetNumberOfItems() > 0)
    {
      repr = vtkSMRepresentationProxy::SafeDownCast(reprs->GetItemAsObject(0));
    }
  }
  reprs->Delete();
  sources->Delete();
  return repr;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::PickBlock(int x, int y, unsigned int& flatIndex)
{
  vtkSMRepresentationProxy* repr = NULL;
  vtkSmartPointer<vtkCollection> reprs = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> sources = vtkSmartPointer<vtkCollection>::New();
  int region[4] = { x, y, x, y };
  if (this->SelectSurfaceCells(region, reprs.GetPointer(), sources.GetPointer(), false))
  {
    if (reprs->GetNumberOfItems() > 0)
    {
      repr = vtkSMRepresentationProxy::SafeDownCast(reprs->GetItemAsObject(0));
    }
  }

  if (!repr)
  {
    // nothing selected
    return 0;
  }

  // get data information
  vtkPVDataInformation* info = repr->GetRepresentedDataInformation();
  vtkPVCompositeDataInformation* compositeInfo = info->GetCompositeDataInformation();

  // get selection in order to determine which block of the data set
  // set was selected (if it is a composite data set)
  if (compositeInfo && compositeInfo->GetDataIsComposite())
  {
    vtkSMProxy* selectionSource = vtkSMProxy::SafeDownCast(sources->GetItemAsObject(0));

    vtkSMPropertyHelper inputHelper(repr, "Input");
    // get representation input data
    vtkSMSourceProxy* reprInput = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());

    // convert cell selection to block selection
    vtkSMProxy* blockSelection = vtkSMSelectionHelper::ConvertSelection(
      vtkSelectionNode::BLOCKS, selectionSource, reprInput, inputHelper.GetOutputPort());

    // set block index
    flatIndex = vtkSMPropertyHelper(blockSelection, "Blocks").GetAsInt();

    blockSelection->Delete();
  }

  // return selected representation
  return repr;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::ConvertDisplayToPointOnSurface(
  const int display_position[2], double world_position[3], bool snapOnMeshPoint)
{
  int region[4] = { display_position[0], display_position[1], display_position[0],
    display_position[1] };

  vtkSMSessionProxyManager* spxm = this->GetSessionProxyManager();
  vtkNew<vtkCollection> representations;
  vtkNew<vtkCollection> sources;

  if (snapOnMeshPoint)
  {
    this->SelectSurfacePoints(region, representations.GetPointer(), sources.GetPointer(), false);
  }
  else
  {
    this->SelectSurfaceCells(region, representations.GetPointer(), sources.GetPointer(), false);
  }

  if (representations->GetNumberOfItems() > 0 && sources->GetNumberOfItems() > 0)
  {
    vtkSMPVRepresentationProxy* rep =
      vtkSMPVRepresentationProxy::SafeDownCast(representations->GetItemAsObject(0));
    vtkSMProxy* input = vtkSMPropertyHelper(rep, "Input").GetAsProxy(0);
    vtkSMSourceProxy* selection = vtkSMSourceProxy::SafeDownCast(sources->GetItemAsObject(0));

    // Picking info
    // {r0, r1, 1} => We want to make sure the ray that start from the camera reach
    // the end of the scene so it could cross any cell of the scene
    double nearDisplayPoint[3] = { (double)region[0], (double)region[1], 0.0 };
    double farDisplayPoint[3] = { (double)region[0], (double)region[1], 1.0 };
    double farLinePoint[3];
    double nearLinePoint[3];

    vtkRenderer* renderer = this->GetRenderer();

    // compute near line point
    renderer->SetDisplayPoint(nearDisplayPoint);
    renderer->DisplayToWorld();
    const double* world = renderer->GetWorldPoint();
    for (int i = 0; i < 3; i++)
    {
      nearLinePoint[i] = world[i] / world[3];
    }

    // compute far line point
    renderer->SetDisplayPoint(farDisplayPoint);
    renderer->DisplayToWorld();
    world = renderer->GetWorldPoint();
    for (int i = 0; i < 3; i++)
    {
      farLinePoint[i] = world[i] / world[3];
    }

    // Compute the  intersection...
    vtkSMProxy* pickingHelper = spxm->NewProxy("misc", "PickingHelper");
    vtkSMPropertyHelper(pickingHelper, "Input").Set(input);
    vtkSMPropertyHelper(pickingHelper, "Selection").Set(selection);
    vtkSMPropertyHelper(pickingHelper, "PointA").Set(nearLinePoint, 3);
    vtkSMPropertyHelper(pickingHelper, "PointB").Set(farLinePoint, 3);
    vtkSMPropertyHelper(pickingHelper, "SnapOnMeshPoint").Set(snapOnMeshPoint);
    pickingHelper->UpdateVTKObjects();
    pickingHelper->UpdateProperty("Update", 1);
    vtkSMPropertyHelper(pickingHelper, "Intersection").UpdateValueFromServer();
    vtkSMPropertyHelper(pickingHelper, "Intersection").Get(world_position, 3);
    pickingHelper->Delete();
  }
  else
  {
    // Need to warn user when used in RenderServer mode
    if (!this->IsSelectionAvailable())
    {
      vtkWarningMacro("Snapping to the surface is not available therefore "
                      "the camera focal point will be used to determine "
                      "the depth of the picking.");
    }

    // Use camera focal point to get some Zbuffer
    double cameraFP[4];
    vtkRenderer* renderer = this->GetRenderer();
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->GetFocalPoint(cameraFP);
    cameraFP[3] = 1.0;
    renderer->SetWorldPoint(cameraFP);
    renderer->WorldToDisplay();
    double* displayCoord = renderer->GetDisplayPoint();

    // Handle display to world conversion
    double display[3] = { (double)region[0], (double)region[1], displayCoord[2] };
    renderer->SetDisplayPoint(display);
    renderer->DisplayToWorld();
    const double* world = renderer->GetWorldPoint();
    for (int i = 0; i < 3; i++)
    {
      world_position[i] = world[i] / world[3];
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectInternal(const vtkClientServerStream& csstream,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
  {
    return false;
  }

  vtkScopedMonitorProgress monitorProgress(this);

  this->IsSelectionCached = true;

  // Call PreRender since Select making will cause multiple renders on the
  // render window. Calling PreRender ensures that the view is ready to render.
  vtkTypeUInt32 render_location = this->PreRender(/*interactive=*/false);
  this->ExecuteStream(csstream, false, render_location);
  bool retVal =
    this->FetchLastSelection(multiple_selections, selectedRepresentations, selectionSources);
  this->PostRender(false);
  return retVal;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfaceCells(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SelectCells" << region[0]
         << region[1] << region[2] << region[3] << vtkClientServerStream::End;
  return this->SelectInternal(
    stream, selectedRepresentations, selectionSources, multiple_selections);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfacePoints(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SelectPoints" << region[0]
         << region[1] << region[2] << region[3] << vtkClientServerStream::End;
  return this->SelectInternal(
    stream, selectedRepresentations, selectionSources, multiple_selections);
}

namespace
{
//-----------------------------------------------------------------------------
static void vtkShrinkSelection(vtkSelection* sel)
{
  std::map<void*, int> pixelCounts;
  unsigned int numNodes = sel->GetNumberOfNodes();
  void* chosen = NULL;
  int maxPixels = -1;
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    vtkInformation* properties = node->GetProperties();
    if (properties->Has(vtkSelectionNode::PIXEL_COUNT()) &&
      properties->Has(vtkSelectionNode::SOURCE()))
    {
      int numPixels = properties->Get(vtkSelectionNode::PIXEL_COUNT());
      void* source = properties->Get(vtkSelectionNode::SOURCE());
      pixelCounts[source] += numPixels;
      if (pixelCounts[source] > maxPixels)
      {
        maxPixels = numPixels;
        chosen = source;
      }
    }
  }

  std::vector<vtkSmartPointer<vtkSelectionNode> > chosenNodes;
  if (chosen != NULL)
  {
    for (unsigned int cc = 0; cc < numNodes; cc++)
    {
      vtkSelectionNode* node = sel->GetNode(cc);
      vtkInformation* properties = node->GetProperties();
      if (properties->Has(vtkSelectionNode::SOURCE()) &&
        properties->Get(vtkSelectionNode::SOURCE()) == chosen)
      {
        chosenNodes.push_back(node);
      }
    }
  }
  sel->RemoveAllNodes();
  for (unsigned int cc = 0; cc < chosenNodes.size(); cc++)
  {
    sel->AddNode(chosenNodes[cc]);
  }
}
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::FetchLastSelection(
  bool multiple_selections, vtkCollection* selectedRepresentations, vtkCollection* selectionSources)
{
  if (selectionSources && selectedRepresentations)
  {
    vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
    vtkSelection* selection = rv->GetLastSelection();
    if (!multiple_selections)
    {
      // only pass through selection over a single representation.
      vtkShrinkSelection(selection);
    }
    vtkSMSelectionHelper::NewSelectionSourcesFromSelection(
      selection, this, selectionSources, selectedRepresentations);
    return (selectionSources->GetNumberOfItems() > 0);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::ComputeVisibleScalarRange(
  int fieldAssociation, const char* scalarName, int component, double range[])
{
  const int* size = this->GetRenderer()->GetSize();
  int region[4] = { 0, 0, size[0] - 1, size[1] - 1 };
  return this->ComputeVisibleScalarRange(region, fieldAssociation, scalarName, component, range);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::ComputeVisibleScalarRange(
  const int region[4], int fieldAssociation, const char* scalarName, int component, double range[])
{
  if (!this->IsSelectionAvailable())
  {
    vtkErrorMacro("Cannot ComputeVisibleScalarRange since surface selection is currently "
                  "unsupported.");
    return false;
  }

  vtkScopedMonitorProgress monitorProgress(this);

  bool multiple_selections = true;

  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;

  vtkNew<vtkCollection> selectedRepresentations;
  vtkNew<vtkCollection> selectionSources;

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->SelectSurfacePoints(
      region, selectedRepresentations.Get(), selectionSources.Get(), multiple_selections);
  }
  else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->SelectSurfaceCells(
      region, selectedRepresentations.Get(), selectionSources.Get(), multiple_selections);
  }
  else
  {
    return false;
  }
  assert(selectedRepresentations->GetNumberOfItems() == selectionSources->GetNumberOfItems());

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> _rangeExtractor;
  _rangeExtractor.TakeReference(pxm->NewProxy("internal_filters", "ExtractSelectionRange"));
  vtkSMSourceProxy* rangeExtractor = vtkSMSourceProxy::SafeDownCast(_rangeExtractor);
  if (!rangeExtractor)
  {
    vtkErrorMacro("Failed to create 'ExtractSelectionRange' proxy. "
                  "ComputeVisibleScalarRange() cannot work as expected.");
    return false;
  }

  for (int cc = 0, max = selectionSources->GetNumberOfItems(); cc < max; cc++)
  {
    vtkSMProxy* selectedRepresentation =
      vtkSMProxy::SafeDownCast(selectedRepresentations->GetItemAsObject(cc));
    vtkSMProxy* selectionSource = vtkSMProxy::SafeDownCast(selectionSources->GetItemAsObject(cc));

    vtkSMPropertyHelper(rangeExtractor, "Input")
      .Set(vtkSMPropertyHelper(selectedRepresentation, "Input").GetAsProxy(),
        vtkSMPropertyHelper(selectedRepresentation, "Input").GetOutputPort());
    vtkSMPropertyHelper(rangeExtractor, "Selection").Set(selectionSource);
    vtkSMPropertyHelper(rangeExtractor, "ArrayName").Set(scalarName);
    vtkSMPropertyHelper(rangeExtractor, "Component").Set(component);
    vtkSMPropertyHelper(rangeExtractor, "FieldType").Set(fieldAssociation);
    rangeExtractor->UpdateVTKObjects();

    rangeExtractor->UpdatePipeline();
    rangeExtractor->UpdatePropertyInformation();

    double tempRange[2];
    vtkSMPropertyHelper(rangeExtractor, "Range").Get(tempRange, 2);
    if (tempRange[0] < range[0])
    {
      range[0] = tempRange[0];
    }
    if (tempRange[1] > range[1])
    {
      range[1] = tempRange[1];
    }
  }

  return (range[1] >= range[0]);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumCells(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  return this->SelectFrustumInternal(
    region, selectedRepresentations, selectionSources, multiple_selections, vtkSelectionNode::CELL);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumPoints(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  return this->SelectFrustumInternal(region, selectedRepresentations, selectionSources,
    multiple_selections, vtkSelectionNode::POINT);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumInternal(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int fieldAssociation)
{
  vtkScopedMonitorProgress monitorProgress(this);

  // Simply stealing old code for now. This code have many coding style
  // violations and seems too long for what it does. At some point we'll check
  // it out.

  int displayRectangle[4] = { region[0], region[1], region[2], region[3] };
  if (displayRectangle[0] == displayRectangle[2])
  {
    displayRectangle[2] += 1;
  }
  if (displayRectangle[1] == displayRectangle[3])
  {
    displayRectangle[3] += 1;
  }

  // 1) Create frustum selection
  // convert screen rectangle to world frustum
  vtkRenderer* renderer = this->GetRenderer();
  double frustum[32];
  int index = 0;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index * 4]);

  vtkSMProxy* selectionSource =
    this->GetSessionProxyManager()->NewProxy("sources", "FrustumSelectionSource");
  vtkSMPropertyHelper(selectionSource, "FieldType").Set(fieldAssociation);
  vtkSMPropertyHelper(selectionSource, "Frustum").Set(frustum, 32);
  selectionSource->UpdateVTKObjects();

  // 2) Figure out which representation is "selected".
  vtkExtractSelectedFrustum* extractor = vtkExtractSelectedFrustum::New();
  extractor->CreateFrustum(frustum);

  // Now we just use the first selected representation,
  // until we have other mechanisms to select one.
  vtkSMPropertyHelper reprsHelper(this, "Representations");

  for (unsigned int cc = 0; cc < reprsHelper.GetNumberOfElements(); cc++)
  {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(reprsHelper.GetAsProxy(cc));
    if (!repr || vtkSMPropertyHelper(repr, "Visibility", true).GetAsInt() == 0)
    {
      continue;
    }
    if (vtkSMPropertyHelper(repr, "Pickable", true).GetAsInt() == 0)
    {
      // skip non-pickable representations.
      continue;
    }
    vtkPVDataInformation* datainfo = repr->GetRepresentedDataInformation();
    if (!datainfo)
    {
      continue;
    }

    double bounds[6];
    datainfo->GetBounds(bounds);

    if (extractor->OverallBoundsTest(bounds))
    {
      selectionSources->AddItem(selectionSource);
      selectedRepresentations->AddItem(repr);
      if (!multiple_selections)
      {
        break;
      }
    }
  }

  extractor->Delete();
  selectionSource->Delete();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonPoints(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  return this->SelectPolygonInternal(polygonPts, selectedRepresentations, selectionSources,
    multiple_selections, "SelectPolygonPoints");
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonCells(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections)
{
  return this->SelectPolygonInternal(polygonPts, selectedRepresentations, selectionSources,
    multiple_selections, "SelectPolygonCells");
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonInternal(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  const char* method)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << method
         << vtkClientServerStream::InsertArray(polygonPts->GetPointer(0),
              polygonPts->GetNumberOfTuples() * polygonPts->GetNumberOfComponents())
         << polygonPts->GetNumberOfTuples() * polygonPts->GetNumberOfComponents()
         << vtkClientServerStream::End;
  return this->SelectInternal(
    stream, selectedRepresentations, selectionSources, multiple_selections);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::RenderForImageCapture()
{
  if (vtkSMPropertyHelper(this, "UseInteractiveRenderingForScreenshots", /*quiet=*/true).GetAsInt())
  {
    this->InteractiveRender();
  }
  else
  {
    this->StillRender();
  }
}

//----------------------------------------------------------------------------
vtkFloatArray* vtkSMRenderViewProxy::CaptureDepthBuffer()
{
  this->InvokeCommand("CaptureZBuffer");
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  vtkFloatArray* capture = view->GetCapturedZBuffer();
  return capture;
}

//----------------------------------------------------------------------------
vtkFloatArray* vtkSMRenderViewProxy::GetValuesFloat()
{
  this->InvokeCommand("CaptureValuesFloat");
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  vtkFloatArray* capture = view->GetCapturedValuesFloat();
  return capture;
}

//------------------------------------------------------------------------------
void vtkSMRenderViewProxy::StartCaptureValues()
{
  this->InvokeCommand("BeginValueCapture");
}

//------------------------------------------------------------------------------
void vtkSMRenderViewProxy::StopCaptureValues()
{
  this->InvokeCommand("EndValueCapture");
}

//------------------------------------------------------------------------------
int vtkSMRenderViewProxy::GetValueRenderingMode()
{
  vtkSMPropertyHelper helper(this, "ValueRenderingModeGet");
  helper.UpdateValueFromServer();
  return helper.GetAsInt();
}

//------------------------------------------------------------------------------
void vtkSMRenderViewProxy::SetValueRenderingMode(int mode)
{
  vtkSMPropertyHelper(this, "ValueRenderingMode").Set(mode);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::NewMasterCallback(vtkObject*, unsigned long, void*)
{
  if (this->Session && this->Session->IsMultiClients() &&
    this->Session->GetCollaborationManager()->IsMaster())
  {
    // Make sure we clear the selection cache server side as well as previous
    // master might already have set a selection that has been cached.
    this->ClearSelectionCache(/*force=*/true);
  }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ClearSelectionCache(bool force /*=false*/)
{
  // We check if we're currently selecting. If that's the case, any non-forced
  // modifications (i.e. those coming through because of proxy-modifications)
  // are considered a part of the making/showing selection and hence we
  // don't clear the selection cache. While this doesn't help us preserve the
  // cache between separate surface selection invocations, it does help us with
  // reusing the case when in interactive selection mode.
  if ((this->IsSelectionCached && !this->IsInSelectionMode()) || force)
  {
    this->IsSelectionCached = false;
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "InvalidateCachedSelection"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
  }
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::IsInSelectionMode()
{
  switch (vtkSMPropertyHelper(this, "InteractionMode", /*quiet*/ true).GetAsInt())
  {
    case vtkPVRenderView::INTERACTION_MODE_SELECTION:
    case vtkPVRenderView::INTERACTION_MODE_POLYGON:
      return true;

    default:
      return false;
  }
}
