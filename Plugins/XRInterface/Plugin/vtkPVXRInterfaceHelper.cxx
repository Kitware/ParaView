// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVXRInterfaceHelper.h"

#if XRINTERFACE_HAS_OPENVR_SUPPORT
// must be before others due to glew include
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
#endif

#if XRINTERFACE_HAS_OPENXR_SUPPORT
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#endif

#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
#include "vtkOpenXRManagerRemoteConnection.h"
#include "vtkOpenXRRemotingRenderWindow.h"
#include "vtkWin32OpenGLDXRenderWindow.h"
#endif

#include "vtkVRRenderWindow.h"

#include "QVTKOpenGLWindow.h"
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqMainWindowEventManager.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqXRInterfaceControls.h"
#include "vtkCallbackCommand.h"
#include "vtkCullerCollection.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXRInterfaceCollaborationClient.h"
#include "vtkPVXRInterfaceExporter.h"
#include "vtkPVXRInterfacePluginLocation.h"
#include "vtkPVXRInterfaceWidgets.h"
#include "vtkPlaneSource.h"
#include "vtkQWidgetRepresentation.h"
#include "vtkQWidgetTexture.h"
#include "vtkQWidgetWidget.h"
#include "vtkRenderViewBase.h"
#include "vtkRendererCollection.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRepresentedArrayListDomain.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkStringArray.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVRFollower.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRModel.h"
#include "vtkVRRay.h"
#include "vtkVRRenderWindowInteractor.h"
#include "vtkVRRenderer.h"
#include "vtkVector.h"
#include "vtkXMLDataElement.h"
#include "vtkXRInterfacePolyfill.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"
#include <QCoreApplication>
#include <QModelIndex>
#include <chrono>
#include <sstream>
#include <thread>

struct vtkPVXRInterfaceHelper::vtkInternals
{
  vtkNew<vtkQWidgetWidget> QWidgetWidget;
  vtkPVRenderView* View = nullptr;
  vtkSmartPointer<vtkOpenGLRenderWindow> RenderWindow;
  vtkSmartPointer<vtkRenderWindowInteractor> Interactor;
  vtkTimeStamp PropUpdateTime;
  bool Done = false;
  std::vector<vtkVRCamera::Pose> SavedCameraPoses;
  std::size_t LastCameraPoseIndex = 0;
  std::vector<vtkPVXRInterfaceHelperLocation> Locations;
  int LoadLocationValue = -1;
  QVTKOpenGLWindow* ObserverWidget = nullptr;
  vtkNew<vtkOpenGLCamera> ObserverCamera;
  vtkNew<vtkPVXRInterfaceExporter> Exporter;

  // To simulate dpad with a trackpad on OpenXR we need to
  // store the last position
  double LeftTrackPadPosition[2] = { 0., 0. };
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVXRInterfaceHelper);

//----------------------------------------------------------------------------
vtkPVXRInterfaceHelper::vtkPVXRInterfaceHelper()
  : Internals(new vtkPVXRInterfaceHelper::vtkInternals())
{
  this->CollaborationClient->SetHelper(this);
  this->Widgets->SetHelper(this);
}

//----------------------------------------------------------------------------
vtkPVXRInterfaceHelper::~vtkPVXRInterfaceHelper() = default;

