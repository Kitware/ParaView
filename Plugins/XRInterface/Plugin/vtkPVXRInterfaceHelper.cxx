/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXRInterfaceHelper.h"

// must be before others due to glew include
#include "vtkOpenVRRenderWindow.h"

#if PARAVIEW_HAS_OPENXR_SUPPORT
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
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
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
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
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRepresentedArrayListDomain.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
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
#include "vtkVRRenderWindow.h"
#include "vtkVRRenderWindowInteractor.h"
#include "vtkVRRenderer.h"
#include "vtkVectorOperators.h"
#include "vtkXMLDataElement.h"
#include "vtkXRInterfacePolyfill.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"
#include <QCoreApplication>
#include <QModelIndex>
#include <chrono>
#include <sstream>
#include <thread>

//----------------------------------------------------------------------------
vtkPVXRInterfaceHelperLocation::vtkPVXRInterfaceHelperLocation()
{
  this->Pose = new vtkVRCamera::Pose();
}

//----------------------------------------------------------------------------
vtkPVXRInterfaceHelperLocation::~vtkPVXRInterfaceHelperLocation()
{
  delete this->Pose;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVXRInterfaceHelper);

//----------------------------------------------------------------------------
vtkPVXRInterfaceHelper::vtkPVXRInterfaceHelper()
{
  this->View = nullptr;
  this->Renderer = nullptr;
  this->RenderWindow = nullptr;
  this->Interactor = nullptr;
  this->XRInterfacePolyfill = nullptr;

  this->AddedProps = vtkPropCollection::New();

  this->BaseStationVisibility = false;
  this->MultiSample = false;

  this->NeedStillRender = false;
  this->LoadLocationValue = -1;

  this->CollaborationClient = vtkPVXRInterfaceCollaborationClient::New();
  this->CollaborationClient->SetHelper(this);

  this->QWidgetWidget = nullptr;

  this->XRInterfacePolyfill = vtkXRInterfacePolyfill::New();

  this->ObserverWidget = nullptr;

  this->Widgets->SetHelper(this);
}

//----------------------------------------------------------------------------
vtkPVXRInterfaceHelper::~vtkPVXRInterfaceHelper()
{
  this->XRInterfacePolyfill->Delete();
  this->AddedProps->Delete();

  this->CollaborationClient->Delete();
  this->CollaborationClient = nullptr;
}

