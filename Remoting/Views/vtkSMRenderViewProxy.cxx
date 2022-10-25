/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.cxx

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
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVEncodeSelectionForServer.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPointData.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMDataDeliveryManagerProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMViewProxyInteractorHelper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

#include <cassert>
#include <cmath>

namespace
{
// magic number used as elevation to achieve an isometric view direction.
const double isometric_elev = vtkMath::DegreesFromRadians(std::asin(std::tan(vtkMath::Pi() / 6.0)));

void RotateElevation(vtkCamera* camera, double angle)
{
  vtkNew<vtkTransform> transform;

  double scale = vtkMath::Norm(camera->GetPosition());
  if (scale <= 0.0)
  {
    scale = vtkMath::Norm(camera->GetFocalPoint());
    if (scale <= 0.0)
    {
      scale = 1.0;
    }
  }
  double* temp = camera->GetFocalPoint();
  camera->SetFocalPoint(temp[0] / scale, temp[1] / scale, temp[2] / scale);
  temp = camera->GetPosition();
  camera->SetPosition(temp[0] / scale, temp[1] / scale, temp[2] / scale);

  double v2[3];
  // translate to center
  // we rotate around 0,0,0 rather than the center of rotation
  transform->Identity();

  // elevation
  camera->OrthogonalizeViewUp();
  double* viewUp = camera->GetViewUp();
  vtkMath::Cross(camera->GetDirectionOfProjection(), viewUp, v2);
  transform->RotateWXYZ(-angle, v2[0], v2[1], v2[2]);

  // translate back
  // we are already at 0,0,0
  camera->ApplyTransform(transform.GetPointer());
  camera->OrthogonalizeViewUp();

  // For rescale back.
  temp = camera->GetFocalPoint();
  camera->SetFocalPoint(temp[0] * scale, temp[1] * scale, temp[2] * scale);
  temp = camera->GetPosition();
  camera->SetPosition(temp[0] * scale, temp[1] * scale, temp[2] * scale);
}
}