//==========================================================
// these methods mostly just forward to helper classes

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ExportLocationsAsSkyboxes(vtkSMViewProxy* view)
{
  this->SMView = view;
  this->Internals->View = vtkPVRenderView::SafeDownCast(view->GetClientSideView());

  // save the old values and create temp values to use
  vtkSmartPointer<vtkOpenGLRenderWindow> oldrw = this->Internals->RenderWindow;
  vtkSmartPointer<vtkOpenGLRenderer> oldr = this->Renderer;
  vtkSmartPointer<vtkRenderWindowInteractor> oldi = this->Internals->Interactor;
  this->Internals->RenderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(vtkSmartPointer<vtkRenderWindow>::New());
  this->Renderer = vtkOpenGLRenderer::SafeDownCast(vtkSmartPointer<vtkRenderer>::New());
  this->Internals->RenderWindow->AddRenderer(this->Renderer);
  this->Internals->Interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  this->Internals->RenderWindow->SetInteractor(this->Internals->Interactor);

  this->Internals->Exporter->ExportLocationsAsSkyboxes(
    this, view, this->Internals->Locations, this->Renderer);

  // restore previous values
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  this->Renderer = oldr;
  this->Internals->Interactor = oldi;
  this->Internals->RenderWindow = oldrw;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ExportLocationsAsView(vtkSMViewProxy* view)
{
  this->SMView = view;
  this->Internals->View = vtkPVRenderView::SafeDownCast(view->GetClientSideView());

  // save the old values and create temp values to use
  vtkSmartPointer<vtkOpenGLRenderWindow> oldrw = this->Internals->RenderWindow;
  vtkSmartPointer<vtkOpenGLRenderer> oldr = this->Renderer;
  vtkSmartPointer<vtkRenderWindowInteractor> oldi = this->Internals->Interactor;
  this->Internals->RenderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(vtkSmartPointer<vtkRenderWindow>::New());
  this->Renderer = vtkOpenGLRenderer::SafeDownCast(vtkSmartPointer<vtkRenderer>::New());
  this->Internals->RenderWindow->AddRenderer(this->Renderer);
  this->Internals->Interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  this->Internals->RenderWindow->SetInteractor(this->Internals->Interactor);

  this->Internals->Exporter->ExportLocationsAsView(this, view, this->Internals->Locations);

  // restore previous values
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  this->Renderer = oldr;
  this->Internals->Interactor = oldi;
  this->Internals->RenderWindow = oldrw;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::TakeMeasurement()
{
  this->ToggleShowControls();
  this->Widgets->TakeMeasurement(this->Internals->RenderWindow);
  this->SetShowRightControllerMarker(true);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::RemoveMeasurement()
{
  this->Widgets->RemoveMeasurement();
  this->SetShowRightControllerMarker(false);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetShowNavigationPanel(bool val)
{
  this->Widgets->SetShowNavigationPanel(val, this->Internals->RenderWindow);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetDefaultCropThickness(double val)
{
  this->Widgets->SetDefaultCropThickness(val);
}

//----------------------------------------------------------------------------
double vtkPVXRInterfaceHelper::GetDefaultCropThickness()
{
  return this->Widgets->GetDefaultCropThickness();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetEditableField(std::string val)
{
  this->Widgets->SetEditableField(val);
}

//----------------------------------------------------------------------------
std::string vtkPVXRInterfaceHelper::GetEditableField()
{
  return this->Widgets->GetEditableField();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabUpdateCropPlane(int index, double* origin, double* normal)
{
  this->Widgets->collabUpdateCropPlane(index, origin, normal);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabUpdateThickCrop(int index, double* matrix)
{
  this->Widgets->collabUpdateThickCrop(index, matrix);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::AddACropPlane(double* origin, double* normal)
{
  this->Widgets->AddACropPlane(origin, normal);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::RemoveAllCropPlanesAndThickCrops()
{
  this->collabRemoveAllCropPlanes();
  this->collabRemoveAllThickCrops();
  this->CollaborationClient->RemoveAllCropPlanes();
  this->CollaborationClient->RemoveAllThickCrops();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ShowCropPlanes(bool visible)
{
  this->Widgets->ShowCropPlanes(visible);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabRemoveAllCropPlanes()
{
  this->Widgets->collabRemoveAllCropPlanes();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabRemoveAllThickCrops()
{
  this->Widgets->collabRemoveAllThickCrops();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::AddAThickCrop(vtkTransform* intrans)
{
  this->Widgets->AddAThickCrop(intrans);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetCropSnapping(bool val)
{
  this->Widgets->SetCropSnapping(val);
}

// end of methods that mostly just forward to helper classes
//==========================================================

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetUseOpenXR(bool useOpenXr)
{
#if XRINTERFACE_HAS_OPENXR_SUPPORT
  this->UseOpenXR = useOpenXr;
#else
  if (useOpenXr)
  {
    vtkWarningMacro("Attempted to enable UseOpenXR without OpenXR support");
  }
#endif
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetUseOpenXRRemoting(bool useOpenXRRemoting)
{
#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
  this->UseOpenXRRemoting = useOpenXRRemoting;
#else
  if (useOpenXRRemoting)
  {
    vtkWarningMacro("Attempted to enable UseOpenXRRemoting without OpenXRRemoting support");
  }
#endif
}

//----------------------------------------------------------------------------
bool vtkPVXRInterfaceHelper::CollaborationConnect()
{
  if (!this->Renderer)
  {
    return false;
  }

  // add observers to vr rays
  if (auto* ovr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    vtkVRModel* cmodel = ovr_rw->GetModelForDevice(vtkEventDataDevice::LeftController);
    if (cmodel)
    {
      cmodel->GetRay()->AddObserver(
        vtkCommand::ModifiedEvent, this, &vtkPVXRInterfaceHelper::EventCallback);
    }
    cmodel = ovr_rw->GetModelForDevice(vtkEventDataDevice::RightController);
    if (cmodel)
    {
      cmodel->GetRay()->AddObserver(
        vtkCommand::ModifiedEvent, this, &vtkPVXRInterfaceHelper::EventCallback);
    }
  }

  return this->CollaborationClient->Connect(this->Renderer);
}

//----------------------------------------------------------------------------
bool vtkPVXRInterfaceHelper::CollaborationDisconnect()
{
  auto ovr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow);
  if (ovr_rw)
  {
    vtkVRModel* cmodel = ovr_rw->GetModelForDevice(vtkEventDataDevice::LeftController);
    if (cmodel)
    {
      cmodel->GetRay()->RemoveObservers(vtkCommand::ModifiedEvent);
    }
    cmodel = ovr_rw->GetModelForDevice(vtkEventDataDevice::RightController);
    if (cmodel)
    {
      cmodel->GetRay()->RemoveObservers(vtkCommand::ModifiedEvent);
    }
  }
  return this->CollaborationClient->Disconnect();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadNextCameraPose()
{
  if (this->Internals->SavedCameraPoses.empty())
  {
    return;
  }

  this->Internals->LastCameraPoseIndex =
    (this->Internals->LastCameraPoseIndex + 1) % this->Internals->SavedCameraPoses.size();
  this->LoadCameraPose(this->Internals->LastCameraPoseIndex);
}

//----------------------------------------------------------------------------
QStringList vtkPVXRInterfaceHelper::GetCustomViewpointToolTips()
{
  QStringList output;
  for (std::size_t i = 0; i < this->Internals->SavedCameraPoses.size(); ++i)
  {
    output << QString::number(i);
  }

  return output;
}

//----------------------------------------------------------------------------
std::size_t vtkPVXRInterfaceHelper::GetNextPoseIndex()
{
  return this->Internals->SavedCameraPoses.size();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadPoseInternal(vtkVRRenderWindow* vr_rw, std::size_t slot)
{
  if (slot < this->Internals->SavedCameraPoses.size())
  {
    // If we ever add ability to remove camera poses from the SavedCameraPoses
    // map, be sure to check that LastCameraPoseIndex isn't left holding a key
    // which is no longer valid.
    this->Internals->LastCameraPoseIndex = slot;
    vtkVRCamera::Pose pose = this->Internals->SavedCameraPoses[slot];
    vtkRenderer* ren = vr_rw->GetRenderers()->GetFirstRenderer();
    vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
    cam->ApplyPoseToCamera(&pose, vr_rw);
    ren->ResetCameraClippingRange();
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    auto slotPtr = reinterpret_cast<void*>(static_cast<std::uintptr_t>(slot));
    this->InvokeEvent(vtkCommand::LoadStateEvent, slotPtr);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadCameraPose(std::size_t slot)
{
  if (this->Internals->RenderWindow)
  {
    if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
    {
      this->LoadPoseInternal(vr_rw, slot);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SavePoseInternal(vtkVRRenderWindow* vr_rw, std::size_t slot)
{
  if (slot >= this->Internals->SavedCameraPoses.size())
  {
    this->Internals->SavedCameraPoses.resize(slot + 1);
  }

  vtkVRCamera::Pose& pose = this->Internals->SavedCameraPoses[slot];
  vtkRenderer* ren = vr_rw->GetRenderers()->GetFirstRenderer();
  vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
  cam->SetPoseFromCamera(&pose, vr_rw);
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  auto slotPtr = reinterpret_cast<void*>(static_cast<std::uintptr_t>(slot));
  this->InvokeEvent(vtkCommand::SaveStateEvent, slotPtr);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SaveCameraPose(std::size_t slot)
{
  constexpr std::size_t maxSlots = 6; // maximum number of saved camera poses
  if (slot >= maxSlots)
  {
    return;
  }

  if (this->Internals->RenderWindow)
  {
    auto vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow);
    if (vr_rw)
    {
      this->SavePoseInternal(vr_rw, slot);
    }
    else
    {
      this->SaveLocationState(slot);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ClearCameraPoses()
{
  this->Internals->SavedCameraPoses.clear();
  this->Internals->Locations.clear();
  this->Internals->LastCameraPoseIndex = 0;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabGoToPose(
  vtkVRCamera::Pose* pose, double* collabTrans, double* collabDir)
{
  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    vtkVRCamera* cam = vtkVRCamera::SafeDownCast(this->Renderer->GetActiveCamera());
    cam->ApplyPoseToCamera(pose, vr_rw);
    this->Renderer->ResetCameraClippingRange();
    vr_rw->UpdateHMDMatrixPose();
    vr_rw->SetPhysicalTranslation(collabTrans);
    vr_rw->SetPhysicalViewDirection(collabDir);
    vr_rw->UpdateHMDMatrixPose();
  }
  else
  {
    this->XRInterfacePolyfill->ApplyPose(pose, this->Renderer, this->Internals->RenderWindow);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ComeToMe()
{
  // get the current pose
  if (this->Internals->RenderWindow)
  {
    if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
    {
      vtkVRCamera* cam = vtkVRCamera::SafeDownCast(this->Renderer->GetActiveCamera());
      vtkVRCamera::Pose pose;
      cam->SetPoseFromCamera(&pose, vr_rw);
      double* collabTrans = this->XRInterfacePolyfill->GetPhysicalTranslation();
      double* collabDir = this->XRInterfacePolyfill->GetPhysicalViewDirection();
      this->CollaborationClient->GoToPose(pose, collabTrans, collabDir);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetViewUp(const std::string& axis)
{
  if (vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    if (axis == "+X")
    {
      renWin->SetPhysicalViewUp(1.0, 0.0, 0.0);
      renWin->SetPhysicalViewDirection(0.0, 0.0, -1.0);
    }
    else if (axis == "-X")
    {
      renWin->SetPhysicalViewUp(-1.0, 0.0, 0.0);
      renWin->SetPhysicalViewDirection(0.0, 0.0, 1.0);
    }
    else if (axis == "+Y")
    {
      renWin->SetPhysicalViewUp(0.0, 1.0, 0.0);
      renWin->SetPhysicalViewDirection(-1.0, 0.0, 0.0);
    }
    else if (axis == "-Y")
    {
      renWin->SetPhysicalViewUp(0.0, -1.0, 0.0);
      renWin->SetPhysicalViewDirection(1.0, 0.0, 0.0);
    }
    else if (axis == "+Z")
    {
      renWin->SetPhysicalViewUp(0.0, 0.0, 1.0);
      renWin->SetPhysicalViewDirection(0.0, -1.0, 0.0);
    }
    else if (axis == "-Z")
    {
      renWin->SetPhysicalViewUp(0.0, 0.0, -1.0);
      renWin->SetPhysicalViewDirection(0.0, 1.0, 0.0);
    }

    // Close menu
    this->ToggleShowControls();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetScaleFactor(float val)
{
  auto style = this->Internals->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->SetScale(this->Renderer->GetActiveCamera(), 1.0 / val);
    this->Renderer->ResetCameraClippingRange();
    this->ToggleShowControls();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetMotionFactor(float val)
{
  auto style = this->Internals->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->SetDollyPhysicalSpeed(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetBaseStationVisibility(bool v)
{
  if (this->BaseStationVisibility == v)
  {
    return;
  }

  this->BaseStationVisibility = v;
  if (this->Internals->RenderWindow)
  {
    if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
    {
      vr_rw->SetBaseStationVisibility(v);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ToggleShowControls()
{
  // only show when in VR not simulated
  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    this->Internals->QWidgetWidget->CreateDefaultRepresentation();
    this->Internals->QWidgetWidget->SetWidget(this->XRInterfaceControls);
    this->XRInterfaceControls->show();
    this->Internals->QWidgetWidget->SetCurrentRenderer(this->Renderer);
    this->Internals->QWidgetWidget->SetInteractor(this->Internals->Interactor);

    if (!this->Internals->QWidgetWidget->GetEnabled())
    {
      // place widget in front of the viewer, facing them
      double aspect = static_cast<double>(this->XRInterfaceControls->height()) /
        this->XRInterfaceControls->width();
      double scale = vr_rw->GetPhysicalScale();

      vtkVector3d camPos;
      this->Renderer->GetActiveCamera()->GetPosition(camPos.GetData());
      vtkVector3d camDOP;
      this->Renderer->GetActiveCamera()->GetDirectionOfProjection(camDOP.GetData());
      vtkVector3d physUp;
      vr_rw->GetPhysicalViewUp(physUp.GetData());

      // orthogonalize dop to vup
      camDOP = camDOP - physUp * camDOP.Dot(physUp);
      camDOP.Normalize();
      vtkVector3d vRight;
      vRight = camDOP.Cross(physUp);
      vtkVector3d center = camPos + camDOP * scale * 2.0;

      vtkPlaneSource* ps =
        this->Internals->QWidgetWidget->GetQWidgetRepresentation()->GetPlaneSource();

      vtkVector3d pos = center - 1.3 * physUp * scale * aspect - vRight * scale;
      ps->SetOrigin(pos.GetData());
      pos = center - 1.3 * physUp * scale * aspect + vRight * scale;
      ps->SetPoint1(pos.GetData());
      pos = center + 0.7 * physUp * scale * aspect - vRight * scale;
      ps->SetPoint2(pos.GetData());

      this->Internals->QWidgetWidget->SetInteractor(this->Internals->Interactor);
      this->Internals->QWidgetWidget->SetCurrentRenderer(this->Renderer);
      this->Internals->QWidgetWidget->SetEnabled(1);
      vtkVRModel* vrmodel = vr_rw->GetModelForDevice(vtkEventDataDevice::RightController);
      vrmodel->SetShowRay(true);
      vrmodel->SetRayLength(this->Renderer->GetActiveCamera()->GetClippingRange()[1]);
    }
    else
    {
      this->Internals->QWidgetWidget->SetEnabled(0);
      this->NeedStillRender = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetDrawControls(bool val)
{
  auto style = this->Internals->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->SetDrawControls(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabAddPointToSource(std::string const& name, double const* pnt)
{
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(
    pqActiveObjects::instance().proxyManager()->GetProxy("sources", name.c_str()));

  // first try addPoint
  vtkSMProperty* property = source->GetProperty("Points");
  // maybe a parametric function with points?
  if (!property)
  {
    property = source->GetProperty("ParametricFunction");
    if (property)
    {
      vtkSMPropertyHelper ptshelper(property);
      vtkSMProxy* param = ptshelper.GetAsProxy();
      property = nullptr;
      if (param)
      {
        property = param->GetProperty("Points");
      }
    }
  }

  if (property)
  {
    vtkSMPropertyHelper ptshelper(property);
    std::vector<double> pts = ptshelper.GetDoubleArray();
    pts.push_back(pnt[0]);
    pts.push_back(pnt[1]);
    pts.push_back(pnt[2]);
    ptshelper.SetNumberOfElements(static_cast<unsigned int>(pts.size()));
    ptshelper.Set(pts.data(), static_cast<unsigned int>(pts.size()));
  }

  source->UpdateVTKObjects();
  this->NeedStillRender = true;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::AddPointToSource(double const* pnt)
{
  // Get the selected node and try adding the point to that
  pqPipelineSource* psrc = this->XRInterfaceControls->GetSelectedPipelineSource();
  if (!psrc)
  {
    return;
  }
  vtkSMSourceProxy* source = psrc->getSourceProxy();
  if (!source)
  {
    return;
  }
  std::string name = psrc->getSMName().toUtf8().data();

  this->collabAddPointToSource(name, pnt);

  this->CollaborationClient->AddPointToSource(name, pnt);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetEditableFieldValue(std::string value)
{
  this->Widgets->SetEditableFieldValue(value);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetHoverPick(bool val)
{
  auto style = this->Internals->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->SetHoverPick(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetShowRightControllerMarker(bool visibility)
{
  vtkVRRenderer* ren = vtkVRRenderer::SafeDownCast(this->Renderer);

  if (!ren)
  {
    return;
  }

  // Check that the marker is not used anywhere before removing it
  if (!visibility &&
    (this->RightTriggerMode == vtkPVXRInterfaceHelper::ADD_POINT_TO_SOURCE ||
      this->RightTriggerMode == vtkPVXRInterfaceHelper::GRAB ||
      this->Widgets->IsMeasurementEnabled()))
  {
    return;
  }

  ren->SetShowRightMarker(visibility);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetRightTriggerMode(int index)
{
  this->HideBillboard();
  this->CollaborationClient->HideBillboard();
  this->RightTriggerMode = static_cast<vtkPVXRInterfaceHelper::RightTriggerAction>(index);

  auto style = this->Internals->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    : nullptr;
  if (!style)
  {
    return;
  }

  style->HidePickActor();

  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    vtkVRModel* vrmodel = vr_rw->GetModelForDevice(vtkEventDataDevice::RightController);

    if (vrmodel)
    {
      vrmodel->SetShowRay(this->Internals->QWidgetWidget->GetEnabled() ||
        this->RightTriggerMode == vtkPVXRInterfaceHelper::PICK);
    }

    style->GrabWithRayOff();

    switch (this->RightTriggerMode)
    {
      case vtkPVXRInterfaceHelper::ADD_POINT_TO_SOURCE:
      {
        this->SetShowRightControllerMarker(true);
        break;
      }
      case vtkPVXRInterfaceHelper::GRAB:
        this->SetShowRightControllerMarker(true);
        style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_POSITION_PROP);
        break;
      case vtkPVXRInterfaceHelper::PICK:
        this->SetShowRightControllerMarker(false);
        style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_POSITION_PROP);
        style->GrabWithRayOn();
        break;
      case vtkPVXRInterfaceHelper::INTERACTIVE_CROP:
        this->SetShowRightControllerMarker(false);
        style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_CLIP);
        break;
      case vtkPVXRInterfaceHelper::PROBE:
        this->SetShowRightControllerMarker(false);
        style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_PICK);
        break;
      case vtkPVXRInterfaceHelper::TELEPORTATION:
        this->SetShowRightControllerMarker(false);
        style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_TELEPORTATION);
        style->GrabWithRayOn();
        break;
      default:
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetMovementStyle(int index)
{
  // Get interactor style
  auto interactorStyle = this->Internals->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    : nullptr;

  if (interactorStyle)
  {
    interactorStyle->SetStyle(static_cast<vtkVRInteractorStyle::MovementStyle>(index));
  }
}

//----------------------------------------------------------------------------
bool vtkPVXRInterfaceHelper::InteractorEventCallback(
  vtkObject*, unsigned long eventID, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  this->Widgets->SetLastEventData(edata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();

  if (edd && edd->GetDevice() == vtkEventDataDevice::LeftController &&
    edd->GetAction() == vtkEventDataAction::Press && eventID == vtkCommand::NextPose3DEvent)
  {
    this->LoadNextCameraPose();
    return true;
  }

  if (edd && edd->GetDevice() == vtkEventDataDevice::LeftController &&
    this->Widgets->GetNavigationPanelVisibility() && eventID == vtkCommand::Move3DEvent /* &&
      cam->GetLeftEye() */)
  {
    this->Widgets->UpdateNavigationText(edd, this->Internals->RenderWindow);
  }

  // handle right trigger
  if (edd && eventID == vtkCommand::Select3DEvent)
  {
    // always pass events on to QWidget if enabled
    if (this->Internals->QWidgetWidget->GetEnabled())
    {
      return false;
    }

    // in add point mode, then do that
    if (this->RightTriggerMode == vtkPVXRInterfaceHelper::ADD_POINT_TO_SOURCE)
    {
      if (edd->GetAction() == vtkEventDataAction::Press)
      {
        double pos[4];
        edd->GetWorldPosition(pos);
        this->AddPointToSource(pos);
      }
      return true;
    }

    // otherwise let it pass onto someone else to handle
  }

  return false;
}

//----------------------------------------------------------------------------
namespace
{
template <typename T>
void addVectorAttribute(vtkPVXMLElement* el, const char* name, T* data, int count)
{
  std::ostringstream o;
  vtkNumberToString converter;
  for (int i = 0; i < count; ++i)
  {
    if (i)
    {
      o << " ";
    }
    o << converter.Convert(data[i]);
  }
  el->AddAttribute(name, o.str().c_str());
}
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::HandleDeleteEvent(vtkObject* caller)
{
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(caller);
  if (!proxy)
  {
    return;
  }

  // remove the proxy from all visibility lists
  for (auto& loc : this->Internals->Locations)
  {
    for (auto it = loc.Visibility.begin(); it != loc.Visibility.end();)
    {
      if (it->first == proxy)
      {
        it = loc.Visibility.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SaveState(vtkPVXMLElement* root)
{
  root->AddAttribute("PluginVersion", "1.2");

  // save the locations
  vtkNew<vtkPVXMLElement> e;
  e->SetName("Locations");
  for (std::size_t i = 0; i < this->Internals->Locations.size(); ++i)
  {
    auto& loc = this->Internals->Locations[i];

    vtkVRCamera::Pose& pose = *loc.Pose;
    vtkNew<vtkPVXMLElement> locel;
    locel->SetName("Location");
    locel->AddAttribute("PoseNumber", static_cast<int>(i));

    // camera pose
    {
      vtkNew<vtkPVXMLElement> el;
      el->SetName("CameraPose");
      el->AddAttribute("PoseNumber", static_cast<int>(i));
      addVectorAttribute(el, "Position", pose.Position, 3);
      el->AddAttribute("Distance", pose.Distance, 20);
      el->AddAttribute("MotionFactor", pose.MotionFactor, 20);
      addVectorAttribute(el, "Translation", pose.Translation, 3);
      addVectorAttribute(el, "InitialViewUp", pose.PhysicalViewUp, 3);
      addVectorAttribute(el, "InitialViewDirection", pose.PhysicalViewDirection, 3);
      addVectorAttribute(el, "ViewDirection", pose.ViewDirection, 3);
      locel->AddNestedElement(el);
    }

    locel->AddAttribute("NavigationPanel", loc.NavigationPanelVisibility);

    { // regular crops
      vtkNew<vtkPVXMLElement> el;
      el->SetName("CropPlanes");
      for (auto cps : loc.CropPlaneStates)
      {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Crop");
        child->AddAttribute("origin0", cps.first[0], 20);
        child->AddAttribute("origin1", cps.first[1], 20);
        child->AddAttribute("origin2", cps.first[2], 20);
        child->AddAttribute("normal0", cps.second[0], 20);
        child->AddAttribute("normal1", cps.second[1], 20);
        child->AddAttribute("normal2", cps.second[2], 20);
        el->AddNestedElement(child);
      }
      locel->AddNestedElement(el);
    }

    { // thick crops
      vtkNew<vtkPVXMLElement> el;
      el->SetName("ThickCrops");
      for (auto t : loc.ThickCropStates)
      {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Crop");
        for (int j = 0; j < 16; ++j)
        {
          std::ostringstream o;
          o << "transform" << j;
          child->AddAttribute(o.str().c_str(), t[j]);
        }
        el->AddNestedElement(child);
      }
      locel->AddNestedElement(el);
    }

    // save the visibility information
    {
      vtkNew<vtkPVXMLElement> el;
      el->SetName("Visibility");
      for (auto vdi : loc.Visibility)
      {
        vtkNew<vtkPVXMLElement> gchild;
        gchild->SetName("RepVisibility");
        gchild->AddAttribute("id", vdi.first->GetGlobalID());
        gchild->AddAttribute("visibility", vdi.second);
        el->AddNestedElement(gchild);
      }
      locel->AddNestedElement(el);
    }

    e->AddNestedElement(locel);
  }
  root->AddNestedElement(e);

  root->AddAttribute("CropSnapping", this->Widgets->GetCropSnapping() ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadState(vtkPVXMLElement* e, vtkSMProxyLocator* locator)
{
  this->Internals->Locations.clear();

  double version = -1.0;
  e->GetScalarAttribute("PluginVersion", &version);

  if (version < 1.1)
  {
    vtkErrorMacro("State file too old for XRInterface Plugin to load.");
    return;
  }

  if (version > 1.2)
  {
    vtkErrorMacro("State file too recent for XRInterface Plugin to load.");
    return;
  }

  // new style 1.2 or later
  vtkPVXMLElement* locels = e->FindNestedElementByName("Locations");
  if (locels)
  {
    int numnest = locels->GetNumberOfNestedElements();
    this->Internals->Locations.resize(numnest);
    for (int li = 0; li < numnest; ++li)
    {
      vtkPVXMLElement* locel = locels->GetNestedElement(li);
      int poseNum = 0;
      locel->GetScalarAttribute("PoseNumber", &poseNum);

      vtkPVXRInterfaceHelperLocation& loc = this->Internals->Locations[poseNum];

      vtkPVXMLElement* child = locel->FindNestedElementByName("CameraPose");
      if (child)
      {
        auto& pose = *loc.Pose;
        child->GetVectorAttribute("Position", 3, pose.Position);
        child->GetScalarAttribute("Distance", &pose.Distance);
        child->GetScalarAttribute("MotionFactor", &pose.MotionFactor);
        child->GetVectorAttribute("Translation", 3, pose.Translation);
        child->GetVectorAttribute("InitialViewUp", 3, pose.PhysicalViewUp);
        child->GetVectorAttribute("InitialViewDirection", 3, pose.PhysicalViewDirection);
        child->GetVectorAttribute("ViewDirection", 3, pose.ViewDirection);
      }

      child = locel->FindNestedElementByName("Visibility");
      if (child)
      {
        int numnest2 = child->GetNumberOfNestedElements();
        for (int i = 0; i < numnest2; ++i)
        {
          vtkPVXMLElement* gchild = child->GetNestedElement(i);
          int id = 0;
          gchild->GetScalarAttribute("id", &id);
          int vis;
          gchild->GetScalarAttribute("visibility", &vis);
          vtkSMProxy* proxy = locator->LocateProxy(id);
          if (!proxy)
          {
            vtkErrorMacro("unable to lookup proxy for id " << id);
          }
          else
          {
            loc.Visibility[proxy] = (vis != 0 ? true : false);
            proxy->AddObserver(
              vtkCommand::DeleteEvent, this, &vtkPVXRInterfaceHelper::EventCallback);
          }
        }
      }

      // load crops
      child = locel->FindNestedElementByName("CropPlanes");
      if (child)
      {
        int numnest2 = child->GetNumberOfNestedElements();
        for (int i = 0; i < numnest2; ++i)
        {
          vtkPVXMLElement* gchild = child->GetNestedElement(i);
          std::array<double, 3> origin;
          std::array<double, 3> normal;
          gchild->GetScalarAttribute("origin0", origin.data());
          gchild->GetScalarAttribute("origin1", origin.data() + 1);
          gchild->GetScalarAttribute("origin2", origin.data() + 2);
          gchild->GetScalarAttribute("normal0", normal.data());
          gchild->GetScalarAttribute("normal1", normal.data() + 1);
          gchild->GetScalarAttribute("normal2", normal.data() + 2);
          loc.CropPlaneStates.push_back(
            std::pair<std::array<double, 3>, std::array<double, 3>>(origin, normal));
        }
      }

      // load thick crops
      child = locel->FindNestedElementByName("ThickCrops");
      if (child)
      {
        int numnest2 = child->GetNumberOfNestedElements();
        for (int i = 0; i < numnest2; ++i)
        {
          std::array<double, 16> tform;
          vtkPVXMLElement* gchild = child->GetNestedElement(i);
          for (int j = 0; j < 16; ++j)
          {
            std::ostringstream o;
            o << "transform" << j;
            gchild->GetScalarAttribute(o.str().c_str(), tform.data() + j);
          }
          loc.ThickCropStates.push_back(tform);
        }
      }

      locel->GetScalarAttribute("NavigationPanel", &loc.NavigationPanelVisibility);
    }

    // if we are in VR then applyState
    if (this->Internals->Interactor)
    {
      this->ApplyState();
    }

    return;
  }

  // old style XML 1.1
  // load the camera poses and create a location per pose
  {
    vtkPVXMLElement* e2 = e->FindNestedElementByName("CameraPoses");
    if (e2)
    {
      int numnest = e2->GetNumberOfNestedElements();
      for (int i = 0; i < numnest; ++i)
      {
        vtkPVXMLElement* child = e2->GetNestedElement(i);
        int poseNum = 0;
        child->GetScalarAttribute("PoseNumber", &poseNum);

        vtkPVXRInterfaceHelperLocation& loc = this->Internals->Locations[poseNum];
        auto& pose = *loc.Pose;

        child->GetVectorAttribute("Position", 3, pose.Position);
        child->GetScalarAttribute("Distance", &pose.Distance);
        child->GetScalarAttribute("MotionFactor", &pose.MotionFactor);
        child->GetVectorAttribute("Translation", 3, pose.Translation);
        child->GetVectorAttribute("InitialViewUp", 3, pose.PhysicalViewUp);
        child->GetVectorAttribute("InitialViewDirection", 3, pose.PhysicalViewDirection);
        child->GetVectorAttribute("ViewDirection", 3, pose.ViewDirection);
      }
    }
  }

  // load the visibility information
  {
    vtkPVXMLElement* e2 = e->FindNestedElementByName("ExtraCameraLocationInformation");
    if (e2)
    {
      int numnest = e2->GetNumberOfNestedElements();
      for (int i = 0; i < numnest; ++i)
      {
        vtkPVXMLElement* child = e2->GetNestedElement(i);
        int slot = 0;
        child->GetScalarAttribute("slot", &slot);
        int cnumnest = child->GetNumberOfNestedElements();
        for (int j = 0; j < cnumnest; ++j)
        {
          vtkPVXMLElement* gchild = child->GetNestedElement(j);
          int id = 0;
          gchild->GetScalarAttribute("id", &id);
          int vis;
          gchild->GetScalarAttribute("visibility", &vis);
          vtkSMProxy* proxy = locator->LocateProxy(id);
          if (!proxy)
          {
            vtkErrorMacro("unable to lookup proxy for id " << id);
          }
          else
          {
            this->Internals->Locations[slot].Visibility[proxy] = (vis != 0 ? true : false);
          }
        }
      }
    }
  }

  for (auto& loc : this->Internals->Locations)
  {
    // navigation panel
    e->GetScalarAttribute("NavigationPanel", &loc.NavigationPanelVisibility);

    // load crops
    {
      vtkPVXMLElement* e2 = e->FindNestedElementByName("CropPlanes");
      if (e2)
      {
        int numnest = e2->GetNumberOfNestedElements();
        for (int i = 0; i < numnest; ++i)
        {
          vtkPVXMLElement* child = e2->GetNestedElement(i);
          std::array<double, 3> origin;
          std::array<double, 3> normal;
          child->GetScalarAttribute("origin0", origin.data());
          child->GetScalarAttribute("origin1", origin.data() + 1);
          child->GetScalarAttribute("origin2", origin.data() + 2);
          child->GetScalarAttribute("normal0", normal.data());
          child->GetScalarAttribute("normal1", normal.data() + 1);
          child->GetScalarAttribute("normal2", normal.data() + 2);
          loc.CropPlaneStates.push_back(
            std::pair<std::array<double, 3>, std::array<double, 3>>(origin, normal));
        }
      }
    }

    // load thick crops
    {
      vtkPVXMLElement* e2 = e->FindNestedElementByName("ThickCrops");
      if (e2)
      {
        int numnest = e2->GetNumberOfNestedElements();
        for (int i = 0; i < numnest; ++i)
        {
          std::array<double, 16> tform;
          vtkPVXMLElement* child = e2->GetNestedElement(i);
          for (int j = 0; j < 16; ++j)
          {
            std::ostringstream o;
            o << "transform" << j;
            child->GetScalarAttribute(o.str().c_str(), tform.data() + j);
          }
          loc.ThickCropStates.push_back(tform);
        }
      }
    }
  }

  int itmp = 0;
  if (e->GetScalarAttribute("CropSnapping", &itmp))
  {
    this->SetCropSnapping(itmp == 0 ? false : true);
  }

  // if we are in VR then applyState
  if (this->Internals->Interactor)
  {
    this->ApplyState();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ApplyState()
{
  // set camera poses
  this->Internals->SavedCameraPoses.resize(this->Internals->Locations.size());
  for (std::size_t i = 0; i < this->Internals->Locations.size(); ++i)
  {
    if (this->Internals->Locations[i].Pose)
    {
      auto& src = this->Internals->Locations[i].Pose;
      auto& dst = this->Internals->SavedCameraPoses[i];

      dst.Position[0] = src->Position[0];
      dst.Position[1] = src->Position[1];
      dst.Position[2] = src->Position[2];
      dst.PhysicalViewUp[0] = src->PhysicalViewUp[0];
      dst.PhysicalViewUp[1] = src->PhysicalViewUp[1];
      dst.PhysicalViewUp[2] = src->PhysicalViewUp[2];
      dst.PhysicalViewDirection[0] = src->PhysicalViewDirection[0];
      dst.PhysicalViewDirection[1] = src->PhysicalViewDirection[1];
      dst.PhysicalViewDirection[2] = src->PhysicalViewDirection[2];
      dst.ViewDirection[0] = src->ViewDirection[0];
      dst.ViewDirection[1] = src->ViewDirection[1];
      dst.ViewDirection[2] = src->ViewDirection[2];
      dst.Translation[0] = src->Translation[0];
      dst.Translation[1] = src->Translation[1];
      dst.Translation[2] = src->Translation[2];
      dst.Distance = src->Distance;
      dst.MotionFactor = src->MotionFactor;
    }
  }

  this->XRInterfaceControls->UpdateCustomViewpointsToolbar();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ShowBillboard(
  const std::string& text, bool updatePosition, std::string const& textureFile)
{
  this->Widgets->ShowBillboard(text, updatePosition, textureFile);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::HideBillboard()
{
  this->Widgets->HideBillboard();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::UpdateBillboard(bool updatePosition)
{
  this->Widgets->UpdateBillboard(updatePosition);
}

//----------------------------------------------------------------------------
bool vtkPVXRInterfaceHelper::EventCallback(vtkObject* caller, unsigned long eventID, void* calldata)
{
  // handle different events
  switch (eventID)
  {
    case vtkCommand::DeleteEvent:
    {
      this->HandleDeleteEvent(caller);
    }
    break;
    case vtkCommand::SaveStateEvent:
    {
      this->SaveLocationState(reinterpret_cast<std::uintptr_t>(calldata));
    }
    break;
    case vtkCommand::LoadStateEvent:
    {
      this->Internals->LoadLocationValue = reinterpret_cast<std::intptr_t>(calldata);
    }
    break;
    case vtkCommand::EndPickEvent:
    {
      this->Widgets->HandlePickEvent(caller, calldata);
    }
    break;
    case vtkCommand::ModifiedEvent:
    {
      auto vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow);
      vtkVRRay* ray = vtkVRRay::SafeDownCast(caller);
      if (vr_rw && ray)
      {
        // find the model
        vtkVRModel* model = vr_rw->GetModelForDevice(vtkEventDataDevice::LeftController);
        if (model && model->GetRay() == ray)
        {
          this->CollaborationClient->UpdateRay(model, vtkEventDataDevice::LeftController);
          return false;
        }
        model = vr_rw->GetModelForDevice(vtkEventDataDevice::RightController);
        if (model && model->GetRay() == ray)
        {
          this->CollaborationClient->UpdateRay(model, vtkEventDataDevice::RightController);
          return false;
        }
      }
    }
    break;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::GoToSavedLocation(
  std::size_t pos, double* collabTrans, double* collabDir)
{
  if (pos >= this->Internals->Locations.size())
  {
    return;
  }

  this->CollaborationClient->SetCurrentLocation(static_cast<int>(pos));

  auto& loc = this->Internals->Locations[pos];

  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    this->LoadPoseInternal(vr_rw, pos);
    vr_rw->UpdateHMDMatrixPose();
    vr_rw->SetPhysicalTranslation(collabTrans);
    vr_rw->SetPhysicalViewDirection(collabDir);
    vr_rw->UpdateHMDMatrixPose();
  }
  else
  {
    this->XRInterfacePolyfill->ApplyPose(
      loc.Pose.get(), this->Renderer, this->Internals->RenderWindow);
    this->Internals->LoadLocationValue = static_cast<int>(pos);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadLocationState(std::size_t slot)
{
  if (slot >= this->Internals->Locations.size())
  {
    return;
  }

  this->CollaborationClient->GoToSavedLocation(static_cast<int>(slot));

  auto& loc = this->Internals->Locations[slot];

  // apply navigation panel
  this->SetShowNavigationPanel(loc.NavigationPanelVisibility);

  // clear crops
  this->collabRemoveAllCropPlanes();
  this->collabRemoveAllThickCrops();

  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMProxy* repr = helper.GetAsProxy(i);
    vtkSMProperty* prop = repr ? repr->GetProperty("Visibility") : nullptr;
    if (prop)
    {
      auto ri = loc.Visibility.find(repr);
      bool vis = (vtkSMPropertyHelper(repr, "Visibility").GetAsInt() != 0 ? true : false);
      if (ri != loc.Visibility.end() && vis != ri->second)
      {
        vtkSMPropertyHelper(repr, "Visibility").Set((ri->second ? 1 : 0));
        repr->UpdateVTKObjects();
      }
    }
  }

  for (auto i : loc.CropPlaneStates)
  {
    this->AddACropPlane(i.first.data(), i.second.data());
  }

  // load thick crops
  vtkNew<vtkTransform> t;
  for (auto i : loc.ThickCropStates)
  {
    t->Identity();
    t->Concatenate(i.data());
    this->AddAThickCrop(t);
  }

  this->XRInterfaceControls->SetCurrentMotionFactor(loc.Pose->MotionFactor);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SaveLocationState(std::size_t slot)
{
  if (slot >= this->Internals->Locations.size())
  {
    this->Internals->Locations.resize(slot + 1);
  }

  vtkPVXRInterfaceHelperLocation& sd = this->Internals->Locations[slot];
  *sd.Pose = this->Internals->SavedCameraPoses[slot];

  this->Widgets->SaveLocationState(sd);

  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMProxy* repr = helper.GetAsProxy(i);
    vtkSMProperty* prop = repr ? repr->GetProperty("Visibility") : nullptr;
    if (prop)
    {
      sd.Visibility[repr] =
        (vtkSMPropertyHelper(repr, "Visibility").GetAsInt() != 0 ? true : false);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ViewRemoved(vtkSMViewProxy* smview)
{
  // if this is not our view then we don't care
  if (this->SMView != smview)
  {
    return;
  }
  this->Quit();
  this->SMView = nullptr;
  this->Internals->View = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::DoOneEvent()
{
  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow))
  {
    vtkVRRenderWindowInteractor::SafeDownCast(this->Internals->Interactor)
      ->DoOneEvent(vr_rw, this->Renderer);
  }
  else
  {
    double dist = this->Renderer->GetActiveCamera()->GetDistance();
    this->Renderer->ResetCameraClippingRange();
    double farz = this->Renderer->GetActiveCamera()->GetClippingRange()[1];
    this->Renderer->GetActiveCamera()->SetClippingRange(dist * 0.1, farz + 3 * dist);
    this->SMView->StillRender();
    this->NeedStillRender = false;
  }
}

//----------------------------------------------------------------------------
class vtkEndRenderObserver : public vtkCommand
{
public:
  static vtkEndRenderObserver* New() { return new vtkEndRenderObserver; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event),
    void* vtkNotUsed(calldata)) override
  {
    if (!this->ObserverWindow)
    {
      return;
    }

    if (!this->ObserverDraw)
    {
      // use a simple vertex shader
      // remove clang option once we move to cpp11 on clang
      std::string vshader = R"***(
        //VTK::System::Dec
        in vec4 ndCoordIn;
      in vec2 texCoordIn;
      out vec2 texCoords;
      uniform vec4 aspect;
      void main()
      {
        gl_Position = ndCoordIn * aspect * vec4(1.5, 1.5, 1.0, 1.0);
        texCoords = texCoordIn;
      }
      )***";

      // just add the standard VTK header to the fragment shader
      std::string fshader = "//VTK::System::Dec\n\n";

      fshader += R"***(
        in vec2 texCoords;
      out vec4 fragColor;
      uniform sampler2D screenTex;
      void main() { fragColor = vec4(texture(screenTex, texCoords.xy).rgb, 1.0); }
        )***";
      this->ObserverDraw =
        new vtkOpenGLQuadHelper(this->ObserverWindow, vshader.c_str(), fshader.c_str(), "");
    }
    else
    {
      this->ObserverWindow->GetShaderCache()->ReadyShaderProgram(this->ObserverDraw->Program);
    }

    auto& prog = this->ObserverDraw->Program;

    vtkOpenGLState* ostate = this->ObserverWindow->GetState();

    // push and bind
    ostate->PushFramebufferBindings();
    this->ObserverWindow->GetRenderFramebuffer()->Bind();
    this->ObserverWindow->GetRenderFramebuffer()->ActivateDrawBuffer(0);

    this->ObserverWindow->GetState()->vtkglDepthMask(GL_FALSE);
    this->ObserverWindow->GetState()->vtkglDisable(GL_DEPTH_TEST);

    int* size = this->ObserverWindow->GetSize();
    this->ObserverWindow->GetState()->vtkglViewport(0, 0, size[0], size[1]);
    this->ObserverWindow->GetState()->vtkglScissor(0, 0, size[0], size[1]);

    float faspect[4];
    faspect[0] = static_cast<float>(ResolveSize[0] * size[1]) / (ResolveSize[1] * size[0]);
    faspect[1] = 1.0;
    faspect[2] = 1.0;
    faspect[3] = 1.0;
    prog->SetUniform4f("aspect", faspect);
    // pass to ObserverTexture
    if (this->ObserverTexture->GetHandle() != this->RenderTextureHandle)
    {
      this->ObserverTexture->AssignToExistingTexture(this->RenderTextureHandle, GL_TEXTURE_2D);
    }

    this->ObserverTexture->Activate();
    prog->SetUniformi("screenTex", this->ObserverTexture->GetTextureUnit());

    // draw the full screen quad using the special shader
    this->ObserverDraw->Render();

    this->ObserverTexture->Deactivate();
    ostate->PopFramebufferBindings();
  }

  vtkOpenGLRenderWindow* ObserverWindow = nullptr;
  unsigned int RenderTextureHandle = 0;
  int ResolveSize[2];
  vtkSmartPointer<vtkTextureObject> ObserverTexture;
  vtkOpenGLQuadHelper* ObserverDraw = nullptr;

  void Initialize(vtkOpenGLRenderWindow* rw, unsigned int handle, int* resolveSize)
  {
    this->ObserverWindow = rw;
    this->RenderTextureHandle = handle;
    this->ResolveSize[0] = resolveSize[0];
    this->ResolveSize[1] = resolveSize[1];
    if (!this->ObserverTexture)
    {
      this->ObserverTexture = vtkSmartPointer<vtkTextureObject>::New();
      this->ObserverTexture->SetMagnificationFilter(vtkTextureObject::Linear);
      this->ObserverTexture->SetMinificationFilter(vtkTextureObject::Linear);
    }
    this->ObserverTexture->SetContext(rw);
  }

protected:
  vtkEndRenderObserver()
  {
    this->ObserverWindow = nullptr;
    this->ObserverTexture = nullptr;
    this->ObserverDraw = nullptr;
  }
  ~vtkEndRenderObserver() override
  {
    if (this->ObserverDraw != nullptr)
    {
      delete this->ObserverDraw;
      this->ObserverDraw = nullptr;
    }
    if (this->ObserverTexture != nullptr)
    {
      this->ObserverTexture = nullptr;
    }
  }
};

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::RenderXRView()
{
  if (this->Internals->ObserverWidget)
  {
    vtkRenderer* ren = this->Internals->RenderWindow->GetRenderers()->GetFirstRenderer();

#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
    vtkSmartPointer<vtkWin32OpenGLDXRenderWindow> hw = nullptr;
    if (this->UseOpenXR && this->UseOpenXRRemoting)
    {
      hw = vtkWin32OpenGLDXRenderWindow::SafeDownCast(
        vtkOpenXRRemotingRenderWindow::SafeDownCast(this->Internals->RenderWindow)
          ->GetHelperWindow());
      if (hw)
      {
        hw->Lock();
      }
    }
#endif

    vtkVRRenderWindow* vrrw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow);

    // bind framebuffer with texture
    this->Internals->RenderWindow->GetState()->PushFramebufferBindings();
    this->Internals->RenderWindow->GetRenderFramebuffer()->Bind();
    this->Internals->RenderWindow->GetRenderFramebuffer()->ActivateDrawBuffer(0);

    // save old camera
    auto* ocam = ren->GetActiveCamera();

    // update observer camera based on oldCam
    // watch for sudden jumps
    double pos[3];
    double dop[3];
    double vup[3];
    double fp[3];
    ocam->GetPosition(pos);
    ocam->GetFocalPoint(fp);
    ocam->GetDirectionOfProjection(dop);
    ocam->GetViewUp(vup);
    double distance = ocam->GetDistance();

    auto* opos = this->Internals->ObserverCamera->GetPosition();
    auto* odop = this->Internals->ObserverCamera->GetDirectionOfProjection();
    auto* ovup = this->Internals->ObserverCamera->GetViewUp();

    // adjust ratio based on current delta
    double ratio = 0.0;
    double angle = vtkMath::DegreesFromRadians(acos(vtkMath::Dot(vup, ovup)));
    if (angle > 3.0)
    {
      ratio = (angle - 3.0) / 27.0;
    }
    ratio = ratio > 1.0 ? 1.0 : ratio;
    double ratio2 = 1.0 - ratio;
    ocam->SetViewUp(ratio * vup[0] + ratio2 * ovup[0], ratio * vup[1] + ratio2 * ovup[1],
      ratio * vup[2] + ratio2 * ovup[2]);

    // adjust ratio based on movement
    double moveDist = sqrt(vtkMath::Distance2BetweenPoints(pos, opos));
    double scale = vrrw->GetPhysicalScale();
    moveDist /= scale;
    ratio = 0.0;
    if (moveDist > 0.05) // in meters
    {
      ratio = (moveDist - 0.05) / 0.5;
    }
    ratio = ratio > 1.0 ? 1.0 : ratio;
    ratio2 = 1.0 - ratio;
    ocam->SetPosition(ratio * pos[0] + ratio2 * opos[0], ratio * pos[1] + ratio2 * opos[1],
      ratio * pos[2] + ratio2 * opos[2]);

    ratio = 0.0;
    angle = vtkMath::DegreesFromRadians(acos(vtkMath::Dot(dop, odop)));
    if (angle > 3.0)
    {
      ratio = (angle - 3.0) / 27.0;
    }
    ratio = ratio > 1.0 ? 1.0 : ratio;
    ratio2 = 1.0 - ratio;
    double npos[3];
    ocam->GetPosition(npos);
    double ndop[3] = { ratio * dop[0] + ratio2 * odop[0], ratio * dop[1] + ratio2 * odop[1],
      ratio * dop[2] + ratio2 * odop[2] };
    vtkMath::Normalize(ndop);
    ocam->SetFocalPoint(
      npos[0] + distance * ndop[0], npos[1] + distance * ndop[1], npos[2] + distance * ndop[2]);

    // store new values
    this->Internals->ObserverCamera->SetPosition(ocam->GetPosition());
    this->Internals->ObserverCamera->SetFocalPoint(ocam->GetFocalPoint());
    this->Internals->ObserverCamera->SetViewUp(ocam->GetViewUp());

    // render
    this->Internals->RenderWindow->GetRenderers()->Render();
    vrrw->RenderModels();

    // restore
    ocam->SetPosition(pos);
    ocam->SetFocalPoint(fp);
    ocam->SetViewUp(vup);

    // resolve the framebuffer
    this->Internals->RenderWindow->GetDisplayFramebuffer()->Bind(GL_DRAW_FRAMEBUFFER);

    int* size = this->Internals->RenderWindow->GetDisplayFramebuffer()->GetLastSize();
    glBlitFramebuffer(
      0, 0, size[0], size[1], 0, 0, size[0], size[1], GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // unbind framebuffer
    this->Internals->RenderWindow->GetState()->PopFramebufferBindings();

    // do a FSQ render
    this->Internals->ObserverWidget->renderWindow()->Render();
    this->Internals->RenderWindow->MakeCurrent();

#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
    if (this->UseOpenXR && this->UseOpenXRRemoting && hw)
    {
      hw->Unlock();
    }
#endif
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ShowXRView()
{
  vtkVRRenderWindow* vrrw = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow);
  if (!vrrw)
  {
    return;
  }

  if (!this->Internals->ObserverWidget)
  {
    vrrw->MakeCurrent();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    this->Internals->ObserverWidget = new QVTKOpenGLWindow(ctx);
    this->Internals->ObserverWidget->setFormat(QVTKOpenGLWindow::defaultFormat());

    // we have to setup a close on main window close otherwise
    // qt will still think there is a toplevel window open and
    // will not close or destroy this view.
    pqMainWindowEventManager* mainWindowEventManager =
      pqApplicationCore::instance()->getMainWindowEventManager();
    QObject::connect(mainWindowEventManager, SIGNAL(close(QCloseEvent*)),
      this->Internals->ObserverWidget, SLOT(close()));

    vtkNew<vtkGenericOpenGLRenderWindow> observerWindow;
    observerWindow->GetState()->SetVBOCache(this->Internals->RenderWindow->GetVBOCache());
    this->Internals->ObserverWidget->setRenderWindow(observerWindow);
    this->Internals->ObserverWidget->resize(QSize(800, 600));
    this->Internals->ObserverWidget->show();

    vtkNew<vtkEndRenderObserver> endObserver;
    endObserver->Initialize(observerWindow,
      this->Internals->RenderWindow->GetDisplayFramebuffer()
        ->GetColorAttachmentAsTextureObject(0)
        ->GetHandle(),
      this->Internals->RenderWindow->GetSize());
    observerWindow->AddObserver(vtkCommand::RenderEvent, endObserver);
    observerWindow->SetMultiSamples(8);
  }
  else
  {
    this->Internals->ObserverWidget->show();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::AttachToCurrentView(vtkSMViewProxy* smview)
{
  this->SMView = smview;
  this->Internals->View = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());

  if (!this->Internals->View)
  {
    vtkErrorMacro("Attach without a valid view");
    return;
  }

  vtkOpenGLRenderer* pvRenderer =
    vtkOpenGLRenderer::SafeDownCast(this->Internals->View->GetRenderView()->GetRenderer());
  vtkOpenGLRenderWindow* pvRenderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(pvRenderer->GetVTKWindow());

  // we support desktop and OpenVR
  this->XRInterfacePolyfill->SetRenderWindow(pvRenderWindow);
  this->Internals->RenderWindow = pvRenderWindow;
  this->Renderer = pvRenderer;
  this->Internals->Interactor = this->Internals->RenderWindow->GetInteractor();

  // add a pick observer
  this->Internals->Interactor->GetInteractorStyle()->AddObserver(
    vtkCommand::EndPickEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);

  auto origLocked = this->Internals->View->GetLockBounds();

  // set locked bounds to prevent PV from setting it's own clipping range
  // which ignores avatars
  this->Internals->View->SetLockBounds(true);

  // start the event loop
  this->UpdateProps();
  this->Internals->RenderWindow->Render();
  this->Renderer->ResetCamera();
  this->Renderer->GetActiveCamera()->SetViewAngle(60);
  this->Renderer->ResetCameraClippingRange();
  this->ApplyState();
  this->Internals->Done = false;
  double lastRenderTime = 0;

  double minTime = 0.04; // 25 fps
  while (!this->Internals->Done)
  {
    // throttle rendering
    this->DoOneEvent();
    this->CollaborationClient->Render();

    QCoreApplication::processEvents();

    if (this->SMView && this->Internals->LoadLocationValue >= 0)
    {
      this->SMView->StillRender();
      this->LoadLocationState(this->Internals->LoadLocationValue);
      this->Internals->LoadLocationValue = -1;
      this->SMView->StillRender();
    }

    double currTime = vtkTimerLog::GetUniversalTime();
    if (currTime - lastRenderTime < minTime)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(
        static_cast<int>(1000.0 * (minTime - currTime + lastRenderTime))));
      lastRenderTime += minTime;
    }
    else
    {
      lastRenderTime = currTime;
    }
  }

  if (this->Internals->View)
  {
    this->Internals->View->SetLockBounds(origLocked);
  }

  // disconnect
  this->CollaborationClient->Disconnect();

  this->collabRemoveAllCropPlanes();
  this->collabRemoveAllThickCrops();

  this->HideBillboard();

  this->AddedProps->RemoveAllItems();
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor

  this->Renderer = nullptr;
  this->Internals->Interactor = nullptr;
  this->Internals->RenderWindow = nullptr;
}

//----------------------------------------------------------------------------
std::string vtkPVXRInterfaceHelper::GetOpenXRRuntimeVersionString() const
{
  std::string output;

#if XRINTERFACE_HAS_OPENXR_SUPPORT
  vtkSmartPointer<vtkOpenXRManagerConnection> cs;

  if (this->UseOpenXR)
  {
#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
    if (this->UseOpenXRRemoting)
    {
      cs = vtkSmartPointer<vtkOpenXRManagerRemoteConnection>::New();
      output = "OpenXR Remoting runtime version: ";
    }
    else
#endif
    {
      cs = vtkSmartPointer<vtkOpenXRManagerConnection>::New();
      output = "OpenXR runtime version: ";
    }
  }

  const auto version = vtkOpenXRManager::QueryInstanceVersion(cs);

  if (version.Major == 0 && version.Minor == 0 && version.Patch == 0)
  {
    output += "unknown";
  }
  else
  {
    output += std::to_string(version.Major) += '.';
    output += std::to_string(version.Minor) += '.';
    output += std::to_string(version.Patch);
  }
#endif

  return output;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SendToXR(vtkSMViewProxy* smview)
{
  this->SMView = smview;
  this->Internals->View = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());

  if (!this->Internals->View)
  {
    vtkErrorMacro("Send to VR without a valid view");
    return;
  }

  // are we already in VR ?
  if (this->Internals->Interactor)
  {
    // just update the actors and return
    this->UpdateProps();
    return;
  }

  vtkOpenGLRenderer* pvRenderer =
    vtkOpenGLRenderer::SafeDownCast(this->Internals->View->GetRenderView()->GetRenderer());
  vtkOpenGLRenderWindow* pvRenderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(pvRenderer->GetVTKWindow());

  std::string manifestDir;
  std::string pluginLocation = vtkPVXRInterfacePluginLocation::GetPluginLocation();
  if (!pluginLocation.empty())
  {
    // get the path
    manifestDir = vtksys::SystemTools::GetFilenamePath(pluginLocation) + "/";
  }
  else
  {
    qWarning("Unable to find action manifest directory from the plugin location");
  }

  vtkSmartPointer<vtkVRRenderWindow> renWin = nullptr;
  vtkSmartPointer<vtkVRRenderer> ren = nullptr;
  vtkSmartPointer<vtkVRRenderWindowInteractor> vriren = nullptr;

#if XRINTERFACE_HAS_OPENXR_SUPPORT
  if (this->UseOpenXR)
  {

#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
    if (this->UseOpenXRRemoting)
    {
      renWin = vtkSmartPointer<vtkOpenXRRemotingRenderWindow>::New();
      auto* renWinRemote = dynamic_cast<vtkOpenXRRemotingRenderWindow*>(renWin.Get());
      if (renWinRemote)
      {
        renWinRemote->SetRemotingIPAddress(this->RemotingAddress.c_str());
      }
      vtkNew<vtkWin32OpenGLDXRenderWindow> dxw;
      dxw->SetSharedRenderWindow(pvRenderWindow);
      renWin->SetHelperWindow(dxw);
    }
    else
#endif
    {
      renWin = vtkSmartPointer<vtkOpenXRRenderWindow>::New();
      renWin->MakeCurrent();
      renWin->SetHelperWindow(pvRenderWindow);
    }

    ren = vtkSmartPointer<vtkOpenXRRenderer>::New();
    vtkNew<vtkOpenXRRenderWindowInteractor> oxriren;
    vriren = oxriren;
    vriren->SetActionManifestDirectory(manifestDir);
    vriren->SetActionManifestFileName("pv_openxr_actions.json");

    oxriren->AddAction("showmenu",
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->ToggleShowControls();
        }
      });

    oxriren->AddAction("thickcropstart",
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->Widgets->MoveThickCrops(this->Internals->LeftTrackPadPosition[0] > 0.0);
        }
      });

    oxriren->AddAction("thickcropdirection",
      [this](vtkEventData* ed)
      {
        vtkEventDataDevice3D* edd = ed->GetAsEventDataDevice3D();
        if (edd)
        {
          edd->GetTrackPadPosition(this->Internals->LeftTrackPadPosition);
        }
      });

    oxriren->AddAction("forwardthickcrop",
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->Widgets->MoveThickCrops(true);
        }
      });

    oxriren->AddAction("backthickcrop",
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->Widgets->MoveThickCrops(false);
        }
      });

    oxriren->AddAction("shownavigationpanel",
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          bool visibility = !this->Widgets->GetNavigationPanelVisibility();
          this->SetShowNavigationPanel(visibility);
          this->XRInterfaceControls->SetNavigationPanel(visibility);
        }
      });
  }
  else
#else
  (void)pvRenderWindow;
#endif
  {
#if XRINTERFACE_HAS_OPENVR_SUPPORT
    renWin = vtkSmartPointer<vtkOpenVRRenderWindow>::New();
    renWin->SetHelperWindow(pvRenderWindow);
    ren = vtkSmartPointer<vtkOpenVRRenderer>::New();
    vtkNew<vtkOpenVRRenderWindowInteractor> ovriren;
    vriren = ovriren;
    vriren->SetActionManifestDirectory(manifestDir);
    vriren->SetActionManifestFileName("pv_openvr_actions.json");

    ovriren->AddAction("/actions/vtk/in/ShowMenu", false,
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->ToggleShowControls();
        }
      });

    ovriren->AddAction("/actions/vtk/in/ForwardThickCrop", false,
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->Widgets->MoveThickCrops(true);
        }
      });

    ovriren->AddAction("/actions/vtk/in/BackThickCrop", false,
      [this](vtkEventData* ed)
      {
        vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
        if (edd && edd->GetAction() == vtkEventDataAction::Press)
        {
          this->Widgets->MoveThickCrops(false);
        }
      });
#endif
  }

  this->Internals->RenderWindow = renWin;
  this->Renderer = ren;
  this->Internals->Interactor = vriren;

  if (!this->Internals->Interactor || !this->Internals->RenderWindow || !this->Renderer)
  {
    vtkErrorMacro("Attempted to SendToXR without any backend available");
    this->Internals->RenderWindow = nullptr;
    this->Internals->Interactor = nullptr;
    this->Renderer = nullptr;
    return;
  }

  this->XRInterfacePolyfill->SetRenderWindow(renWin);
  vtkVRInteractorStyle::SafeDownCast(this->Internals->Interactor->GetInteractorStyle())
    ->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_PICK);

  this->AddObserver(vtkCommand::SaveStateEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);
  this->AddObserver(vtkCommand::LoadStateEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);

  // pvRenderWindow is a vtkGenericOpenglRenderWindow

  renWin->SetBaseStationVisibility(this->BaseStationVisibility);
  this->Internals->RenderWindow->SetNumberOfLayers(2);
  this->Internals->RenderWindow->AddRenderer(this->Renderer);
  this->Internals->RenderWindow->SetInteractor(this->Internals->Interactor);

  // Setup camera
  renWin->InitializeViewFromCamera(pvRenderer->GetActiveCamera());

  this->Renderer->SetUseImageBasedLighting(pvRenderer->GetUseImageBasedLighting());
  this->Renderer->SetEnvironmentTexture(pvRenderer->GetEnvironmentTexture());

  this->Renderer->RemoveCuller(this->Renderer->GetCullers()->GetLastItem());
  this->Renderer->SetBackground(pvRenderer->GetBackground());
  this->Internals->RenderWindow->SetMultiSamples(this->MultiSample ? 8 : 0);

  // we always get first pick on events
  this->Internals->Interactor->AddObserver(
    vtkCommand::Select3DEvent, this, &vtkPVXRInterfaceHelper::InteractorEventCallback, 2.0);
  this->Internals->Interactor->AddObserver(
    vtkCommand::Move3DEvent, this, &vtkPVXRInterfaceHelper::InteractorEventCallback, 2.0);
  this->Internals->Interactor->AddObserver(
    vtkCommand::NextPose3DEvent, this, &vtkPVXRInterfaceHelper::InteractorEventCallback, 3.0);

  // create 4 lights for even lighting
  pvRenderer->GetLights()->RemoveAllItems();
  {
    vtkNew<vtkLight> light;
    light->SetPosition(0.0, 1.0, 0.0);
    light->SetIntensity(1.0);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
  }
  {
    vtkNew<vtkLight> light;
    light->SetPosition(0.8, -0.2, 0.0);
    light->SetIntensity(0.8);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
  }
  {
    vtkNew<vtkLight> light;
    light->SetPosition(-0.3, -0.2, 0.7);
    light->SetIntensity(0.6);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
  }
  {
    vtkNew<vtkLight> light;
    light->SetPosition(-0.3, -0.2, -0.7);
    light->SetIntensity(0.4);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
  }

  // add a pick observer
  this->Internals->Interactor->GetInteractorStyle()->AddObserver(
    vtkCommand::EndPickEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);

  this->Internals->RenderWindow->SetDesiredUpdateRate(200.0);
  this->Internals->Interactor->SetDesiredUpdateRate(200.0);
  this->Internals->Interactor->SetStillUpdateRate(200.0);

  this->Internals->RenderWindow->Initialize();
  vtkVRRenderWindow* vrRenWin = vtkVRRenderWindow::SafeDownCast(this->Internals->RenderWindow);

  if (vrRenWin && vrRenWin->GetVRInitialized())
  {
    // Set initial values
    this->SetRightTriggerMode(vtkPVXRInterfaceHelper::PICK);
    this->XRInterfaceControls->SetRightTriggerMode(vtkPVXRInterfaceHelper::PICK);
    this->XRInterfaceControls->SetMovementStyle(vtkVRInteractorStyle::FLY_STYLE);
    this->XRInterfaceControls->SetCurrentMotionFactor(1);
    this->XRInterfaceControls->SetShowFloor(true);
    this->XRInterfaceControls->SetInteractiveRay(false);
    this->XRInterfaceControls->SetNavigationPanel(false);
    this->XRInterfaceControls->SetSnapCropPlanes(false);

    // Ensure that the floor actor is displayed in case the checkbox
    // "Show Floor" stays checked between sessions
    bool shouldShowTheFloor = true;

    // As the OpenXRRemoting is only for the Hololens2 which is for AR application only,
    // we force to not display the floor for such context as it's not relevant
#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
    if (this->UseOpenXR && this->UseOpenXRRemoting)
    {
      shouldShowTheFloor = false;
    }
#endif
    ren->SetShowFloor(shouldShowTheFloor);

    // Retrieve initial View Up direction
    double* viewUpDir = renWin->GetPhysicalViewUp();

    if (viewUpDir[0] == -1)
    {
      this->XRInterfaceControls->SetCurrentViewUp("-X");
    }
    else if (viewUpDir[0] == 1)
    {
      this->XRInterfaceControls->SetCurrentViewUp("+X");
    }
    else if (viewUpDir[1] == -1)
    {
      this->XRInterfaceControls->SetCurrentViewUp("-Y");
    }
    else if (viewUpDir[1] == 1)
    {
      this->XRInterfaceControls->SetCurrentViewUp("+Y");
    }
    else if (viewUpDir[2] == -1)
    {
      this->XRInterfaceControls->SetCurrentViewUp("-Z");
    }
    else if (viewUpDir[2] == 1)
    {
      this->XRInterfaceControls->SetCurrentViewUp("+Z");
    }
    else
    {
      this->XRInterfaceControls->SetCurrentViewUp("");
    }

    // start the event loop
    this->UpdateProps();
    this->Internals->RenderWindow->Render();
    this->Renderer->ResetCamera();
    this->Renderer->ResetCameraClippingRange();
    this->ApplyState();

    this->Internals->Done = false;
    while (!this->Internals->Done)
    {
      this->Widgets->UpdateWidgetsFromParaView();
      this->DoOneEvent(); // calls render()
      this->RenderXRView();

      this->CollaborationClient->Render();
      QCoreApplication::processEvents();
      if (this->SMView && this->NeedStillRender)
      {
        this->SMView->StillRender();
        this->NeedStillRender = false;
      }
      if (this->SMView && this->Internals->LoadLocationValue >= 0)
      {
        this->SMView->StillRender();
        this->LoadLocationState(this->Internals->LoadLocationValue);
        this->Internals->LoadLocationValue = -1;
        this->SMView->StillRender();
      }
    }
  }

  // disconnect
  this->CollaborationClient->Disconnect();

  this->collabRemoveAllCropPlanes();
  this->collabRemoveAllThickCrops();

  if (this->Internals->ObserverWidget)
  {
    this->Internals->ObserverWidget->destroy();
    delete this->Internals->ObserverWidget;
    this->Internals->ObserverWidget = nullptr;
  }

  this->AddedProps->RemoveAllItems();
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  if (renWin)
  {
    renWin->SetHelperWindow(nullptr);
  }

  this->Internals->RenderWindow = nullptr;
  this->Internals->Interactor = nullptr;
  this->Renderer = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ResetCamera()
{
  if (!this->Renderer)
  {
    return;
  }

  // Close menu
  this->ToggleShowControls();

  this->Renderer->ResetCamera();
  this->Renderer->ResetCameraClippingRange();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ResetPositions()
{
  if (!this->Renderer)
  {
    return;
  }

  this->Internals->RenderWindow->MakeCurrent();

  vtkCollectionSimpleIterator pit;
  vtkProp* prop;

  for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
  {
    vtkProp3D* prop3d = vtkProp3D::SafeDownCast(prop);

    if (prop3d)
    {
      // Reset internal transformation
      prop3d->SetScale(1.0);
      prop3d->SetOrigin(0.0, 0.0, 0.0);
      prop3d->SetPosition(0.0, 0.0, 0.0);
      prop3d->SetOrientation(0.0, 0.0, 0.0);
    }
  }

  this->DoOneEvent();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::UpdateProps()
{
  if (!this->Internals->View)
  {
    return;
  }

  vtkRenderer* pvRenderer = this->Internals->View->GetRenderView()->GetRenderer();

  if (!this->Renderer)
  {
    return;
  }

  this->Internals->RenderWindow->MakeCurrent();
  if (pvRenderer->GetViewProps()->GetMTime() > this->Internals->PropUpdateTime ||
    this->AddedProps->GetNumberOfItems() == 0)
  {
    // remove prior props
    vtkCollectionSimpleIterator pit;
    vtkProp* prop;

    // if in VR remove props from VR renderer
    if (this->Renderer != pvRenderer)
    {
      for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
      {
        this->Renderer->RemoveViewProp(prop);
      }
    }

    vtkPropCollection* pcol = pvRenderer->GetViewProps();
    this->AddedProps->RemoveAllItems();
    for (pcol->InitTraversal(pit); (prop = pcol->GetNextProp(pit));)
    {
      // look for plane widgets and QWidgetRepresentations and do not add
      // them as we will be creating copies for VR so that we can interact
      // with them.
      auto* impPlane = vtkImplicitPlaneRepresentation::SafeDownCast(prop);
      auto* qwidgetrep = vtkQWidgetRepresentation::SafeDownCast(prop);
      if (impPlane || qwidgetrep)
      {
        continue;
      }

      this->AddedProps->AddItem(prop);
      // if in VR add props to VR renderer
      if (this->Renderer != pvRenderer)
      {
        this->Renderer->AddViewProp(prop);
      }
    }
    this->Internals->PropUpdateTime.Modified();
  }

  this->DoOneEvent();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::Quit()
{
  this->Internals->Done = true;
  this->Widgets->Quit();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVXRInterfaceHelper::InVR()
{
  return this->Internals->Interactor != nullptr;
}