//==========================================================
// these methods mostly just forward to helper classes

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ExportLocationsAsSkyboxes(vtkSMViewProxy* view)
{
  this->SMView = view;
  this->View = vtkPVRenderView::SafeDownCast(view->GetClientSideView());

  // record the state if we are currently in vr
  if (this->Interactor)
  {
    this->RecordState();
  }

  // save the old values and create temp values to use
  vtkSmartPointer<vtkOpenGLRenderWindow> oldrw = this->RenderWindow;
  vtkSmartPointer<vtkOpenGLRenderer> oldr = this->Renderer;
  vtkSmartPointer<vtkRenderWindowInteractor> oldi = this->Interactor;
  this->RenderWindow = vtkOpenGLRenderWindow::SafeDownCast(vtkRenderWindow::New());
  this->Renderer = vtkOpenGLRenderer::SafeDownCast(vtkRenderer::New());
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Interactor = vtkRenderWindowInteractor::New();
  this->RenderWindow->SetInteractor(this->Interactor);

  this->Exporter->ExportLocationsAsSkyboxes(this, view, this->Locations, this->Renderer);

  // restore previous values
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  this->Renderer->Delete();
  this->Renderer = oldr;
  this->Interactor->Delete();
  this->Interactor = oldi;
  this->RenderWindow->Delete();
  this->RenderWindow = oldrw;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ExportLocationsAsView(vtkSMViewProxy* view)
{
  this->SMView = view;
  this->View = vtkPVRenderView::SafeDownCast(view->GetClientSideView());

  // record the state if we are currently in vr
  if (this->Interactor)
  {
    this->RecordState();
  }

  // save the old values and create temp values to use
  vtkSmartPointer<vtkOpenGLRenderWindow> oldrw = this->RenderWindow;
  vtkSmartPointer<vtkOpenGLRenderer> oldr = this->Renderer;
  vtkSmartPointer<vtkRenderWindowInteractor> oldi = this->Interactor;
  this->RenderWindow = vtkOpenGLRenderWindow::SafeDownCast(vtkRenderWindow::New());
  this->Renderer = vtkOpenGLRenderer::SafeDownCast(vtkRenderer::New());
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Interactor = vtkRenderWindowInteractor::New();
  this->RenderWindow->SetInteractor(this->Interactor);

  this->Exporter->ExportLocationsAsView(this, view, this->Locations);

  // restore previous values
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  this->Renderer->Delete();
  this->Renderer = oldr;
  this->Interactor->Delete();
  this->Interactor = oldi;
  this->RenderWindow->Delete();
  this->RenderWindow = oldrw;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::TakeMeasurement()
{
  this->ToggleShowControls();
  this->Widgets->TakeMeasurement(this->RenderWindow);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::RemoveMeasurement()
{
  this->Widgets->RemoveMeasurement();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetShowNavigationPanel(bool val)
{
  this->Widgets->SetShowNavigationPanel(val, this->RenderWindow);
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
void vtkPVXRInterfaceHelper::SetCropSnapping(int val)
{
  this->Widgets->SetCropSnapping(val);
}

// end of methods that mostly just forward to helper classes
//==========================================================

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetUseOpenXR(bool useOpenXr)
{
#if PARAVIEW_HAS_OPENXR_SUPPORT
  this->UseOpenXR = useOpenXr;
#else
  if (useOpenXr)
  {
    vtkWarningMacro("Attempted to enable UseOpenXR without OpenXR support");
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
  if (auto* ovr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
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
  auto ovr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
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
  if (this->SavedCameraPoses.empty())
  {
    return;
  }

  auto lowerIter = this->SavedCameraPoses.upper_bound(this->LastCameraPoseIndex);
  int nextValue = lowerIter != this->SavedCameraPoses.end() ? lowerIter->first
                                                            : this->SavedCameraPoses.begin()->first;

  this->LoadCameraPose(nextValue);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadPoseInternal(vtkVRRenderWindow* vr_rw, int slot)
{
  auto p_iter = this->SavedCameraPoses.find(slot);
  if (p_iter != this->SavedCameraPoses.end())
  {
    // If we ever add ability to remove camera poses from the SavedCameraPoses
    // map, be sure to check that LastCameraPoseIndex isn't left holding a key
    // which is no longer valid.
    this->LastCameraPoseIndex = slot;
    vtkVRCamera::Pose pose = p_iter->second;
    vtkRenderer* ren = static_cast<vtkRenderer*>(vr_rw->GetRenderers()->GetItemAsObject(0));
    vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
    cam->ApplyPoseToCamera(&pose, vr_rw);
    ren->ResetCameraClippingRange();
    this->InvokeEvent(vtkCommand::LoadStateEvent, reinterpret_cast<void*>(slot));
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadCameraPose(int slot)
{
  if (this->RenderWindow)
  {
    if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
    {
      this->LoadPoseInternal(vr_rw, slot);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SavePoseInternal(vtkVRRenderWindow* vr_rw, int slot)
{
  vtkVRCamera::Pose pose = this->SavedCameraPoses[slot];
  vtkRenderer* ren = static_cast<vtkRenderer*>(vr_rw->GetRenderers()->GetItemAsObject(0));
  vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
  cam->SetPoseFromCamera(&pose, vr_rw);
  this->InvokeEvent(vtkCommand::SaveStateEvent, reinterpret_cast<void*>(slot));
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SaveCameraPose(int slot)
{
  if (this->RenderWindow)
  {
    auto vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
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
void vtkPVXRInterfaceHelper::collabGoToPose(
  vtkVRCamera::Pose* pose, double* collabTrans, double* collabDir)
{
  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
  {
    vtkVRCamera* cam = static_cast<vtkVRCamera*>(this->Renderer->GetActiveCamera());
    cam->ApplyPoseToCamera(pose, vr_rw);
    this->Renderer->ResetCameraClippingRange();
    vr_rw->UpdateHMDMatrixPose();
    vr_rw->SetPhysicalTranslation(collabTrans);
    vr_rw->SetPhysicalViewDirection(collabDir);
    vr_rw->UpdateHMDMatrixPose();
  }
  else
  {
    this->XRInterfacePolyfill->ApplyPose(pose, this->Renderer, this->RenderWindow);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ComeToMe()
{
  // get the current pose
  if (this->RenderWindow)
  {
    if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
    {
      vtkVRCamera* cam = static_cast<vtkVRCamera*>(this->Renderer->GetActiveCamera());
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
  if (vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
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
  auto style = this->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle())
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
  auto style = this->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle())
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
  if (this->RenderWindow)
  {
    if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
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
  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
  {
    if (!this->QWidgetWidget)
    {
      this->QWidgetWidget = vtkQWidgetWidget::New();
      this->QWidgetWidget->CreateDefaultRepresentation();
      this->QWidgetWidget->SetWidget(this->XRInterfaceControls);
      this->XRInterfaceControls->show();
      // this->OpenVRControls->resize(960, 700);
      this->QWidgetWidget->SetCurrentRenderer(this->Renderer);
      this->QWidgetWidget->SetInteractor(this->Interactor);
    }

    if (!this->QWidgetWidget->GetEnabled())
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
      vtkVector3d center = camPos + camDOP * scale * 2.7;

      vtkPlaneSource* ps = this->QWidgetWidget->GetQWidgetRepresentation()->GetPlaneSource();

      vtkVector3d pos = center - 2.6 * physUp * scale * aspect - 2.0 * vRight * scale;
      ps->SetOrigin(pos.GetData());
      pos = center - 2.6 * physUp * scale * aspect + 2.0 * vRight * scale;
      ps->SetPoint1(pos.GetData());
      pos = center + 1.4 * physUp * scale * aspect - 2.0 * vRight * scale;
      ps->SetPoint2(pos.GetData());

      this->QWidgetWidget->SetInteractor(this->Interactor);
      this->QWidgetWidget->SetCurrentRenderer(this->Renderer);
      this->QWidgetWidget->SetEnabled(1);
      vtkVRModel* vrmodel = vr_rw->GetModelForDevice(vtkEventDataDevice::RightController);
      vrmodel->SetShowRay(true);
      vrmodel->SetRayLength(this->Renderer->GetActiveCamera()->GetClippingRange()[1]);
    }
    else
    {
      this->QWidgetWidget->SetEnabled(0);
      this->NeedStillRender = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetDrawControls(bool val)
{
  auto style = this->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->SetDrawControls(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::collabAddPointToSource(std::string const& name, double const* pnt)
{
  vtkSMSourceProxy* source = static_cast<vtkSMSourceProxy*>(
    pqActiveObjects::instance().proxyManager()->GetProxy("sources", name.c_str()));

  // pqPipelineSource* psrc =
  //   pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(source);
  // psrc->getRepresentations(this->SMView);

  // find the matching repr
  vtkSMPVRepresentationProxy* repr = nullptr;
  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMPVRepresentationProxy* repr2 =
      vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(i));

    if (!repr2 || !repr2->GetProperty("Input"))
    {
      continue;
    }

    vtkSMPropertyHelper helper2(repr2, "Input");
    vtkSMSourceProxy* rsource = vtkSMSourceProxy::SafeDownCast(helper2.GetAsProxy());
    if (!rsource || rsource != source)
    {
      continue;
    }

    repr = repr2;
    break;
  }

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

  source->UpdateVTKObjects(); // maybe?
  // source->MarkDirty(source);
  // source->UpdateSelfAndAllInputs();
  // source->UpdatePipeline();
  // if (repr)
  // {
  //   repr->UpdateSelfAndAllInputs();
  //   repr->UpdatePipeline();
  // }
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
  auto style = this->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->SetHoverPick(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetRightTriggerMode(std::string const& text)
{
  this->HideBillboard();
  this->CollaborationClient->HideBillboard();
  this->RightTriggerMode = text;

  auto style = this->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle())
    : nullptr;
  if (!style)
  {
    return;
  }

  style->HidePickActor();

  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
  {
    vtkVRModel* vrmodel = vr_rw->GetModelForDevice(vtkEventDataDevice::RightController);

    if (vrmodel)
    {
      vrmodel->SetShowRay(this->QWidgetWidget->GetEnabled() || text == "Pick");
    }

    style->GrabWithRayOff();

    if (text == "Grab")
    {
      style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_POSITION_PROP);
    }
    else if (text == "Pick")
    {
      style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_POSITION_PROP);
      style->GrabWithRayOn();
    }
    else if (text == "Interactive Crop")
    {
      style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_CLIP);
    }
    else if (text == "Probe")
    {
      style->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_PICK);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SetMovementStyle(vtkVRInteractorStyle::MovementStyle style)
{
  // Get interactor style
  auto interactorStyle = this->Interactor
    ? vtkVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle())
    : nullptr;
  if (interactorStyle)
  {
    interactorStyle->SetStyle(style);
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
    this->Widgets->UpdateNavigationText(edd, this->RenderWindow);
  }

  // handle right trigger
  if (edd && eventID == vtkCommand::Select3DEvent)
  {
    // always pass events on to QWidget if enabled
    if (this->QWidgetWidget && this->QWidgetWidget->GetEnabled())
    {
      return false;
    }

    // in add point mode, then do that
    if (this->RightTriggerMode == "Add Point To Source")
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
  vtkNumberToString convert;
  for (int i = 0; i < count; ++i)
  {
    if (i)
    {
      o << " ";
    }
    o << convert(data[i]);
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
  for (auto& loci : this->Locations)
  {
    auto& loc = loci.second;
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
  // if we are in VR then RecordState first
  if (this->Interactor)
  {
    this->RecordState();
  }

  root->AddAttribute("PluginVersion", "1.2");

  // save the locations
  vtkNew<vtkPVXMLElement> e;
  e->SetName("Locations");
  for (auto& loci : this->Locations)
  {
    auto& loc = loci.second;

    vtkVRCamera::Pose& pose = *(loc.Pose);
    vtkNew<vtkPVXMLElement> locel;
    locel->SetName("Location");
    locel->AddAttribute("PoseNumber", loci.first);

    // camera pose
    {
      vtkNew<vtkPVXMLElement> el;
      el->SetName("CameraPose");
      el->AddAttribute("PoseNumber", loci.first);
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
      for (auto i : loc.CropPlaneStates)
      {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Crop");
        child->AddAttribute("origin0", i.first[0], 20);
        child->AddAttribute("origin1", i.first[1], 20);
        child->AddAttribute("origin2", i.first[2], 20);
        child->AddAttribute("normal0", i.second[0], 20);
        child->AddAttribute("normal1", i.second[1], 20);
        child->AddAttribute("normal2", i.second[2], 20);
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
        for (int i = 0; i < 16; ++i)
        {
          std::ostringstream o;
          o << "transform" << i;
          child->AddAttribute(o.str().c_str(), t[i]);
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
void vtkPVXRInterfaceHelper::RecordState()
{
  // save the camera poses
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadState(vtkPVXMLElement* e, vtkSMProxyLocator* locator)
{
  this->Locations.clear();

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
    for (int li = 0; li < numnest; ++li)
    {
      vtkPVXMLElement* locel = locels->GetNestedElement(li);
      int poseNum = 0;
      locel->GetScalarAttribute("PoseNumber", &poseNum);

      vtkPVXRInterfaceHelperLocation& loc = this->Locations[poseNum];

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
    if (this->Interactor)
    {
      this->ApplyState();
    }

    // update the lost of locations in the GUI
    std::vector<int> locs;
    for (auto& l : this->Locations)
    {
      locs.push_back(l.first);
    }
    this->XRInterfaceControls->SetAvailablePositions(locs);

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

        vtkPVXRInterfaceHelperLocation& loc = this->Locations[poseNum];
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
            this->Locations[slot].Visibility[proxy] = (vis != 0 ? true : false);
          }
        }
      }
    }
  }

  for (auto& loci : this->Locations)
  {
    auto& loc = loci.second;

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

    // update the lost of locations in the GUI
    std::vector<int> locs;
    for (auto& l : this->Locations)
    {
      locs.push_back(l.first);
    }
    this->XRInterfaceControls->SetAvailablePositions(locs);

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
  if (this->Interactor)
  {
    this->ApplyState();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ApplyState()
{
  // apply crop snapping setting first
  // this->CropMenu->RenameMenuItem(
  //   "togglesnapping", (this->CropSnapping ? "Turn Snap to Axes Off" : "Turn
  //   Snap to Axes On"));

  // set camera poses
  for (auto& loci : this->Locations)
  {
    if (loci.second.Pose)
    {
      auto& src = loci.second.Pose;
      auto& dst = this->SavedCameraPoses[loci.first];

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
      this->SaveLocationState(reinterpret_cast<vtkTypeInt64>(calldata));
    }
    break;
    case vtkCommand::LoadStateEvent:
    {
      this->LoadLocationValue = reinterpret_cast<vtkTypeInt64>(calldata);
    }
    break;
    case vtkCommand::EndPickEvent:
    {
      this->Widgets->HandlePickEvent(caller, calldata);
    }
    break;
    case vtkCommand::ModifiedEvent:
    {
      auto vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
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
void vtkPVXRInterfaceHelper::GoToSavedLocation(int pos, double* collabTrans, double* collabDir)
{
  this->CollaborationClient->SetCurrentLocation(pos);

  auto sdi = this->Locations.find(pos);
  if (sdi == this->Locations.end())
  {
    return;
  }
  auto& loc = sdi->second;

  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
  {
    this->LoadPoseInternal(vr_rw, pos);
    vr_rw->UpdateHMDMatrixPose();
    vr_rw->SetPhysicalTranslation(collabTrans);
    vr_rw->SetPhysicalViewDirection(collabDir);
    vr_rw->UpdateHMDMatrixPose();
  }
  else
  {
    this->XRInterfacePolyfill->ApplyPose(loc.Pose, this->Renderer, this->RenderWindow);
    this->LoadLocationValue = pos;
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::LoadLocationState(int slot)
{
  auto sdi = this->Locations.find(slot);
  if (sdi == this->Locations.end())
  {
    return;
  }

  this->CollaborationClient->GoToSavedLocation(slot);

  auto& loc = sdi->second;

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
      auto ri = sdi->second.Visibility.find(repr);
      bool vis = (vtkSMPropertyHelper(repr, "Visibility").GetAsInt() != 0 ? true : false);
      if (ri != sdi->second.Visibility.end() && vis != ri->second)
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

  this->XRInterfaceControls->SetCurrentPosition(slot);
  this->XRInterfaceControls->SetCurrentScaleFactor(loc.Pose->Distance);
  this->XRInterfaceControls->SetCurrentMotionFactor(loc.Pose->MotionFactor);
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SaveLocationState(int slot)
{
  vtkPVXRInterfaceHelperLocation& sd = this->Locations[slot];

  auto p_iter = this->SavedCameraPoses.find(slot);
  if (p_iter == this->SavedCameraPoses.end())
  {
    return;
  }

  *sd.Pose = p_iter->second;

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

  // update the lost of locations in the GUI
  std::vector<int> locs;
  for (auto& l : this->Locations)
  {
    locs.push_back(l.first);
  }
  this->XRInterfaceControls->SetAvailablePositions(locs);
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
  this->View = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::DoOneEvent()
{
  if (auto* vr_rw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow))
  {
    static_cast<vtkVRRenderWindowInteractor*>(this->Interactor)->DoOneEvent(vr_rw, this->Renderer);
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
  vtkTextureObject* ObserverTexture = nullptr;
  vtkOpenGLQuadHelper* ObserverDraw = nullptr;

  void Initialize(vtkOpenGLRenderWindow* rw, unsigned int handle, int* resolveSize)
  {
    this->ObserverWindow = rw;
    this->RenderTextureHandle = handle;
    this->ResolveSize[0] = resolveSize[0];
    this->ResolveSize[1] = resolveSize[1];
    if (!this->ObserverTexture)
    {
      this->ObserverTexture = vtkTextureObject::New();
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
      this->ObserverTexture->Delete();
      this->ObserverTexture = nullptr;
    }
  }
};

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::RenderXRView()
{
  if (this->ObserverWidget)
  {
    vtkRenderer* ren =
      static_cast<vtkRenderer*>(this->RenderWindow->GetRenderers()->GetItemAsObject(0));

    vtkVRRenderWindow* vrrw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);

    // bind framebuffer with texture
    this->RenderWindow->GetState()->PushFramebufferBindings();
    this->RenderWindow->GetRenderFramebuffer()->Bind();
    this->RenderWindow->GetRenderFramebuffer()->ActivateDrawBuffer(0);

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

    auto* opos = this->ObserverCamera->GetPosition();
    auto* odop = this->ObserverCamera->GetDirectionOfProjection();
    auto* ovup = this->ObserverCamera->GetViewUp();

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
    this->ObserverCamera->SetPosition(ocam->GetPosition());
    this->ObserverCamera->SetFocalPoint(ocam->GetFocalPoint());
    this->ObserverCamera->SetViewUp(ocam->GetViewUp());

    // render
    this->RenderWindow->GetRenderers()->Render();
    vrrw->RenderModels();

    // restore
    ocam->SetPosition(pos);
    ocam->SetFocalPoint(fp);
    ocam->SetViewUp(vup);

    // resolve the framebuffer
    this->RenderWindow->GetDisplayFramebuffer()->Bind(GL_DRAW_FRAMEBUFFER);

    int* size = this->RenderWindow->GetDisplayFramebuffer()->GetLastSize();
    glBlitFramebuffer(
      0, 0, size[0], size[1], 0, 0, size[0], size[1], GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // unbind framebuffer
    this->RenderWindow->GetState()->PopFramebufferBindings();

    // do a FSQ render
    this->ObserverWidget->renderWindow()->Render();
    this->RenderWindow->MakeCurrent();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ShowXRView()
{
  vtkVRRenderWindow* vrrw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  if (!vrrw)
  {
    return;
  }

  if (!this->ObserverWidget)
  {
    vrrw->MakeCurrent();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    this->ObserverWidget = new QVTKOpenGLWindow(ctx);
    this->ObserverWidget->setFormat(QVTKOpenGLWindow::defaultFormat());

    // we have to setup a close on main window close otherwise
    // qt will still think there is a toplevel window open and
    // will not close or destroy this view.
    pqMainWindowEventManager* mainWindowEventManager =
      pqApplicationCore::instance()->getMainWindowEventManager();
    QObject::connect(
      mainWindowEventManager, SIGNAL(close(QCloseEvent*)), this->ObserverWidget, SLOT(close()));

    vtkNew<vtkGenericOpenGLRenderWindow> observerWindow;
    observerWindow->GetState()->SetVBOCache(this->RenderWindow->GetVBOCache());
    this->ObserverWidget->setRenderWindow(observerWindow);
    this->ObserverWidget->resize(QSize(800, 600));
    this->ObserverWidget->show();

    vtkNew<vtkEndRenderObserver> endObserver;
    endObserver->Initialize(observerWindow,
      this->RenderWindow->GetDisplayFramebuffer()
        ->GetColorAttachmentAsTextureObject(0)
        ->GetHandle(),
      this->RenderWindow->GetSize());
    observerWindow->AddObserver(vtkCommand::RenderEvent, endObserver);
    observerWindow->SetMultiSamples(8);
  }
  else
  {
    this->ObserverWidget->show();
  }
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::AttachToCurrentView(vtkSMViewProxy* smview)
{
  this->SMView = smview;
  this->View = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());

  if (!this->View)
  {
    vtkErrorMacro("Attach without a valid view");
    return;
  }

  vtkOpenGLRenderer* pvRenderer =
    vtkOpenGLRenderer::SafeDownCast(this->View->GetRenderView()->GetRenderer());
  vtkOpenGLRenderWindow* pvRenderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(pvRenderer->GetVTKWindow());

  // we support desktop and OpenVR
  this->XRInterfacePolyfill->SetRenderWindow(pvRenderWindow);
  this->RenderWindow = pvRenderWindow;
  this->RenderWindow->Register(this);
  this->Renderer = pvRenderer;
  this->Renderer->Register(this);
  this->Interactor = this->RenderWindow->GetInteractor();
  this->Interactor->Register(this);

  // add a pick observer
  this->Interactor->GetInteractorStyle()->AddObserver(
    vtkCommand::EndPickEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);

  auto origLocked = this->View->GetLockBounds();

  // set locked bounds to prevent PV from setting it's own clipping range
  // which ignores avatars
  this->View->SetLockBounds(true);

  // start the event loop
  this->UpdateProps();
  this->RenderWindow->Render();
  this->Renderer->ResetCamera();
  this->Renderer->GetActiveCamera()->SetViewAngle(60);
  this->Renderer->ResetCameraClippingRange();
  this->ApplyState();
  this->Done = false;
  double lastRenderTime = 0;

  double minTime = 0.04; // 25 fps
  while (!this->Done)
  {
    // throttle rendering
    this->DoOneEvent();
    this->CollaborationClient->Render();

    QCoreApplication::processEvents();

    if (this->SMView && this->LoadLocationValue >= 0)
    {
      this->SMView->StillRender();
      this->LoadLocationState(this->LoadLocationValue);
      this->LoadLocationValue = -1;
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

  if (this->View)
  {
    this->View->SetLockBounds(origLocked);
  }

  // disconnect
  this->CollaborationClient->Disconnect();

  // record the last state upon exiting VR
  // so that a later SaveState will have our last
  // recorded data
  this->RecordState();
  this->collabRemoveAllCropPlanes();
  this->collabRemoveAllThickCrops();

  this->HideBillboard();

  this->AddedProps->RemoveAllItems();
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  this->Renderer->Delete();
  this->Renderer = nullptr;
  this->Interactor->Delete();
  this->Interactor = nullptr;
  this->RenderWindow->Delete();
  this->RenderWindow = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::SendToXR(vtkSMViewProxy* smview)
{
  this->SMView = smview;
  this->View = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());

  if (!this->View)
  {
    vtkErrorMacro("Send to VR without a valid view");
    return;
  }

  // are we already in VR ?
  if (this->Interactor)
  {
    // just update the actors and return
    this->UpdateProps();
    return;
  }

  vtkOpenGLRenderer* pvRenderer =
    vtkOpenGLRenderer::SafeDownCast(this->View->GetRenderView()->GetRenderer());
  vtkOpenGLRenderWindow* pvRenderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(pvRenderer->GetVTKWindow());

  std::string manifestFile;
  std::string pluginLocation = vtkPVXRInterfacePluginLocation::GetPluginLocation();
  if (!pluginLocation.empty())
  {
    // get the path
    pluginLocation = vtksys::SystemTools::GetFilenamePath(pluginLocation);
    manifestFile = pluginLocation + "/";
  }

  vtkVRRenderWindow* renWin = nullptr;
  vtkVRRenderer* ren = nullptr;
  vtkVRRenderWindowInteractor* vriren = nullptr;

#if PARAVIEW_HAS_OPENXR_SUPPORT
  if (this->UseOpenXR)
  {
    renWin = static_cast<vtkVRRenderWindow*>(vtkOpenXRRenderWindow::New());
    renWin->MakeCurrent();
    renWin->SetHelperWindow(pvRenderWindow);
    ren = static_cast<vtkVRRenderer*>(vtkOpenXRRenderer::New());
    vtkOpenXRRenderWindowInteractor* oxriren = vtkOpenXRRenderWindowInteractor::New();
    vriren = static_cast<vtkVRRenderWindowInteractor*>(oxriren);
    manifestFile += "pv_openxr_actions.json";
    vriren->SetActionManifestFileName(manifestFile.c_str());

    oxriren->AddAction("showmenu", [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->ToggleShowControls();
      }
    });

    oxriren->AddAction("thickcropstart", [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->Widgets->MoveThickCrops(this->LeftTrackPadPosition[0] > 0.0);
      }
    });

    oxriren->AddAction("thickcropdirection", [this](vtkEventData* ed) {
      vtkEventDataDevice3D* edd = ed->GetAsEventDataDevice3D();
      if (edd)
      {
        edd->GetTrackPadPosition(this->LeftTrackPadPosition);
      }
    });

    oxriren->AddAction("forwardthickcrop", [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->Widgets->MoveThickCrops(true);
      }
    });

    oxriren->AddAction("backthickcrop", [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->Widgets->MoveThickCrops(false);
      }
    });
  }
  else
  {
#endif
    renWin = static_cast<vtkVRRenderWindow*>(vtkOpenVRRenderWindow::New());
    renWin->SetHelperWindow(pvRenderWindow);
    ren = static_cast<vtkVRRenderer*>(vtkOpenVRRenderer::New());
    vtkOpenVRRenderWindowInteractor* ovriren = vtkOpenVRRenderWindowInteractor::New();
    vriren = static_cast<vtkVRRenderWindowInteractor*>(ovriren);
    manifestFile += "pv_openvr_actions.json";
    vriren->SetActionManifestFileName(manifestFile);

    ovriren->AddAction("/actions/vtk/in/ShowMenu", false, [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->ToggleShowControls();
      }
    });

    ovriren->AddAction("/actions/vtk/in/ForwardThickCrop", false, [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->Widgets->MoveThickCrops(true);
      }
    });

    ovriren->AddAction("/actions/vtk/in/BackThickCrop", false, [this](vtkEventData* ed) {
      vtkEventDataForDevice* edd = ed->GetAsEventDataForDevice();
      if (edd && edd->GetAction() == vtkEventDataAction::Press)
      {
        this->Widgets->MoveThickCrops(false);
      }
    });
#if PARAVIEW_HAS_OPENXR_SUPPORT
  }
#endif

  this->RenderWindow = static_cast<vtkOpenGLRenderWindow*>(renWin);
  this->XRInterfacePolyfill->SetRenderWindow(renWin);
  this->Renderer = static_cast<vtkOpenGLRenderer*>(ren);

  this->Interactor = vriren;
  static_cast<vtkVRInteractorStyle*>(this->Interactor->GetInteractorStyle())
    ->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_PICK);

  this->AddObserver(vtkCommand::SaveStateEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);
  this->AddObserver(vtkCommand::LoadStateEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);

  // pvRenderWindow is a vtkGenericOpenglRenderWindow

  renWin->SetBaseStationVisibility(this->BaseStationVisibility);
  this->RenderWindow->SetNumberOfLayers(2);
  this->RenderWindow->AddRenderer(this->Renderer);
  this->RenderWindow->SetInteractor(this->Interactor);

  // Setup camera
  renWin->InitializeViewFromCamera(pvRenderer->GetActiveCamera());

  this->Renderer->SetUseImageBasedLighting(pvRenderer->GetUseImageBasedLighting());
  this->Renderer->SetEnvironmentTexture(pvRenderer->GetEnvironmentTexture());

  this->Renderer->RemoveCuller(this->Renderer->GetCullers()->GetLastItem());
  this->Renderer->SetBackground(pvRenderer->GetBackground());
  this->RenderWindow->SetMultiSamples(this->MultiSample ? 8 : 0);

  // we always get first pick on events
  this->Interactor->AddObserver(
    vtkCommand::Select3DEvent, this, &vtkPVXRInterfaceHelper::InteractorEventCallback, 2.0);
  this->Interactor->AddObserver(
    vtkCommand::Move3DEvent, this, &vtkPVXRInterfaceHelper::InteractorEventCallback, 2.0);
  this->Interactor->AddObserver(
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
  this->Interactor->GetInteractorStyle()->AddObserver(
    vtkCommand::EndPickEvent, this, &vtkPVXRInterfaceHelper::EventCallback, 1.0);

  this->RenderWindow->SetDesiredUpdateRate(200.0);
  this->Interactor->SetDesiredUpdateRate(200.0);
  this->Interactor->SetStillUpdateRate(200.0);

  this->RenderWindow->Initialize();
  vtkVRRenderWindow* vrRenWin = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);

  if (vrRenWin && vrRenWin->GetInitialized())
  {
    // Set initial values
    this->SetRightTriggerMode("Pick");
    this->XRInterfaceControls->SetRightTriggerMode("Pick");
    this->XRInterfaceControls->SetCurrentScaleFactor(1);
    this->XRInterfaceControls->SetCurrentMotionFactor(1);
    this->XRInterfaceControls->SetCurrentSavedPosition(-1);

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
    this->RenderWindow->Render();
    this->Renderer->ResetCamera();
    this->Renderer->ResetCameraClippingRange();
    this->ApplyState();

    this->Done = false;
    while (!this->Done)
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
      if (this->SMView && this->LoadLocationValue >= 0)
      {
        this->SMView->StillRender();
        this->LoadLocationState(this->LoadLocationValue);
        this->LoadLocationValue = -1;
        this->SMView->StillRender();
      }
    }
  }

  // disconnect
  this->CollaborationClient->Disconnect();

  // record the last state upon exiting VR
  // so that a later SaveState will have our last
  // recorded data
  this->RecordState();

  this->collabRemoveAllCropPlanes();
  this->collabRemoveAllThickCrops();

  if (this->ObserverWidget)
  {
    this->ObserverWidget->destroy();
    delete this->ObserverWidget;
    this->ObserverWidget = nullptr;
  }

  this->AddedProps->RemoveAllItems();
  this->Widgets->ReleaseGraphicsResources(); // must delete before the interactor
  this->Renderer->Delete();
  this->Renderer = nullptr;
  this->Interactor->Delete();
  this->Interactor = nullptr;
  if (renWin)
  {
    renWin->SetHelperWindow(nullptr);
  }
  this->RenderWindow->Delete();
  this->RenderWindow = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::ResetPositions()
{
  if (!this->Renderer)
  {
    return;
  }

  this->RenderWindow->MakeCurrent();

  vtkCollectionSimpleIterator pit;
  vtkProp* prop;
  for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
  {
    vtkProp3D* prop3d = vtkProp3D::SafeDownCast(prop);
    if (prop3d)
    {
      vtkMatrixToLinearTransform* trans =
        vtkMatrixToLinearTransform::SafeDownCast(prop3d->GetUserTransform());
      if (trans)
      {
        prop3d->GetUserMatrix()->Identity();
      }
    }
  }

  this->DoOneEvent();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::UpdateProps()
{
  if (!this->View)
  {
    return;
  }

  vtkRenderer* pvRenderer = this->View->GetRenderView()->GetRenderer();

  if (!this->Renderer)
  {
    return;
  }

  this->RenderWindow->MakeCurrent();
  if (pvRenderer->GetViewProps()->GetMTime() > this->PropUpdateTime ||
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
    // vtkVolumeCollection* avol = pvRenderer->GetVolumes();
    // vtkVolume* volume;
    // for (avol->InitTraversal(pit); (volume = avol->GetNextVolume(pit));)
    // {
    //   this->AddedProps->AddItem(volume);
    //   this->Renderer->AddVolume(volume);
    // }

    this->PropUpdateTime.Modified();
  }

  this->DoOneEvent();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::Quit()
{
  this->Done = true;
  this->Widgets->Quit();
}

//----------------------------------------------------------------------------
void vtkPVXRInterfaceHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