vtkStandardNewMacro(vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
{
  this->IsSelectionCached = false;
  this->NewMasterObserverId = 0;
  this->NeedsUpdateLOD = true;
  this->InteractorHelper->SetViewProxy(this);
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy()
{
  this->InteractorHelper->SetViewProxy(nullptr);
  this->InteractorHelper->CleanupInteractor();

  if (this->NewMasterObserverId != 0 && this->Session && this->Session->GetCollaborationManager())
  {
    this->Session->GetCollaborationManager()->RemoveObserver(this->NewMasterObserverId);
    this->NewMasterObserverId = 0;
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
  if (server_info && server_info->GetIsInCave())
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

  return nullptr;
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
  bool something_delivered = this->GetDeliveryManager()->DeliverStreamedPieces();
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
  assert(rv != nullptr);
  if (interactive && rv->GetUseLODForInteractiveRender())
  {
    // for interactive renders, we need to determine if we are going to use LOD.
    // If so, we may need to update the LOD geometries.
    this->UpdateLOD();
  }

  return interactive ? rv->GetInteractiveRenderProcesses() : rv->GetStillRenderProcesses();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PostRender(bool interactive)
{
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  this->SynchronizeCameraProperties();
  this->Superclass::PostRender(interactive);
  vtkSMTrace* tracer = nullptr;
  if (!interactive && (tracer = vtkSMTrace::GetActiveTracer()) &&
    tracer->GetFullyTraceCameraAdjustments())
  {
    SM_SCOPED_TRACE(SaveCameras).arg("proxy", this).arg("comment", "Adjust camera");
  }
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
  return rv ? rv->GetRenderer() : nullptr;
}

//----------------------------------------------------------------------------
vtkCamera* vtkSMRenderViewProxy::GetActiveCamera()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetActiveCamera() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AdjustActiveCamera(const int& adjustType, const double& angle)
{
  if (adjustType >= 0 && adjustType < 4)
  {
    this->AdjustActiveCamera(
      static_cast<vtkSMRenderViewProxy::CameraAdjustmentType>(adjustType), angle);
  }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AdjustActiveCamera(
  const CameraAdjustmentType& adjustType, const double& angle)
{
  switch (adjustType)
  {
    case CameraAdjustmentType::Azimuth:
      this->AdjustAzimuth(angle);
      break;
    case CameraAdjustmentType::Roll:
      this->AdjustRoll(angle);
      break;
    case CameraAdjustmentType::Elevation:
      this->AdjustElevation(angle);
      break;
    case CameraAdjustmentType::Zoom:
      this->AdjustZoom(angle);
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AdjustAzimuth(const double& value)
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("AdjustAzimuth").arg(value);
  vtkCamera* camera = this->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  camera->Azimuth(value);
  this->SynchronizeCameraProperties();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AdjustElevation(const double& value)
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("AdjustElevation").arg(value);
  vtkCamera* camera = this->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  // camera->Elevation(angle); // sometimes, this can cause an invalid view-up vector
  RotateElevation(camera, value);
  this->SynchronizeCameraProperties();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AdjustRoll(const double& value)
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("AdjustRoll").arg(value);
  vtkCamera* camera = this->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  camera->Roll(value);
  this->SynchronizeCameraProperties();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::AdjustZoom(const double& value)
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("AdjustZoom").arg(value);
  vtkCamera* camera = this->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  if (camera->GetParallelProjection())
  {
    camera->SetParallelScale(camera->GetParallelScale() / value);
  }
  else
  {
    camera->Dolly(value);
  }
  this->SynchronizeCameraProperties();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ApplyIsometricView()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ApplyIsometricView");
  vtkCamera* cam = this->GetActiveCamera();
  // Ref: Fig 2.4 - Brian Griffith: "Engineering Drawing for Manufacture", DOI
  // https://doi.org/10.1016/B978-185718033-6/50016-1
  this->ResetActiveCameraToDirection(0, 0, -1, 0, 1, 0);
  cam->Azimuth(45.);
  RotateElevation(cam, isometric_elev);
  this->SynchronizeCameraProperties();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToDirection(const double& look_x, const double& look_y,
  const double& look_z, const double& up_x, const double& up_y, const double& up_z)
{
  if (vtkCamera* cam = this->GetActiveCamera())
  {
    cam->SetPosition(0, 0, 0);
    cam->SetFocalPoint(look_x, look_y, look_z);
    cam->SetViewUp(up_x, up_y, up_z);
    this->SynchronizeCameraProperties();
  }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToPositiveX()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetActiveCameraToPositiveX");
  this->ResetActiveCameraToDirection(1, 0, 0, 0, 0, 1);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToNegativeX()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetActiveCameraToNegativeX");
  this->ResetActiveCameraToDirection(-1, 0, 0, 0, 0, 1);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToPositiveY()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetActiveCameraToPositiveY");
  this->ResetActiveCameraToDirection(0, 1, 0, 0, 0, 1);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToNegativeY()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetActiveCameraToNegativeY");
  this->ResetActiveCameraToDirection(0, -1, 0, 0, 0, 1);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToPositiveZ()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetActiveCameraToPositiveZ");
  this->ResetActiveCameraToDirection(0, 0, 1, 0, 1, 0);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetActiveCameraToNegativeZ()
{
  SM_SCOPED_TRACE(CallMethod).arg(this).arg("ResetActiveCameraToNegativeZ");
  this->ResetActiveCameraToDirection(0, 0, -1, 0, 1, 0);
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
  return rv ? rv->GetInteractor() : nullptr;
}

//----------------------------------------------------------------------------
vtkSMViewProxyInteractorHelper* vtkSMRenderViewProxy::GetInteractorHelper()
{
  return this->InteractorHelper;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMRenderViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  return rv ? rv->GetRenderWindow() : nullptr;
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
  auto config = vtkRemotingCoreConfiguration::GetInstance();
  if (config->GetUseStereoRendering())
  {
    vtkSMPropertyHelper(this, "StereoCapableWindow").Set(1);
    vtkSMPropertyHelper(this, "StereoRender").Set(1);
    vtkSMPropertyHelper(this, "StereoType").Set(config->GetStereoType());
  }

  bool remote_rendering_available = true;
  // Update whether render servers can open display i.e. remote rendering is
  // possible on all processes.
  vtkNew<vtkPVRenderingCapabilitiesInformation> info;
  this->GetSession()->GatherInformation(vtkPVSession::RENDER_SERVER, info.Get(), 0);
  if (!info->Supports(vtkPVRenderingCapabilitiesInformation::OPENGL))
  {
    remote_rendering_available = false;
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
}

//----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer);

  if (const char* reprName = this->Superclass::GetRepresentationType(producer, outputPort))
  {
    return reprName;
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
      const char* childType = child->GetAttribute("type");
      if (childName && strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) && index == outputPort && childType)
      {
        if (strcmp(childType, "text") == 0)
        {
          return "TextSourceRepresentation";
        }
        else if (strcmp(childType, "progress") == 0)
        {
          return "ProgressBarSourceRepresentation";
        }
        else if (strcmp(childType, "logo") == 0)
        {
          return "LogoSourceRepresentation";
        }
      }
    }
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  const char* representationsToTry[] = { "UnstructuredGridRepresentation",
    "StructuredGridRepresentation", "HyperTreeGridRepresentation", "AMRRepresentation",
    "UniformGridRepresentation", "PVMoleculeRepresentation", "GeometryRepresentation", nullptr };
  for (int cc = 0; representationsToTry[cc] != nullptr; ++cc)
  {
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", representationsToTry[cc]))
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

  {
    vtkPVDataInformation* dataInformation = nullptr;
    if (vtkSMOutputPort* port = producer->GetOutputPort(outputPort))
    {
      dataInformation = port->GetDataInformation();
    }

    // check if the data type is a vtkTable with a single row and column with
    // a vtkStringArray named "Text". If it is, we render this in a render view
    // with the value shown in the view.
    if (dataInformation)
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

  // Default to "GeometryRepresentation" for composite datasets where we
  // might not yet know the dataset type of the children at the time the
  // representation is created.
  if (vtkSMOutputPort* port = producer->GetOutputPort(outputPort))
  {
    if (vtkPVDataInformation* dataInformation = port->GetDataInformation())
    {
      if (dataInformation->IsCompositeDataSet())
      {
        return "GeometryRepresentation";
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ZoomTo(vtkSMProxy* representation, bool closest)
{
  vtkSMPropertyHelper helper(representation, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy());
  if (!input)
  {
    return;
  }

  // Send client server stream to the vtkPVRenderView to reduce visible bounds
  this->GetSession()->PrepareProgress();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ComputeVisibleBounds"
         << VTKOBJECT(representation) << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  vtkClientServerStream result = this->GetLastResult();

  double bounds[6];
  if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1)
  {
    result.GetArgument(0, 0, bounds, 6);
  }

  if (bounds[1] >= bounds[0] && bounds[3] >= bounds[2] && bounds[5] >= bounds[4])
  {
    this->ResetCamera(bounds, closest);
  }
  this->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(bool closest)
{
  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("ResetCamera")
    .arg(closest)
    .arg("comment", "reset view to fit data");

  this->GetSession()->PrepareProgress();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this);
  if (closest)
  {
    stream << "ResetCameraScreenSpace";
  }
  else
  {
    stream << "ResetCamera";
  }
  stream << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax, bool closest)
{
  double bds[6] = { xmin, xmax, ymin, ymax, zmin, zmax };
  this->ResetCamera(bds, closest);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(double bounds[6], bool closest)
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
    .arg(closest)
    .arg("comment", "reset view to fit data bounds");
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this);
  if (closest)
  {
    stream << "ResetCameraScreenSpace";
  }
  else
  {
    stream << "ResetCamera";
  }
  stream << vtkClientServerStream::InsertArray(bounds, 6) << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");

  // if modified proxy is the camera, we must clear the cache even if we're
  // currently in selection mode.
  bool forceClearCache = (modifiedProxy == cameraProxy);

  // if modified proxy is a source-proxy not part of the selection sub-pipeline,
  // we have to force clear selection buffers (see #20560).
  if (!forceClearCache && (vtkSMSourceProxy::SafeDownCast(modifiedProxy) != nullptr))
  {
    const bool isPVExtractSelectionFilter = strcmp(modifiedProxy->GetXMLGroup(), "filters") == 0 &&
      strcmp(modifiedProxy->GetXMLName(), "PVExtractSelection") == 0;
    const bool isSelectionRepresentation =
      strcmp(modifiedProxy->GetXMLGroup(), "representations") == 0 &&
      strcmp(modifiedProxy->GetXMLName(), "SelectionRepresentation") == 0;

    const bool isFastPreSelection = strcmp(modifiedProxy->GetXMLGroup(), "representations") == 0 &&
      vtkPVRenderViewSettings::GetInstance()->GetEnableFastPreselection();

    forceClearCache =
      !(isPVExtractSelectionFilter || isSelectionRepresentation || isFastPreSelection);
  }

  const bool cacheCleared = this->ClearSelectionCache(forceClearCache);

  // log for debugging purposes.
  vtkLogIfF(TRACE, cacheCleared && forceClearCache, "%s: force-cleared selection cache due to %s",
    this->GetLogNameOrDefault(), modifiedProxy ? modifiedProxy->GetLogNameOrDefault() : nullptr);

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
  vtkSMRepresentationProxy* repr = nullptr;
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
vtkSMRepresentationProxy* vtkSMRenderViewProxy::PickBlock(
  int x, int y, unsigned int& flatIndex, int& rank)
{
  flatIndex = 0;
  rank = 0;

  vtkSMRepresentationProxy* repr = nullptr;
  vtkNew<vtkCollection> reprs;
  vtkNew<vtkCollection> sources;
  int region[4] = { x, y, x, y };
  if (this->SelectSurfaceCells(region, reprs, sources, false))
  {
    if (reprs->GetNumberOfItems() > 0)
    {
      repr = vtkSMRepresentationProxy::SafeDownCast(reprs->GetItemAsObject(0));
    }
  }

  if (!repr)
  {
    // nothing selected
    return nullptr;
  }

  // get data information
  auto input = vtkSMPropertyHelper(repr, "Input", /*quiet*/ true).GetAsOutputPort();
  auto info = input ? input->GetDataInformation() : nullptr;

  // get selection in order to determine which block of the dataset
  // was selected (if it is a composite data set)
  if (info && info->IsCompositeDataSet())
  {
    // selection Source is NOT an appendSelections filter, so we can continue as usual
    auto selectionSource = vtkSMProxy::SafeDownCast(sources->GetItemAsObject(0));

    // since SelectSurfaceCells can ever only return a
    // `CompositeDataIDSelectionSource` or `HierarchicalDataIDSelectionSource`
    // for composite datasets, we use the following trick to extract the
    // block/rank information quickly.
    if (strcmp(selectionSource->GetXMLName(), "CompositeDataIDSelectionSource") == 0)
    {
      vtkSMPropertyHelper ids(selectionSource, "IDs");
      if (ids.GetNumberOfElements() >= 3) // we expect 3-tuples
      {
        flatIndex = ids.GetAsInt(0);
        rank = ids.GetAsInt(1);
      }
    }
    else if (strcmp(selectionSource->GetXMLName(), "HierarchicalDataIDSelectionSource") == 0)
    {
      vtkSMPropertyHelper ids(selectionSource, "IDs");
      if (ids.GetNumberOfElements() >= 3) // we expect 3-tuples
      {
        unsigned int level = ids.GetAsInt(0);
        unsigned int index = ids.GetAsInt(1);

        // convert level,index to flat index.
        flatIndex = info->ComputeCompositeIndexForAMR(level, index);
        rank = 0; // doesn't matter since flatIndex is consistent on all ranks for AMR.
      }
    }
    else
    {
      vtkErrorMacro("Unexpected selection source: " << selectionSource->GetXMLName());
      return nullptr;
    }
  }

  // return selected representation
  return repr;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::ConvertDisplayToPointOnSurface(const int display_position[2],
  double world_position[3], double world_normal[3], bool snapOnMeshPoint)
{
  int region[4] = { display_position[0], display_position[1], display_position[0],
    display_position[1] };

  vtkSMSessionProxyManager* spxm = this->GetSessionProxyManager();
  vtkNew<vtkCollection> representations;
  vtkNew<vtkCollection> sources;

  if (snapOnMeshPoint)
  {
    this->SelectSurfacePoints(region, representations, sources, false);
  }
  else
  {
    this->SelectSurfaceCells(region, representations, sources, false);
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
    vtkSMPropertyHelper(pickingHelper, "IntersectionNormal").UpdateValueFromServer();
    vtkSMPropertyHelper(pickingHelper, "IntersectionNormal").Get(world_normal, 3);
    pickingHelper->Delete();

    static constexpr double PI_2 = vtkMath::Pi() / 2.0f;
    // Note: Fix normal direction in case the orientation of the picked cell is wrong.
    // When you cast a ray to a 3d object from a specific view angle (camera normal), the angle
    // between the camera normal and the normal of the cell surface that the ray intersected,
    // can have an angle up to pi / 2. This is true because you can't see surface objects
    // with a greater angle than pi / 2, therefore, you can't pick them. In case an angle greater
    // than pi / 2 is computed, it must be a result of a wrong orientation of the picked cell.
    // To solve this issue, we reverse the picked normal.
    double cameraNormal[3];
    this->GetRenderer()->GetActiveCamera()->GetViewPlaneNormal(cameraNormal);
    if (vtkMath::AngleBetweenVectors(world_normal, cameraNormal) > PI_2)
    {
      world_normal[0] *= -1;
      world_normal[1] *= -1;
      world_normal[2] *= -1;
    }
    return true;
  }
  else
  {
    // Need to warn user when used in RenderServer mode
    if (!this->IsSelectionAvailable())
    {
      vtkWarningMacro("Snapping to the surface is not available therefore "
                      "the camera focal point and normal will be used to determine "
                      "the depth of the picking.");
    }
    // Use camera focal point to get some Zbuffer
    double cameraFP[4];
    vtkRenderer* renderer = this->GetRenderer();
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->GetViewPlaneNormal(world_normal);
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
    return false;
  }
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectInternal(const vtkClientServerStream& csstream,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int modifier, bool selectBlocks)
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
  bool retVal = this->FetchLastSelection(
    multiple_selections, selectedRepresentations, selectionSources, modifier, selectBlocks);
  this->PostRender(false);
  return retVal;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfaceCells(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int modifier, bool select_blocks, const char* arrayName)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SelectCells" << region[0]
         << region[1] << region[2] << region[3] << arrayName << vtkClientServerStream::End;
  return this->SelectInternal(stream, selectedRepresentations, selectionSources,
    multiple_selections, modifier, select_blocks);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfacePoints(const int region[4],
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int modifier, bool select_blocks, const char* arrayName)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SelectPoints" << region[0]
         << region[1] << region[2] << region[3] << arrayName << vtkClientServerStream::End;
  return this->SelectInternal(stream, selectedRepresentations, selectionSources,
    multiple_selections, modifier, select_blocks);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::FetchLastSelection(bool multiple_selections,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, int modifier,
  bool selectBlocks)
{
  if (selectionSources && selectedRepresentations)
  {
    vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
    vtkSelection* rawSelection = rv->GetLastSelection();

    vtkNew<vtkPVEncodeSelectionForServer> helper;
    return helper->ProcessSelection(rawSelection, this, multiple_selections,
      selectedRepresentations, selectionSources, modifier, selectBlocks);
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
      region, selectedRepresentations, selectionSources, multiple_selections);
  }
  else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->SelectSurfaceCells(
      region, selectedRepresentations, selectionSources, multiple_selections);
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
  vtkNew<vtkExtractSelectedFrustum> extractor;
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

  selectionSource->Delete();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonPoints(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int modifier, bool selectBlocks)
{
  return this->SelectPolygonInternal(polygonPts, selectedRepresentations, selectionSources,
    multiple_selections, vtkSelectionNode::POINT, modifier, selectBlocks);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonCells(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int modifier, bool selectBlocks)
{
  return this->SelectPolygonInternal(polygonPts, selectedRepresentations, selectionSources,
    multiple_selections, vtkSelectionNode::CELL, modifier, selectBlocks);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonInternal(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, bool multiple_selections,
  int fieldAssociation, int modifier, bool selectBlocks)
{
  const char* method =
    fieldAssociation == vtkSelectionNode::POINT ? "SelectPolygonPoints" : "SelectPolygonCells";
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << method
         << vtkClientServerStream::InsertArray(polygonPts->GetPointer(0),
              polygonPts->GetNumberOfTuples() * polygonPts->GetNumberOfComponents())
         << polygonPts->GetNumberOfTuples() * polygonPts->GetNumberOfComponents()
         << vtkClientServerStream::End;
  return this->SelectInternal(
    stream, selectedRepresentations, selectionSources, multiple_selections, modifier, selectBlocks);
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
bool vtkSMRenderViewProxy::ClearSelectionCache(bool force /*=false*/)
{
  // We check if we're currently selecting. If that's the case, any non-forced
  // modifications (i.e. those coming through because of proxy-modifications)
  // are considered a part of the making/showing selection and hence we
  // don't clear the selection cache. While this doesn't help us preserve the
  // cache between separate surface selection invocations, it does help us with
  // reusing the case when in interactive selection mode.
  if (this->IsSelectionCached && (!this->IsInSelectionMode() || force))
  {
    this->IsSelectionCached = false;
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "InvalidateCachedSelection"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    return true;
  }
  return false;
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
