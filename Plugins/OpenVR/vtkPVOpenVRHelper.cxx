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
#include "vtkPVOpenVRHelper.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkAssemblyPath.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkCallbackCommand.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCullerCollection.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDistanceRepresentation3D.h"
#include "vtkDistanceWidget.h"
#include "vtkEquirectangularToCubeMapTexture.h"
#include "vtkFlagpoleLabel.h"
#include "vtkGeometryRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInformation.h"
#include "vtkJPEGReader.h"
#include "vtkJPEGWriter.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenVRFollower.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkOpenVRMenuRepresentation.h"
#include "vtkOpenVRMenuWidget.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVROverlay.h"
#include "vtkOpenVROverlayInternal.h"
#include "vtkOpenVRPanelRepresentation.h"
#include "vtkOpenVRPanelWidget.h"
#include "vtkOpenVRRay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPVOpenVRCollaborationClient.h"
#include "vtkPVXMLElement.h"
#include "vtkPlaneSource.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkQWidgetRepresentation.h"
#include "vtkQWidgetTexture.h"
#include "vtkQWidgetWidget.h"
#include "vtkRenderViewBase.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkVectorOperators.h"
#include "vtkWidgetEvent.h"
#include "vtkWindowToImageFilter.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataObjectWriter.h"
#include "vtkXMLUtilities.h"
#include "vtksys/SystemTools.hxx"

#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"

#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRepresentedArrayListDomain.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"

#include "vtkOpenVROverlayInternal.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOpenVRControls.h"
#include "pqPipelineSource.h"

#include <sstream>

#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QModelIndex>

vtkPVOpenVRHelperLocation::vtkPVOpenVRHelperLocation()
{
  this->Pose = new vtkOpenVRCameraPose();
}
vtkPVOpenVRHelperLocation::~vtkPVOpenVRHelperLocation()
{
  delete this->Pose;
}

vtkStandardNewMacro(vtkPVOpenVRHelper);

//----------------------------------------------------------------------------
vtkPVOpenVRHelper::vtkPVOpenVRHelper()
{
  this->Renderer = nullptr;
  this->RenderWindow = nullptr;
  this->Style = nullptr;
  this->Interactor = nullptr;

  this->AddedProps = vtkPropCollection::New();
  this->NavRepresentation->GetTextActor()->GetTextProperty()->SetFontFamilyToCourier();
  this->NavRepresentation->GetTextActor()->GetTextProperty()->SetFrame(0);
  this->NavRepresentation->SetCoordinateSystemToLeftController();

  this->CropSnapping = false;
  this->MultiSample = false;
  this->DefaultCropThickness = 0;

  this->NeedStillRender = false;
  this->LoadLocationValue = -1;

  this->CollaborationClient = vtkPVOpenVRCollaborationClient::New();
  this->CollaborationClient->SetHelper(this);

  this->QWidgetWidget = nullptr;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(this->ImagePlane->GetOutputPort());
  this->ImageActor->GetProperty()->SetAmbient(1.0);
  this->ImageActor->GetProperty()->SetDiffuse(0.0);
  this->ImageActor->SetMapper(mapper);
}

//----------------------------------------------------------------------------
vtkPVOpenVRHelper::~vtkPVOpenVRHelper()
{
  this->AddedProps->Delete();

  this->CollaborationClient->Delete();
  this->CollaborationClient = nullptr;
}

bool vtkPVOpenVRHelper::CollaborationConnect()
{
  if (!this->Renderer)
  {
    return false;
  }

  // add observers to vr rays
  vtkOpenVRModel* cmodel =
    this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::LeftController);
  if (cmodel)
  {
    cmodel->GetRay()->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkPVOpenVRHelper::EventCallback);
  }
  cmodel = this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::RightController);
  if (cmodel)
  {
    cmodel->GetRay()->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkPVOpenVRHelper::EventCallback);
  }

  return this->CollaborationClient->Connect(this->Renderer);
}

bool vtkPVOpenVRHelper::CollaborationDisconnect()
{
  vtkOpenVRModel* cmodel =
    this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::LeftController);
  if (cmodel)
  {
    cmodel->GetRay()->RemoveObservers(vtkCommand::ModifiedEvent);
  }
  cmodel = this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::RightController);
  if (cmodel)
  {
    cmodel->GetRay()->RemoveObservers(vtkCommand::ModifiedEvent);
  }
  return this->CollaborationClient->Disconnect();
}
namespace
{
vtkPVDataRepresentation* FindRepresentation(vtkProp* prop, vtkView* view)
{
  int nr = view->GetNumberOfRepresentations();

  for (int i = 0; i < nr; ++i)
  {
    vtkGeometryRepresentation* gr =
      vtkGeometryRepresentation::SafeDownCast(view->GetRepresentation(i));
    if (gr && gr->GetActor() == prop)
    {
      return gr;
    }
  }
  return NULL;
}
}

void vtkPVOpenVRHelper::ToggleShowControls()
{
  if (!this->QWidgetWidget)
  {
    this->QWidgetWidget = vtkQWidgetWidget::New();
    this->QWidgetWidget->CreateDefaultRepresentation();
    this->QWidgetWidget->SetWidget(this->OpenVRControls);
    this->OpenVRControls->show();
    this->OpenVRControls->resize(960, 700);
    this->QWidgetWidget->SetCurrentRenderer(this->Renderer);
    this->QWidgetWidget->SetInteractor(this->Interactor);
  }

  if (!this->QWidgetWidget->GetEnabled())
  {
    // place widget in front of the viewer, facing them
    double aspect =
      static_cast<double>(this->OpenVRControls->height()) / this->OpenVRControls->width();
    double scale = this->RenderWindow->GetPhysicalScale();

    vtkVector3d camPos;
    this->Renderer->GetActiveCamera()->GetPosition(camPos.GetData());
    vtkVector3d camDOP;
    this->Renderer->GetActiveCamera()->GetDirectionOfProjection(camDOP.GetData());
    vtkVector3d physUp;
    this->RenderWindow->GetPhysicalViewUp(physUp.GetData());

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
    vtkOpenVRModel* ovrmodel =
      this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::RightController);
    ovrmodel->SetShowRay(true);
    ovrmodel->SetRayLength(this->Renderer->GetActiveCamera()->GetClippingRange()[1]);
  }
  else
  {
    this->QWidgetWidget->SetEnabled(0);
    vtkOpenVRModel* ovrmodel =
      this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::RightController);
    ovrmodel->SetShowRay(false);
    this->NeedStillRender = true;
  }
}

void vtkPVOpenVRHelper::SetShowNavigationPanel(bool val)
{
  if ((this->NavWidget->GetEnabled() != 0) == val)
  {
    return;
  }

  if (this->NavWidget->GetEnabled())
  {
    this->NavWidget->SetEnabled(0);
  }
  else
  {
    // add an observer on the left controller to update the bearing and position
    this->NavWidget->SetInteractor(this->Interactor);
    this->NavWidget->SetRepresentation(this->NavRepresentation.Get());
    this->NavRepresentation->SetText("\n Position not updated yet \n");
    double scale = this->RenderWindow->GetPhysicalScale();

    double bnds[6] = { -0.3, 0.3, 0.01, 0.01, -0.01, -0.01 };
    double normal[3] = { 0, 2, 1 };
    double vup[3] = { 0, 1, -2 };
    this->NavRepresentation->PlaceWidgetExtended(bnds, normal, vup, scale);

    this->NavWidget->SetEnabled(1);
  }
}

void vtkPVOpenVRHelper::SetDrawControls(bool val)
{
  this->Style->SetDrawControls(val);
}

void vtkPVOpenVRHelper::SetNumberOfCropPlanes(int count)
{
  if (count == 0)
  {
    this->RemoveAllCropPlanes();
    return;
  }

  if (count < this->CropPlanes.size())
  {
    this->RemoveAllCropPlanes();
  }

  // update will set the real values
  for (int i = static_cast<int>(this->CropPlanes.size()); i < count; ++i)
  {
    double origin[3] = { 0.0, 0.0, 0.0 };
    this->AddACropPlane(origin, origin);
  }
}

void vtkPVOpenVRHelper::UpdateCropPlane(int index, double* origin, double* normal)
{
  int count = 0;
  for (auto const& widget : this->CropPlanes)
  {
    if (count == index)
    {
      vtkImplicitPlaneRepresentation* rep =
        static_cast<vtkImplicitPlaneRepresentation*>(widget->GetRepresentation());
      rep->SetOrigin(origin);
      rep->SetNormal(normal);
      return;
    }
    count++;
  }
}

void vtkPVOpenVRHelper::AddACropPlane(double* origin, double* normal)
{
  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetHandleSize(15.0);
  rep->SetDrawOutline(0);
  rep->GetPlaneProperty()->SetOpacity(0.01);
  rep->ConstrainToWidgetBoundsOff();
  rep->SetCropPlaneToBoundingBox(false);
  rep->SetSnapToAxes(this->CropSnapping);

  // rep->SetPlaceFactor(1.25);
  double* fp = this->Renderer->GetActiveCamera()->GetFocalPoint();
  double scale = this->RenderWindow->GetPhysicalScale();
  double bnds[6] = { fp[0] - scale * 0.5, fp[0] + scale * 0.5, fp[1] - scale * 0.5,
    fp[1] + scale * 0.5, fp[2] - scale * 0.5, fp[2] + scale * 0.5 };
  rep->PlaceWidget(bnds);
  if (origin)
  {
    rep->SetOrigin(origin);
  }
  else
  {
    rep->SetOrigin(fp);
  }
  if (normal)
  {
    rep->SetNormal(normal);
  }
  else
  {
    rep->SetNormal(this->Renderer->GetActiveCamera()->GetDirectionOfProjection());
  }

  vtkNew<vtkImplicitPlaneWidget2> ps;
  this->CropPlanes.insert(ps.Get());
  ps->Register(this);

  ps->SetRepresentation(rep.Get());
  ps->SetInteractor(this->Interactor);
  ps->SetEnabled(1);
  ps->AddObserver(vtkCommand::InteractionEvent, this, &vtkPVOpenVRHelper::EventCallback);

  vtkCollectionSimpleIterator pit;
  vtkProp* prop;
  vtkAssemblyPath* path;
  for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      vtkProp* aProp = path->GetLastNode()->GetViewProp();
      vtkActor* aPart = vtkActor::SafeDownCast(aProp);
      if (aPart)
      {
        if (aPart->GetMapper())
        {
          aPart->GetMapper()->AddClippingPlane(rep->GetUnderlyingPlane());
          continue;
        }
      }
      else
      {
        vtkVolume* aVol = vtkVolume::SafeDownCast(aProp);
        if (aVol)
        {
          if (aVol->GetMapper())
          {
            aVol->GetMapper()->AddClippingPlane(rep->GetUnderlyingPlane());
            continue;
          }
        }
      }
    }
  }
}

void vtkPVOpenVRHelper::RemoveAllCropPlanes()
{
  for (vtkImplicitPlaneWidget2* iter : this->CropPlanes)
  {
    iter->SetEnabled(0);

    vtkImplicitPlaneRepresentation* rep =
      static_cast<vtkImplicitPlaneRepresentation*>(iter->GetRepresentation());

    vtkCollectionSimpleIterator pit;
    vtkProp* prop;
    vtkAssemblyPath* path;
    for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
    {
      for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
      {
        vtkProp* aProp = path->GetLastNode()->GetViewProp();
        vtkActor* aPart = vtkActor::SafeDownCast(aProp);
        if (aPart)
        {
          if (aPart->GetMapper())
          {
            aPart->GetMapper()->RemoveClippingPlane(rep->GetUnderlyingPlane());
            continue;
          }
        }
        else
        {
          vtkVolume* aVol = vtkVolume::SafeDownCast(aProp);
          if (aVol)
          {
            if (aVol->GetMapper())
            {
              aVol->GetMapper()->RemoveClippingPlane(rep->GetUnderlyingPlane());
              continue;
            }
          }
        }
      }
    }

    iter->UnRegister(this);
  }
  this->CropPlanes.clear();

  for (vtkBoxWidget2* iter : this->ThickCrops)
  {
    iter->SetEnabled(0);

    vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(iter->GetRepresentation());

    vtkCollectionSimpleIterator pit;
    vtkProp* prop;
    vtkAssemblyPath* path;
    for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
    {
      for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
      {
        vtkProp* aProp = path->GetLastNode()->GetViewProp();
        vtkActor* aPart = vtkActor::SafeDownCast(aProp);
        if (aPart)
        {
          if (aPart->GetMapper())
          {
            aPart->GetMapper()->RemoveClippingPlane(rep->GetUnderlyingPlane(0));
            aPart->GetMapper()->RemoveClippingPlane(rep->GetUnderlyingPlane(1));
            continue;
          }
        }
        else
        {
          vtkVolume* aVol = vtkVolume::SafeDownCast(aProp);
          if (aVol)
          {
            if (aVol->GetMapper())
            {
              aVol->GetMapper()->RemoveClippingPlane(rep->GetUnderlyingPlane(0));
              aVol->GetMapper()->RemoveClippingPlane(rep->GetUnderlyingPlane(1));
              continue;
            }
          }
        }
      }
    }

    iter->UnRegister(this);
  }
  this->ThickCrops.clear();

  this->CollaborationClient->RemoveAllCropPlanes();
}

void vtkPVOpenVRHelper::AddAThickCrop(vtkTransform* intrans)
{
  vtkNew<vtkBoxRepresentation> rep;
  rep->SetHandleSize(15.0);
  rep->SetTwoPlaneMode(true);
  double bnds[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  rep->PlaceWidget(bnds);

  if (intrans)
  {
    rep->SetTransform(intrans);
  }
  else
  {
    vtkNew<vtkTransform> t;
    double* fp = this->Renderer->GetActiveCamera()->GetFocalPoint();
    double scale = this->RenderWindow->GetPhysicalScale();
    if (this->DefaultCropThickness != 0)
    {
      scale = this->DefaultCropThickness;
    }
    t->Translate(fp);
    t->Scale(scale, scale, scale);
    rep->SetTransform(t);
  }

  rep->SetSnapToAxes(this->CropSnapping);

  vtkNew<vtkBoxWidget2> ps;
  this->ThickCrops.insert(ps.Get());
  ps->Register(this);

  ps->SetRepresentation(rep.Get());
  ps->SetInteractor(this->Interactor);
  ps->SetEnabled(1);

  vtkCollectionSimpleIterator pit;
  vtkProp* prop;
  vtkAssemblyPath* path;
  for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      vtkProp* aProp = path->GetLastNode()->GetViewProp();
      vtkActor* aPart = vtkActor::SafeDownCast(aProp);
      if (aPart)
      {
        if (aPart->GetMapper())
        {
          aPart->GetMapper()->AddClippingPlane(rep->GetUnderlyingPlane(0));
          aPart->GetMapper()->AddClippingPlane(rep->GetUnderlyingPlane(1));
          continue;
        }
      }
      else
      {
        vtkVolume* aVol = vtkVolume::SafeDownCast(aProp);
        if (aVol)
        {
          if (aVol->GetMapper())
          {
            aVol->GetMapper()->AddClippingPlane(rep->GetUnderlyingPlane(0));
            aVol->GetMapper()->AddClippingPlane(rep->GetUnderlyingPlane(1));
            continue;
          }
        }
      }
    }
  }
}

void vtkPVOpenVRHelper::SetCropSnapping(int val)
{
  this->CropSnapping = val;

  for (vtkBoxWidget2* iter : this->ThickCrops)
  {
    vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(iter->GetRepresentation());
    rep->SetSnapToAxes(this->CropSnapping);
  }
  for (vtkImplicitPlaneWidget2* iter : this->CropPlanes)
  {
    vtkImplicitPlaneRepresentation* rep =
      static_cast<vtkImplicitPlaneRepresentation*>(iter->GetRepresentation());
    rep->SetSnapToAxes(this->CropSnapping);
  }
}

void vtkPVOpenVRHelper::AddPointToSource(double const* pnt)
{
  // Get the selected node and try adding the point to that
  pqPipelineSource* psrc = this->OpenVRControls->GetSelectedPipelineSource();
  if (!psrc)
  {
    return;
  }
  vtkSMSourceProxy* source = psrc->getSourceProxy();
  if (!source)
  {
    return;
  }

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
    ptshelper.Set(&(pts[0]), static_cast<unsigned int>(pts.size()));
  }

  source->MarkDirty(source);
  source->UpdateSelfAndAllInputs();
  source->UpdatePipeline();
  if (repr)
  {
    repr->UpdateSelfAndAllInputs();
    repr->UpdatePipeline();
  }
  this->NeedStillRender = true;
}

void vtkPVOpenVRHelper::TakeMeasurement()
{
  this->SetRightTriggerMode("Grab");
  this->OpenVRControls->SetRightTriggerMode("Grab");
  this->ToggleShowControls();
  this->DistanceWidget->SetWidgetStateToStart();
  this->DistanceWidget->SetEnabled(0);
  this->DistanceWidget->SetEnabled(1);
}

void vtkPVOpenVRHelper::RemoveMeasurement()
{
  this->DistanceWidget->SetWidgetStateToStart();
  this->DistanceWidget->SetEnabled(0);
}

void vtkPVOpenVRHelper::SetRightTriggerMode(std::string const& text)
{
  this->RightTriggerMode = text;
  // if (text == "Manipulate Widgets")
  // {
  //   this->GetStyle()->MapInputToAction(
  //     vtkEventDataDevice::RightController,
  //     vtkEventDataDeviceInput::Trigger, VTKIS_NONE);
  // }
  if (text == "Grab")
  {
    this->GetStyle()->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_POSITION_PROP);
  }
  if (text == "Interactive Crop")
  {
    this->GetStyle()->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_CLIP);
  }
  if (text == "Probe")
  {
    this->GetStyle()->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_PICK);
  }
}

bool vtkPVOpenVRHelper::InteractorEventCallback(vtkObject*, unsigned long eventID, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  this->LastEventData = edata;
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();

  if (edd && edd->GetDevice() == vtkEventDataDevice::LeftController &&
    this->NavWidget->GetEnabled() && eventID == vtkCommand::Move3DEvent /* &&
      cam->GetLeftEye() */)
  {
    double pos[4];
    edd->GetWorldPosition(pos);
    pos[3] = 1.0;

    // use scale to control resolution, we want to show about
    // 2mm of resolution
    double scale = this->RenderWindow->GetPhysicalScale();
    double sfactor = pow(10.0, floor(log10(scale * 0.002)));
    pos[0] = floor(pos[0] / sfactor) * sfactor;
    pos[1] = floor(pos[1] / sfactor) * sfactor;
    pos[2] = floor(pos[2] / sfactor) * sfactor;
    std::ostringstream toString;
    toString << std::resetiosflags(std::ios::adjustfield);
    toString << std::setiosflags(std::ios::left);
    toString << setprecision(7);
    toString << "\n Position:\n " << setw(8) << pos[0] << ", " << setw(8) << pos[1] << ", "
             << setw(8) << pos[2] << " \n";

    // compute the bearing, the bearing is the angle in
    // the XY plane
    edd->GetWorldDirection(pos);
    vtkVector3d vup(0, 0, 1);
    vtkVector3d vdir(0, 1, 0);
    vtkVector3d bear(pos);

    // remove any up component
    double upc = vup.Dot(bear);
    if (fabs(upc) < 1.0)
    {
      bear = bear - vup * upc;
      bear.Normalize();

      double theta = acos(bear.Dot(vdir));
      if (vup.Cross(bear).Dot(vdir) < 0.0)
      {
        theta = -theta;
      }
      theta = vtkMath::DegreesFromRadians(theta);
      theta = 0.1 * floor(theta * 10);
      toString << std::fixed;
      toString << setprecision(1);
      toString << " Bearing: " << setw(5) << theta << " East of North \n";
    }
    else
    {
      toString << " Bearing: undefined \n";
    }
    this->NavRepresentation->SetText(toString.str().c_str());
  }

  if (edd && edd->GetDevice() == vtkEventDataDevice::RightController &&
    eventID == vtkCommand::Button3DEvent &&
    edd->GetInput() == vtkEventDataDeviceInput::ApplicationMenu)
  {
    if (edd->GetAction() == vtkEventDataAction::Press)
    {
      this->ToggleShowControls();
    }
    return true;
  }

  // handle right trigger
  if (edd && edd->GetDevice() == vtkEventDataDevice::RightController &&
    eventID == vtkCommand::Button3DEvent && edd->GetInput() == vtkEventDataDeviceInput::Trigger)
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
        this->CollaborationClient->AddPointToSource(pos);
      }
      return true;
    }

    // otherwise let it pass onto someone else to handle
  }

  if (edd && edd->GetDevice() == vtkEventDataDevice::LeftController &&
    eventID == vtkCommand::Button3DEvent && edd->GetInput() == vtkEventDataDeviceInput::TrackPad &&
    edd->GetAction() == vtkEventDataAction::Press)
  {
    const double* tpos = edd->GetTrackPadPosition();
    for (vtkBoxWidget2* iter : this->ThickCrops)
    {
      vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(iter->GetRepresentation());
      vtkErrorMacro("tpad " << tpos[0] << " " << tpos[1]);
      if (tpos[0] + tpos[1] > 0)
      {
        rep->StepForward();
      }
      else
      {
        rep->StepBackward();
      }
    }
  }

  return false;
}

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

void vtkPVOpenVRHelper::SaveState(vtkPVXMLElement* root)
{
  // if we are in VR then RecordState first
  if (this->Interactor)
  {
    this->RecordState();
  }

  root->AddAttribute("PluginVersion", "1.2");
  // ??  OpenVR_Plugin()->GetPluginVersionString());

  // save the locations
  vtkNew<vtkPVXMLElement> e;
  e->SetName("Locations");
  for (auto& loci : this->Locations)
  {
    auto& loc = loci.second;

    vtkOpenVRCameraPose& pose = *loc.Pose;
    if (pose.Loaded)
    {
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
  }
  root->AddNestedElement(e);

  root->AddAttribute("CropSnapping", this->CropSnapping ? 1 : 0);
}

void vtkPVOpenVRHelper::RecordState()
{
  // save the camera poses
}

void vtkPVOpenVRHelper::LoadState(vtkPVXMLElement* e, vtkSMProxyLocator* locator)
{
  this->Locations.clear();

  double version = -1.0;
  e->GetScalarAttribute("PluginVersion", &version);

  if (version < 1.1)
  {
    vtkErrorMacro("State file too old for OpenVRPlugin to load.");
    return;
  }

  if (version > 1.2)
  {
    vtkErrorMacro("State file too recent for OpenVRPlugin to load.");
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

      vtkPVOpenVRHelperLocation& loc = this->Locations[poseNum];

      vtkPVXMLElement* child = locel->FindNestedElementByName("CameraPose");
      if (child)
      {
        auto& pose = *loc.Pose;
        pose.Loaded = true;
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
            std::pair<std::array<double, 3>, std::array<double, 3> >(origin, normal));
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

        vtkPVOpenVRHelperLocation& loc = this->Locations[poseNum];
        auto& pose = *loc.Pose;

        pose.Loaded = true;
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
            std::pair<std::array<double, 3>, std::array<double, 3> >(origin, normal));
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
    this->CropSnapping = (itmp == 0 ? false : true);
  }

  // if we are in VR then applyState
  if (this->Interactor)
  {
    this->ApplyState();
  }
}

void vtkPVOpenVRHelper::ApplyState()
{
  // apply crop snapping setting first
  // this->CropMenu->RenameMenuItem(
  //   "togglesnapping", (this->CropSnapping ? "Turn Snap to Axes Off" : "Turn
  //   Snap to Axes On"));

  // set camera poses
  this->RenderWindow->GetDashboardOverlay()->GetSavedCameraPoses().clear();
  for (auto& loci : this->Locations)
  {
    this->RenderWindow->GetDashboardOverlay()->SetSavedCameraPose(loci.first, loci.second.Pose);
  }
}

void vtkPVOpenVRHelper::ShowBillboard(
  const std::string& text, bool updatePosition, vtkTexture* texture)
{
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  vtkRenderer* ren = this->Renderer;
  if (!renWin || !ren)
  {
    return;
  }

  double orient[3];
  double tpos[3];
  if (updatePosition)
  {
    renWin->UpdateHMDMatrixPose();
    double dop[3];
    ren->GetActiveCamera()->GetDirectionOfProjection(dop);
    double vr[3];
    double* vup = renWin->GetPhysicalViewUp();
    double dtmp[3];
    double vupdot = vtkMath::Dot(dop, vup);
    if (fabs(vupdot) < 0.999)
    {
      dtmp[0] = dop[0] - vup[0] * vupdot;
      dtmp[1] = dop[1] - vup[1] * vupdot;
      dtmp[2] = dop[2] - vup[2] * vupdot;
      vtkMath::Normalize(dtmp);
    }
    else
    {
      renWin->GetPhysicalViewDirection(dtmp);
    }
    vtkMath::Cross(dtmp, vup, vr);
    vtkNew<vtkMatrix4x4> rot;
    for (int i = 0; i < 3; ++i)
    {
      rot->SetElement(0, i, vr[i]);
      rot->SetElement(1, i, vup[i]);
      rot->SetElement(2, i, -dtmp[i]);
    }
    rot->Transpose();
    vtkTransform::GetOrientation(orient, rot);
    this->TextActor3D->SetOrientation(orient);
    this->TextActor3D->RotateX(-30.0);

    double scale = renWin->GetPhysicalScale();
    ren->GetActiveCamera()->GetPosition(tpos);
    tpos[0] += (0.7 * scale * dop[0] - 0.1 * scale * vr[0] - 0.4 * scale * vup[0]);
    tpos[1] += (0.7 * scale * dop[1] - 0.1 * scale * vr[1] - 0.4 * scale * vup[1]);
    tpos[2] += (0.7 * scale * dop[2] - 0.1 * scale * vr[2] - 0.4 * scale * vup[2]);
    this->TextActor3D->SetPosition(tpos);
    // scale should cover 10% of FOV
    double fov = ren->GetActiveCamera()->GetViewAngle();
    double tsize = 0.1 * 2.0 * atan(fov * 0.5); // 10% of fov
    tsize /= 200.0;                             // about 200 pixel texture map
    scale *= tsize;
    this->TextActor3D->SetScale(scale, scale, scale);
  }

  this->TextActor3D->SetInput(text.c_str());
  this->Renderer->AddActor(this->TextActor3D);

  this->TextActor3D->ForceOpaqueOn();
  vtkTextProperty* prop = this->TextActor3D->GetTextProperty();
  prop->SetFrame(1);
  prop->SetFrameColor(1.0, 1.0, 1.0);
  prop->SetBackgroundOpacity(1.0);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(14);

  if (texture)
  {
    texture->Update();
    int* dims = texture->GetInput()->GetDimensions();
    double aspect = static_cast<double>(dims[1]) / dims[0];
    this->ImagePlane->SetOrigin(-1.0, 0.0, 0.0);
    this->ImagePlane->SetPoint1(0.0, 0.0, 0.0);
    this->ImagePlane->SetPoint2(-1.0, aspect, 0.0);

    if (updatePosition)
    {
      this->ImageActor->SetOrientation(orient);
      this->ImageActor->RotateX(-30.0);
      double scale = renWin->GetPhysicalScale();
      this->ImageActor->SetPosition(tpos);
      this->ImageActor->SetScale(scale, scale, scale);
    }
    this->ImageActor->SetTexture(texture);

    this->Renderer->AddActor(this->ImageActor);
    texture->Delete();
  }
}

void vtkPVOpenVRHelper::HideBillboard()
{
  this->Renderer->RemoveActor(this->TextActor3D);
  this->Renderer->RemoveActor(this->ImageActor);
}

void vtkPVOpenVRHelper::MoveToNextImage()
{
  // find the intersection with the image actor
  vtkEventDataDevice3D* edd = this->LastEventData->GetAsEventDataDevice3D();
  if (!edd)
  {
    this->PreviousPickedRepresentation = nullptr;
    this->PreviousPickedDataSet = nullptr;
    return;
  }

  vtkVector3d rpos;
  edd->GetWorldPosition(rpos.GetData());
  vtkVector3d rdir;
  edd->GetWorldDirection(rdir.GetData());

  vtkNew<vtkMatrix4x4> mat4;
  this->ImageActor->GetMatrix(mat4);

  // compute plane basis
  double origin[4]{ -1.0, 0.0, 0.0, 1.0 };
  double originWC[4];
  double point1[4]{ 0.0, 0.0, 0.0, 1.0 };
  double point1WC[4];
  mat4->MultiplyPoint(origin, originWC);
  mat4->MultiplyPoint(point1, point1WC);

  // now we have the points all in WC create a coord system
  // with X being along the X axis of the plane
  // XZ containing the ray position and Y perp
  vtkVector3d ipos(originWC);
  vtkVector3d idir(point1WC);

  vtkVector3d dirx = idir - ipos;
  dirx.Normalize();
  vtkVector3d dirz = rpos - ipos;
  dirz.Normalize();

  vtkVector3d plane = dirz.Cross(dirx);
  plane.Normalize();
  dirz = dirx.Cross(plane);
  dirz.Normalize();

  // remove the Y axis from the ray dir
  rdir = rdir - plane * rdir.Dot(plane);
  rdir.Normalize();

  // compute the intersection point
  double t = -dirz.Dot(rpos - ipos) / rdir.Dot(dirz);
  vtkVector3d intersect = rpos + rdir * t;

  double value = (intersect - ipos).Norm() / (idir - ipos).Norm();

  // now move up or down the list of cells to the next different image
  // this only works for polylines, find the line with the cell, then navigate up or down
  vtkPolyData* pd = vtkPolyData::SafeDownCast(this->LastPickedDataSet);
  if (!pd)
  {
    return;
  }

  std::string lastImage;
  vtkDataSetAttributes* celld = pd->GetCellData();
  vtkIdType cid = this->LastPickedCellId;

  vtkIdType numCells = pd->GetNumberOfCells();
  vtkIdType currCell = cid;
  bool found = false;

  vtkStringArray* sa = nullptr;
  vtkIdType numcd = celld->GetNumberOfArrays();
  for (vtkIdType i = 0; i < numcd; ++i)
  {
    vtkAbstractArray* aa = celld->GetAbstractArray(i);
    if (aa && aa->GetName())
    {
      sa = vtkStringArray::SafeDownCast(aa);
      if (sa)
      {
        // watch for image file names
        std::string svalue = sa->GetValue(currCell);
        if (!strncmp(svalue.c_str(), "file://", 7))
        {
          lastImage = svalue;
          break;
        }
      }
    }
  }

  // now move forward or back to find the next new image
  int step = (value < 0.5 ? -1 : 1);
  while (!found && currCell < numCells && currCell >= 0)
  {
    currCell += step;
    std::string svalue = sa->GetValue(currCell);
    if (!strncmp(svalue.c_str(), "file://", 7) && svalue != lastImage)
    {
      found = true;
    }
  }

  if (!found)
  {
    return;
  }

  this->LastPickedCellId = currCell;
  this->UpdateBillboard(false);
}

void vtkPVOpenVRHelper::MoveToNextCell()
{
  // find the intersection with the image actor
  vtkEventDataDevice3D* edd = this->LastEventData->GetAsEventDataDevice3D();
  if (!edd)
  {
    this->PreviousPickedRepresentation = nullptr;
    this->PreviousPickedDataSet = nullptr;
    return;
  }

  vtkVector3d rpos;
  edd->GetWorldPosition(rpos.GetData());
  vtkVector3d rdir;
  edd->GetWorldDirection(rdir.GetData());

  vtkNew<vtkMatrix4x4> mat4;
  this->TextActor3D->GetMatrix(mat4);

  // compute plane basis
  double origin[4]{ 0.0, 0.0, 0.0, 1.0 };
  double originWC[4];
  double point1[4]{ 200.0, 0.0, 0.0, 1.0 };
  double point1WC[4];
  mat4->MultiplyPoint(origin, originWC);
  mat4->MultiplyPoint(point1, point1WC);

  // now we have the points all in WC create a coord system
  // with X being along the X axis of the plane
  // XZ containing the ray position and Y perp
  vtkVector3d ipos(originWC);
  vtkVector3d idir(point1WC);

  vtkVector3d dirx = idir - ipos;
  dirx.Normalize();
  vtkVector3d dirz = rpos - ipos;
  dirz.Normalize();

  vtkVector3d plane = dirz.Cross(dirx);
  plane.Normalize();
  dirz = dirx.Cross(plane);
  dirz.Normalize();

  // remove the Y axis from the ray dir
  rdir = rdir - plane * rdir.Dot(plane);
  rdir.Normalize();

  // compute the intersection point
  double t = -dirz.Dot(rpos - ipos) / rdir.Dot(dirz);
  vtkVector3d intersect = rpos + rdir * t;

  double value = (intersect - ipos).Norm() / (idir - ipos).Norm();

  // now move up or down the list of cells to the next different image
  // this only works for polylines, find the line with the cell, then navigate up or down
  vtkPolyData* pd = vtkPolyData::SafeDownCast(this->LastPickedDataSet);
  if (!pd)
  {
    return;
  }

  vtkIdType cid = this->LastPickedCellId;
  vtkIdType numCells = pd->GetNumberOfCells();
  vtkIdType currCell = cid;

  // now move forward or back to find the next new image
  int step = (value < 0.5 ? -1 : 1);
  if (currCell + step >= numCells || currCell + step < 0)
  {
    return;
  }

  this->LastPickedCellId = currCell + step;
  this->UpdateBillboard(false);
}

void vtkPVOpenVRHelper::HandlePickEvent(vtkObject*, void* calldata)
{
  this->SelectedCells.clear();
  this->HideBillboard();

  vtkSelection* sel = vtkSelection::SafeDownCast(reinterpret_cast<vtkObjectBase*>(calldata));

  if (!sel || sel->GetNumberOfNodes() == 0)
  {
    this->PreviousPickedRepresentation = nullptr;
    this->PreviousPickedDataSet = nullptr;
    return;
  }

  // for multiple nodes which one do we use?
  vtkSelectionNode* node = sel->GetNode(0);
  vtkProp3D* prop = vtkProp3D::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));

  // if the selection is the image actor?
  if (prop == this->ImageActor)
  {
    this->MoveToNextImage();
    return;
  }

  if (prop == this->TextActor3D)
  {
    this->MoveToNextCell();
    return;
  }

  vtkPVDataRepresentation* repr = FindRepresentation(prop, this->View);
  if (!repr)
  {
    return;
  }
  this->PreviousPickedRepresentation = this->LastPickedRepresentation;
  this->LastPickedRepresentation = repr;

  // next two lines are debugging code to mark what actor was picked
  // by changing its color. Useful to track down picking errors.
  // double *color = static_cast<vtkActor*>(prop)->GetProperty()->GetColor();
  // static_cast<vtkActor*>(prop)->GetProperty()->SetColor(color[0] > 0.0 ?
  // 0.0 : 1.0, 0.5, 0.5);
  vtkDataObject* dobj = repr->GetInput();
  node->GetProperties()->Set(vtkSelectionNode::SOURCE(), repr);

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(dobj);
  vtkDataSet* ds = NULL;
  // handle composite datasets
  if (cds)
  {
    vtkIdType cid = node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX());
    vtkNew<vtkDataObjectTreeIterator> iter;
    iter->SetDataSet(cds);
    iter->SkipEmptyNodesOn();
    iter->SetVisitOnlyLeaves(1);
    iter->InitTraversal();
    while (iter->GetCurrentFlatIndex() != cid && !iter->IsDoneWithTraversal())
    {
      iter->GoToNextItem();
    }
    if (iter->GetCurrentFlatIndex() == cid)
    {
      ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    }
  }
  else
  {
    ds = vtkDataSet::SafeDownCast(dobj);
  }
  if (!ds)
  {
    return;
  }

  // get the picked cell
  vtkIdTypeArray* ids = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
  if (ids == 0)
  {
    return;
  }
  vtkIdType aid = ids->GetComponent(0, 0);

  this->PreviousPickedDataSet = this->LastPickedDataSet;
  this->LastPickedDataSet = ds;
  this->PreviousPickedCellId = this->LastPickedCellId;
  this->LastPickedCellId = aid;
  this->SelectedCells.push_back(aid);
  this->LastPickedProp = prop;
  this->UpdateBillboard(true);
}

void vtkPVOpenVRHelper::UpdateBillboard(bool updatePosition)
{
  vtkCellData* celld = this->LastPickedDataSet->GetCellData();
  vtkIdType aid = this->LastPickedCellId;
  vtkCell* cell = this->LastPickedDataSet->GetCell(aid);

  // update the billboard with information about this data
  std::ostringstream toString;
  double p1[6];
  cell->GetBounds(p1);
  double pos[3] = { 0.5 * (p1[1] + p1[0]), 0.5 * (p1[3] + p1[2]), 0.5 * (p1[5] + p1[4]) };
  toString << "\n Cell Center (DC): " << pos[0] << ", " << pos[1] << ", " << pos[2] << " \n";
  vtkMatrix4x4* pmat = this->LastPickedProp->GetMatrix();
  double wpos[4];
  wpos[0] = pos[0];
  wpos[1] = pos[1];
  wpos[2] = pos[2];
  wpos[3] = 1.0;
  pmat->MultiplyPoint(wpos, wpos);
  toString << " Cell Center (WC): " << wpos[0] << ", " << wpos[1] << ", " << wpos[2] << " \n";

  vtkTexture* texture = nullptr;

  vtkIdType numcd = celld->GetNumberOfArrays();
  for (vtkIdType i = 0; i < numcd; ++i)
  {
    vtkAbstractArray* aa = celld->GetAbstractArray(i);
    if (aa && aa->GetName())
    {
      toString << " " << aa->GetName() << ": ";
      vtkStringArray* sa = vtkStringArray::SafeDownCast(aa);
      if (sa)
      {
        // watch for image file names
        std::string value = sa->GetValue(aid);
        if (!strncmp(value.c_str(), "file://", 7))
        {
          vtkNew<vtkJPEGReader> rdr;
          if (rdr->CanReadFile(value.c_str() + 7))
          {
            rdr->SetFileName(value.c_str() + 7);
            if (!texture)
            {
              texture = vtkTexture::New();
            }
            texture->SetInputConnection(rdr->GetOutputPort());
          }
        }
        toString << value << " \n";
      }
      vtkDataArray* da = vtkDataArray::SafeDownCast(aa);
      if (da)
      {
        int nc = da->GetNumberOfComponents();
        for (int ci = 0; ci < nc; ++ci)
        {
          toString << da->GetComponent(aid, ci) << " ";
        }
        toString << "\n";
      }
    }
  }

  // check to see if we have selected a range of data from
  // the same dataset.
  vtkAbstractArray* holeid = celld->GetAbstractArray("Hole_ID");
  vtkDataArray* fromArray = celld->GetArray("from");
  vtkDataArray* toArray = celld->GetArray("to");
  vtkDataArray* validArray1 = celld->GetArray("vtkCompositingValid");
  vtkDataArray* validArray2 = celld->GetArray("vtkConversionValid");
  if (holeid && fromArray && toArray && validArray1 && validArray2 &&
    this->PreviousPickedRepresentation == this->LastPickedRepresentation &&
    this->PreviousPickedDataSet == this->LastPickedDataSet)
  {
    // ok same dataset, lets see if we have composite results
    vtkVariant hid1 = holeid->GetVariantValue(aid);
    vtkVariant hid2 = holeid->GetVariantValue(this->PreviousPickedCellId);
    if (hid1 == hid2)
    {
      toString << "\n Composite results:\n";
      double totDist = 0;
      double fromEnd = fromArray->GetTuple1(aid);
      double toEnd = toArray->GetTuple1(aid);
      double fromEnd2 = fromArray->GetTuple1(this->PreviousPickedCellId);
      double toEnd2 = toArray->GetTuple1(this->PreviousPickedCellId);
      if (fromEnd2 < fromEnd)
      {
        fromEnd = fromEnd2;
      }
      if (toEnd2 > toEnd)
      {
        toEnd = toEnd2;
      }
      toString << " From: " << fromEnd << " To: " << toEnd << " \n";

      // OK for each cell that is between from and to
      // and part of the same hid, accumulate the numeric data
      for (vtkIdType i = 0; i < numcd; ++i)
      {
        bool insertedCells = false;
        vtkDataArray* da = vtkDataArray::SafeDownCast(celld->GetAbstractArray(i));
        if (da && da->GetName() && strcmp("vtkCompositingValid", da->GetName()) != 0 &&
          strcmp("vtkConversionValid", da->GetName()) != 0 && da != fromArray && da != toArray)
        {
          // for each cell
          totDist = 0;
          int nc = da->GetNumberOfComponents();
          double* result = new double[nc];
          for (int ci = 0; ci < nc; ++ci)
          {
            result[ci] = 0.0;
          }
          for (vtkIdType cidx = 0; cidx < da->GetNumberOfTuples(); ++cidx)
          {
            vtkVariant hid3 = holeid->GetVariantValue(cidx);
            double fromV = fromArray->GetTuple1(cidx);
            double toV = toArray->GetTuple1(cidx);
            double valid1 = validArray1->GetTuple1(cidx);
            double valid2 = validArray2->GetTuple1(cidx);
            if (hid3 == hid1 && fromV >= fromEnd && toV <= toEnd)
            {
              if (!insertedCells)
              {
                this->SelectedCells.push_back(cidx);
              }
              if (valid1 != 0 && valid2 != 0)
              {
                double dist = toV - fromV;
                for (int ci = 0; ci < nc; ++ci)
                {
                  result[ci] += dist * da->GetComponent(cidx, ci);
                }
                totDist += dist;
              }
            }
          }
          insertedCells = true;
          toString << " " << da->GetName() << ": ";
          for (int ci = 0; ci < nc; ++ci)
          {
            toString << result[ci] / totDist << " \n";
          }
        }
      }
      toString << " TotalDistance: " << totDist << " \n";
    }
  }

  toString << "\n";

  this->CollaborationClient->ShowBillboard(toString.str());
  this->ShowBillboard(toString.str(), updatePosition, texture);
  this->Style->ShowPickCell(cell, vtkProp3D::SafeDownCast(this->LastPickedProp));
}

bool vtkPVOpenVRHelper::EventCallback(vtkObject* caller, unsigned long eventID, void* calldata)
{
  // handle different events
  switch (eventID)
  {
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
      this->HandlePickEvent(caller, calldata);
    }
    break;
    case vtkCommand::InteractionEvent:
    {
      vtkImplicitPlaneWidget2* widget = vtkImplicitPlaneWidget2::SafeDownCast(caller);
      if (widget)
      {
        this->CollaborationClient->UpdateCropPlanes(this->CropPlanes);
      }
    }
    break;
    case vtkCommand::ModifiedEvent:
    {
      vtkOpenVRRay* ray = vtkOpenVRRay::SafeDownCast(caller);
      if (ray)
      {
        // find the model
        vtkOpenVRModel* model =
          this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::LeftController);
        if (model && model->GetRay() == ray)
        {
          this->CollaborationClient->UpdateRay(model, vtkEventDataDevice::LeftController);
          return false;
        }
        model = this->RenderWindow->GetTrackedDeviceModel(vtkEventDataDevice::RightController);
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

void vtkPVOpenVRHelper::GoToSavedLocation(int pos, double* collabTrans, double* collabDir)
{
  this->CollaborationClient->SetCurrentLocation(pos);

  this->RenderWindow->GetDashboardOverlay()->LoadCameraPose(pos);
  this->RenderWindow->UpdateHMDMatrixPose();
  this->RenderWindow->SetPhysicalTranslation(collabTrans);
  this->RenderWindow->SetPhysicalViewDirection(collabDir);
  this->RenderWindow->UpdateHMDMatrixPose();
}

void vtkPVOpenVRHelper::LoadLocationState()
{
  int slot = this->LoadLocationValue;
  this->LoadLocationValue = -1;

  auto sdi = this->Locations.find(slot);
  if (sdi == this->Locations.end())
  {
    return;
  }

  this->CollaborationClient->GoToSavedLocation(slot);

  auto& loc = sdi->second;

  // apply navigation panel
  this->SetShowNavigationPanel(loc.NavigationPanelVisibility);

  // load crops
  this->RemoveAllCropPlanes();
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

  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMPVRepresentationProxy* repr =
      vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(i));
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
}

void vtkPVOpenVRHelper::SaveLocationState(int slot)
{
  auto spose = this->RenderWindow->GetDashboardOverlay()->GetSavedCameraPose(slot);
  if (!spose)
  {
    return;
  }

  vtkPVOpenVRHelperLocation& sd = this->Locations[slot];
  *sd.Pose = *spose;
  sd.NavigationPanelVisibility = this->NavWidget->GetEnabled();

  { // regular crops
    sd.CropPlaneStates.clear();
    for (vtkImplicitPlaneWidget2* iter : this->CropPlanes)
    {
      vtkImplicitPlaneRepresentation* rep =
        static_cast<vtkImplicitPlaneRepresentation*>(iter->GetRepresentation());
      std::pair<std::array<double, 3>, std::array<double, 3> > data;
      rep->GetOrigin(data.first.data());
      rep->GetNormal(data.second.data());
      sd.CropPlaneStates.push_back(data);
    }
  }

  { // thick crops
    sd.ThickCropStates.clear();
    for (vtkBoxWidget2* iter : this->ThickCrops)
    {
      vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(iter->GetRepresentation());
      vtkNew<vtkTransform> t;
      rep->GetTransform(t);
      std::array<double, 16> tdata;
      std::copy(t->GetMatrix()->GetData(), t->GetMatrix()->GetData() + 16, tdata.data());
      sd.ThickCropStates.push_back(tdata);
    }
  }

  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMPVRepresentationProxy* repr =
      vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(i));
    vtkSMProperty* prop = repr ? repr->GetProperty("Visibility") : nullptr;
    if (prop)
    {
      sd.Visibility[repr] =
        (vtkSMPropertyHelper(repr, "Visibility").GetAsInt() != 0 ? true : false);
    }
  }
}

void vtkPVOpenVRHelper::ViewRemoved(vtkSMViewProxy* smview)
{
  // if this is not our view then we don't care
  if (this->SMView != smview)
  {
    return;
  }

  this->Quit();
}

void vtkPVOpenVRHelper::ExportLocationsAsSkyboxes(vtkSMViewProxy* smview)
{
  // record the state if we are currently in vr
  if (this->Interactor)
  {
    this->RecordState();
  }

  this->SMView = smview;
  this->View = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());
  vtkRenderer* pvRenderer = this->View->GetRenderView()->GetRenderer();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(1024, 1024);
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(pvRenderer->GetBackground());
  renWin->AddRenderer(ren);
  ren->SetClippingRangeExpansion(0.05);

  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(0.0, 1.0, 0.0);
    light->SetIntensity(1.0);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(0.8, -0.2, 0.0);
    light->SetIntensity(0.8);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(-0.3, -0.2, 0.7);
    light->SetIntensity(0.6);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(-0.3, -0.2, -0.7);
    light->SetIntensity(0.4);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }

  std::string dir = "pv-skybox/";
  vtksys::SystemTools::MakeDirectory(dir);
  ofstream json("pv-skybox/index.json");
  json << "{ \"data\": [ { \"mimeType\": \"image/jpg\","
          "\"pattern\": \"{poseIndex}/{orientation}.jpg\","
          "\"type\": \"blob\", \"name\": \"image\", \"metadata\": {}}], "
          "\"type\": [\"tonic-query-data-model\"],"
          "\"arguments\": { \"poseIndex\": { \"values\": [";

  int count = 0;
  for (auto& loci : this->Locations)
  {
    auto& loc = loci.second;
    if (!loc.Pose->Loaded)
    {
      continue;
    }
    // create subdir for each pose
    std::ostringstream sdir;
    sdir << dir << count;
    vtksys::SystemTools::MakeDirectory(sdir.str());

    this->LoadLocationValue = loci.first;
    this->LoadLocationState();

    auto& camPose = *loc.Pose;

    //    QCoreApplication::processEvents();
    //  this->SMView->StillRender();

    renWin->Render();

    double vright[3];

    vtkMath::Cross(camPose.PhysicalViewDirection, camPose.PhysicalViewUp, vright);

    // now generate the six images, right, left, top, bottom, back, front
    //
    double directions[6][3] = { vright[0], vright[1], vright[2], -1 * vright[0], -1 * vright[1],
      -1 * vright[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], -1 * camPose.PhysicalViewUp[0], -1 * camPose.PhysicalViewUp[1],
      -1 * camPose.PhysicalViewUp[2], -1 * camPose.PhysicalViewDirection[0],
      -1 * camPose.PhysicalViewDirection[1], -1 * camPose.PhysicalViewDirection[2],
      camPose.PhysicalViewDirection[0], camPose.PhysicalViewDirection[1],
      camPose.PhysicalViewDirection[2] };
    double vups[6][3] = { camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], -1 * camPose.PhysicalViewDirection[0],
      -1 * camPose.PhysicalViewDirection[1], -1 * camPose.PhysicalViewDirection[2],
      camPose.PhysicalViewDirection[0], camPose.PhysicalViewDirection[1],
      camPose.PhysicalViewDirection[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2] };

    const char* dirnames[6] = { "right", "left", "up", "down", "back", "front" };

    if (count)
    {
      json << ",";
    }
    json << " \"" << count << "\"";

    vtkCamera* cam = ren->GetActiveCamera();
    cam->SetViewAngle(90);
    cam->SetPosition(camPose.Position);
    // doubel *drange = ren->GetActiveCamera()->GetClippingRange();
    // cam->SetClippingRange(0.2*camPose.Distance, drange);
    double* pos = cam->GetPosition();

    renWin->MakeCurrent();
    // remove prior props
    vtkCollectionSimpleIterator pit;
    ren->RemoveAllViewProps();

    vtkActorCollection* acol = pvRenderer->GetActors();
    vtkActor* actor;
    for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
    {
      actor->ReleaseGraphicsResources(pvRenderer->GetVTKWindow());
      ren->AddActor(actor);
    }

    for (int j = 0; j < 6; ++j)
    {
      // view angle of 90
      cam->SetFocalPoint(pos[0] + camPose.Distance * directions[j][0],
        pos[1] + camPose.Distance * directions[j][1], pos[2] + camPose.Distance * directions[j][2]);
      cam->SetViewUp(vups[j][0], vups[j][1], vups[j][2]);
      ren->ResetCameraClippingRange();
      double* crange = cam->GetClippingRange();
      cam->SetClippingRange(0.2 * camPose.Distance, crange[1]);

      vtkNew<vtkWindowToImageFilter> w2i;
      w2i->SetInput(renWin);
      w2i->SetInputBufferTypeToRGB();
      w2i->ReadFrontBufferOff(); // read from the back buffer
      w2i->Update();

      vtkNew<vtkJPEGWriter> jwriter;
      std::ostringstream filename;
      filename << sdir.str();
      filename << "/" << dirnames[j] << ".jpg";
      jwriter->SetFileName(filename.str().c_str());
      jwriter->SetQuality(75);
      jwriter->SetInputConnection(w2i->GetOutputPort());
      jwriter->Write();
    }
    count++;
  }

  json << "], \"name\": \"poseIndex\" }, "
          "\"orientation\": { \"values\": [\"right\", \"left\", \"up\", "
          "\"down\", \"back\", "
          "\"front\"], "
          "  \"name\": \"orientation\" } }, "
          "\"arguments_order\": [\"orientation\", \"poseIndex\"], "
          "\"metadata\": {\"backgroundColor\": \"rgb(0, 0, 0)\"} }";
}

namespace
{
vtkPolyData* findPolyData(vtkDataObject* input)
{
  // do we have polydata?
  vtkPolyData* pd = vtkPolyData::SafeDownCast(input);
  if (pd)
  {
    return pd;
  }
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
  if (cd)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        return pd;
      }
    }
  }
  return nullptr;
}
}

namespace
{
template <typename T>
void setVectorAttribute(vtkXMLDataElement* el, const char* name, int count, T* data)
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
  el->SetAttribute(name, o.str().c_str());
}

void writeTextureReference(vtkXMLDataElement* adatael, vtkTexture* texture, const char* tname,
  std::map<vtkTexture*, size_t>& textures)
{
  if (texture)
  {
    adatael->SetIntAttribute(tname, static_cast<int>(textures[texture]));
  }
}
}

void vtkPVOpenVRHelper::ExportLocationsAsView(vtkSMViewProxy* smview)
{
  // record the state if we are currently in vr
  if (this->Interactor)
  {
    this->RecordState();
  }

  this->SMView = smview;
  this->View = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());
  vtkRenderer* pvRenderer = this->View->GetRenderView()->GetRenderer();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(1024, 1024);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetClippingRangeExpansion(0.05);

  std::string dir = "pv-view/";
  vtksys::SystemTools::MakeDirectory(dir);

  std::map<vtkActor*, size_t> actors;
  std::map<vtkDataObject*, size_t> dataobjects;
  std::map<vtkTexture*, size_t> textures;

  vtkNew<vtkXMLDataElement> topel;
  topel->SetName("View");
  topel->SetIntAttribute("Version", 2);
  topel->SetIntAttribute("UseImageBasedLighting", pvRenderer->GetUseImageBasedLighting());
  if (pvRenderer->GetEnvironmentCubeMap())
  {
    vtkTexture* cubetex = pvRenderer->GetEnvironmentCubeMap();
    if (textures.find(cubetex) == textures.end())
    {
      textures[cubetex] = textures.size();
    }
    topel->SetIntAttribute("EnvironmentCubeMap", static_cast<int>(textures[cubetex]));
  }

  vtkNew<vtkXMLDataElement> posesel;
  posesel->SetName("CameraPoses");
  int count = 0;
  for (auto& loci : this->Locations)
  {
    auto& loc = loci.second;
    if (!loc.Pose->Loaded)
    {
      continue;
    }

    vtkOpenVRCameraPose& pose = *loc.Pose;

    this->LoadLocationValue = loci.first;
    this->LoadLocationState();

    QCoreApplication::processEvents();
    this->SMView->StillRender();

    vtkNew<vtkXMLDataElement> poseel;
    poseel->SetName("CameraPose");
    poseel->SetIntAttribute("PoseNumber", static_cast<int>(count + 1));
    setVectorAttribute(poseel, "Position", 3, pose.Position);
    poseel->SetDoubleAttribute("Distance", pose.Distance);
    poseel->SetDoubleAttribute("MotionFactor", pose.MotionFactor);
    setVectorAttribute(poseel, "Translation", 3, pose.Translation);
    setVectorAttribute(poseel, "InitialViewUp", 3, pose.PhysicalViewUp);
    setVectorAttribute(poseel, "InitialViewDirection", 3, pose.PhysicalViewDirection);
    setVectorAttribute(poseel, "ViewDirection", 3, pose.ViewDirection);

    vtkCollectionSimpleIterator pit;
    vtkActorCollection* acol = pvRenderer->GetActors();
    vtkActor* actor;
    for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
    {
      vtkNew<vtkXMLDataElement> actorel;
      actorel->SetName("Actor");

      if (!actor->GetVisibility() || !actor->GetMapper() ||
        !actor->GetMapper()->GetInputAlgorithm())
      {
        continue;
      }

      // handle flagpoles in the extra file
      if (vtkFlagpoleLabel::SafeDownCast(actor))
      {
        continue;
      }

      // get the polydata
      actor->GetMapper()->GetInputAlgorithm()->Update();
      vtkPolyData* pd = findPolyData(actor->GetMapper()->GetInputDataObject(0, 0));
      if (!pd || pd->GetNumberOfCells() == 0)
      {
        continue;
      }

      if (actors.find(actor) == actors.end())
      {
        actors[actor] = actors.size();
      }

      actorel->SetIntAttribute("ActorID", static_cast<int>(actors[actor]));
      poseel->AddNestedElement(actorel);
    }

    posesel->AddNestedElement(poseel);
    count++;
  }
  topel->AddNestedElement(posesel);

  // export the actors
  vtkNew<vtkXMLDataElement> actorsel;
  actorsel->SetName("Actors");
  for (auto& ait : actors)
  {
    auto& actor = ait.first;

    // record the textures if there are any
    if (actor->GetTexture() && actor->GetTexture()->GetInput())
    {
      vtkTexture* texture = actor->GetTexture();
      if (textures.find(texture) == textures.end())
      {
        textures[texture] = textures.size();
      }
    }

    // record the property textures if any
    std::map<std::string, vtkTexture*>& tex = actor->GetProperty()->GetAllTextures();
    for (auto& t : tex)
    {
      vtkTexture* texture = t.second;
      if (textures.find(texture) == textures.end())
      {
        textures[texture] = textures.size();
      }
    }

    // record actor properties
    vtkNew<vtkXMLDataElement> adatael;
    adatael->SetName("ActorData");

    // write out texture references
    writeTextureReference(adatael, actor->GetTexture(), "TextureID", textures);
    for (auto& t : tex)
    {
      writeTextureReference(adatael, t.second, t.first.c_str(), textures);
    }

    vtkPolyData* pd = findPolyData(actor->GetMapper()->GetInputDataObject(0, 0));
    // record the polydata if not already done
    if (dataobjects.find(pd) == dataobjects.end())
    {
      dataobjects[pd] = dataobjects.size();
    }
    adatael->SetIntAttribute("PolyDataID", static_cast<int>(dataobjects[pd]));

    adatael->SetIntAttribute("ActorID", static_cast<int>(ait.second));
    adatael->SetAttribute("ClassName", actor->GetClassName());
    adatael->SetVectorAttribute("DiffuseColor", 3, actor->GetProperty()->GetDiffuseColor());
    adatael->SetDoubleAttribute("Diffuse", actor->GetProperty()->GetDiffuse());
    adatael->SetVectorAttribute("AmbientColor", 3, actor->GetProperty()->GetAmbientColor());
    adatael->SetDoubleAttribute("Ambient", actor->GetProperty()->GetAmbient());
    adatael->SetVectorAttribute("SpecularColor", 3, actor->GetProperty()->GetSpecularColor());
    adatael->SetDoubleAttribute("Specular", actor->GetProperty()->GetSpecular());
    adatael->SetDoubleAttribute("SpecularPower", actor->GetProperty()->GetSpecularPower());
    adatael->SetDoubleAttribute("Opacity", actor->GetProperty()->GetOpacity());
    adatael->SetDoubleAttribute("LineWidth", actor->GetProperty()->GetLineWidth());

    adatael->SetIntAttribute("Interpolation", actor->GetProperty()->GetInterpolation());
    adatael->SetDoubleAttribute("Metallic", actor->GetProperty()->GetMetallic());
    adatael->SetDoubleAttribute("Roughness", actor->GetProperty()->GetRoughness());
    adatael->SetDoubleAttribute("NormalScale", actor->GetProperty()->GetNormalScale());
    adatael->SetDoubleAttribute("OcclusionStrength", actor->GetProperty()->GetOcclusionStrength());
    adatael->SetVectorAttribute("EmissiveFactor", 3, actor->GetProperty()->GetEmissiveFactor());

    adatael->SetVectorAttribute("Scale", 3, actor->GetScale());
    setVectorAttribute(adatael, "Position", 3, actor->GetPosition());
    setVectorAttribute(adatael, "Origin", 3, actor->GetOrigin());
    setVectorAttribute(adatael, "Orientation", 3, actor->GetOrientation());

    // scalar visibility
    adatael->SetIntAttribute("ScalarVisibility", actor->GetMapper()->GetScalarVisibility());
    adatael->SetIntAttribute("ScalarMode", actor->GetMapper()->GetScalarMode());

    if (actor->GetMapper()->GetUseLookupTableScalarRange())
    {
      adatael->SetVectorAttribute(
        "ScalarRange", 2, actor->GetMapper()->GetLookupTable()->GetRange());
    }
    else
    {
      setVectorAttribute(adatael, "ScalarRange", 2, actor->GetMapper()->GetScalarRange());
    }
    adatael->SetIntAttribute("ScalarArrayId", actor->GetMapper()->GetArrayId());
    adatael->SetIntAttribute("ScalarArrayAccessMode", actor->GetMapper()->GetArrayAccessMode());
    adatael->SetIntAttribute("ScalarArrayComponent", actor->GetMapper()->GetArrayComponent());
    adatael->SetAttribute("ScalarArrayName", actor->GetMapper()->GetArrayName());

    actorsel->AddNestedElement(adatael);
  }
  topel->AddNestedElement(actorsel);

  // now write the textures
  vtkNew<vtkXMLDataElement> texturesel;
  texturesel->SetName("Textures");
  for (auto const& t : textures)
  {
    vtkNew<vtkXMLDataElement> texel;
    texel->SetName("Texture");
    texel->SetAttribute("ClassName", t.first->GetClassName());
    texel->SetIntAttribute("Repeat", t.first->GetRepeat());
    texel->SetIntAttribute("Interpolate", t.first->GetInterpolate());
    texel->SetIntAttribute("Mipmap", t.first->GetMipmap());
    texel->SetIntAttribute("UseSRGBColorSpace", t.first->GetUseSRGBColorSpace() ? 1 : 0);
    texel->SetDoubleAttribute(
      "MaximumAnisotropicFiltering", t.first->GetMaximumAnisotropicFiltering());
    texel->SetIntAttribute("TextureID", static_cast<int>(t.second));

    vtkImageData* idata = nullptr;
    if (t.first->GetCubeMap())
    {
      vtkEquirectangularToCubeMapTexture* cubetex =
        vtkEquirectangularToCubeMapTexture::SafeDownCast(t.first);
      if (cubetex)
      {
        cubetex->GetInputTexture()->Update();
        idata = cubetex->GetInputTexture()->GetInput();
      }
    }
    else
    {
      t.first->Update();
      idata = vtkImageData::SafeDownCast(t.first->GetInputDataObject(0, 0));
    }

    if (dataobjects.find(idata) == dataobjects.end())
    {
      dataobjects[idata] = dataobjects.size();
    }

    texel->SetIntAttribute("ImageDataID", static_cast<int>(dataobjects[idata]));
    texturesel->AddNestedElement(texel);
  }
  topel->AddNestedElement(texturesel);

  // create subdir for the data
  std::string datadir = dir;
  datadir += "data/";
  vtksys::SystemTools::MakeDirectory(datadir);

  // now write the dataobjects
  vtkNew<vtkXMLDataElement> datasel;
  datasel->SetName("DataObjects");
  for (auto const& dit : dataobjects)
  {
    vtkDataObject* data = dit.first;
    vtkNew<vtkXMLDataElement> datael;
    datael->SetName("DataObject");
    datael->SetIntAttribute("DataObjectType", data->GetDataObjectType());
    datael->SetIntAttribute("MemorySize", data->GetActualMemorySize());
    datael->SetIntAttribute("DataObjectID", static_cast<int>(dit.second));
    datasel->AddNestedElement(datael);

    std::ostringstream sdir;
    sdir << "data" << dit.second;
    switch (data->GetDataObjectType())
    {
      case VTK_IMAGE_DATA:
        sdir << ".vti";
        break;
      case VTK_POLY_DATA:
        sdir << ".vtp";
        break;
    }
    std::string fileName = "data/" + sdir.str();
    datael->SetAttribute("FileName", fileName.c_str());
    vtkNew<vtkXMLDataObjectWriter> writer;
    writer->SetDataModeToAppended();
    writer->SetCompressorTypeToLZ4();
    writer->EncodeAppendedDataOff();
    fileName = datadir + sdir.str();
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(data);
    writer->Write();
  }
  topel->AddNestedElement(datasel);

  vtkIndent indent;
  vtkXMLUtilities::WriteElementToFile(topel, "pv-view/index.mvx", &indent);

  // create empty extra.xml file
  vtkNew<vtkXMLDataElement> topel2;
  topel2->SetName("View");
  topel2->SetAttribute("ViewImage", "Filename.jpg");
  topel2->SetAttribute("Longitude", "0.0");
  topel2->SetAttribute("Latitude", "0.0");

  // write out flagpoles
  vtkNew<vtkXMLDataElement> fsel;
  fsel->SetName("Flagpoles");
  vtkCollectionSimpleIterator pit;
  vtkActorCollection* acol = pvRenderer->GetActors();
  vtkActor* actor;
  for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
  {
    vtkFlagpoleLabel* flag = vtkFlagpoleLabel::SafeDownCast(actor);

    if (!flag || !actor->GetVisibility())
    {
      continue;
    }

    vtkNew<vtkXMLDataElement> flagel;
    flagel->SetName("Flagpole");
    flagel->SetAttribute("Label", flag->GetInput());
    flagel->SetVectorAttribute("Position", 3, flag->GetBasePosition());
    flagel->SetDoubleAttribute("Height",
      sqrt(vtkMath::Distance2BetweenPoints(flag->GetTopPosition(), flag->GetBasePosition())));

    fsel->AddNestedElement(flagel);
  }
  topel2->AddNestedElement(fsel);

  vtkNew<vtkXMLDataElement> psel;
  psel->SetName("PhotoSpheres");
  vtkNew<vtkXMLDataElement> cpel;
  cpel->SetName("CameraPoses");
  topel2->AddNestedElement(psel);
  topel2->AddNestedElement(cpel);
  vtkXMLUtilities::WriteElementToFile(topel2, "pv-view/extra.xml", &indent);
}

void vtkPVOpenVRHelper::SendToOpenVR(vtkSMViewProxy* smview)
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

  vtkRenderer* pvRenderer = this->View->GetRenderView()->GetRenderer();
  vtkRenderWindow* pvRenderWindow = vtkRenderWindow::SafeDownCast(pvRenderer->GetVTKWindow());

  if (!this->RenderWindow)
  {
    this->RenderWindow = vtkOpenVRRenderWindow::New();
    this->RenderWindow->SetNumberOfLayers(2);
    // test out sharing the context
    this->RenderWindow->SetHelperWindow(static_cast<vtkOpenGLRenderWindow*>(pvRenderWindow));
  }

  this->Renderer = vtkOpenVRRenderer::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Interactor = vtkOpenVRRenderWindowInteractor::New();
  this->RenderWindow->SetInteractor(this->Interactor);

  this->Renderer->SetUseImageBasedLighting(pvRenderer->GetUseImageBasedLighting());
  this->Renderer->SetEnvironmentCubeMap(pvRenderer->GetEnvironmentCubeMap());

  // required for LOD volume rendering
  // iren->SetDesiredUpdateRate(220.0);
  // iren->SetStillUpdateRate(220.0);
  // renWin->SetDesiredUpdateRate(220.0);

  // add a pick observer
  this->Style = vtkOpenVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle());
  this->Style->AddObserver(vtkCommand::EndPickEvent, this, &vtkPVOpenVRHelper::EventCallback, 1.0);

  // we always get first pick on events
  this->Interactor->AddObserver(
    vtkCommand::Button3DEvent, this, &vtkPVOpenVRHelper::InteractorEventCallback, 2.0);
  this->Interactor->AddObserver(
    vtkCommand::Move3DEvent, this, &vtkPVOpenVRHelper::InteractorEventCallback, 2.0);

  this->RenderWindow->GetDashboardOverlay()->AddObserver(
    vtkCommand::SaveStateEvent, this, &vtkPVOpenVRHelper::EventCallback, 1.0);
  this->RenderWindow->GetDashboardOverlay()->AddObserver(
    vtkCommand::LoadStateEvent, this, &vtkPVOpenVRHelper::EventCallback, 1.0);

  this->Style->MapInputToAction(
    vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_PICK);

  this->Renderer->RemoveCuller(this->Renderer->GetCullers()->GetLastItem());
  this->Renderer->SetBackground(pvRenderer->GetBackground());

  this->RenderWindow->SetDesiredUpdateRate(200.0);
  this->Interactor->SetDesiredUpdateRate(200.0);
  this->Interactor->SetStillUpdateRate(200.0);

  this->DistanceWidget = vtkDistanceWidget::New();
  this->DistanceWidget->SetInteractor(this->Interactor);
  vtkNew<vtkDistanceRepresentation3D> drep;
  this->DistanceWidget->SetRepresentation(drep.Get());
  vtkNew<vtkOpenVRFollower> fol;
  drep->SetLabelActor(fol.Get());
  vtkNew<vtkPointHandleRepresentation3D> hr;
  drep->SetHandleRepresentation(hr.Get());
  hr->SetHandleSize(40);

  drep->SetLabelFormat("Dist: %g\ndeltaX: %g\ndeltaY: %g\ndeltaZ: %g");
  // create 4 lights for even lighting
  pvRenderer->GetLights()->RemoveAllItems();
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(0.0, 1.0, 0.0);
    light->SetIntensity(1.0);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(0.8, -0.2, 0.0);
    light->SetIntensity(0.8);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(-0.3, -0.2, 0.7);
    light->SetIntensity(0.6);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(-0.3, -0.2, -0.7);
    light->SetIntensity(0.4);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light);
    pvRenderer->AddLight(light);
    light->Delete();
  }

#if 1
  vtkNew<vtkCamera> zcam;
  double fp[3];
  pvRenderer->GetActiveCamera()->GetFocalPoint(fp);
  zcam->SetFocalPoint(fp);
  zcam->SetViewAngle(pvRenderer->GetActiveCamera()->GetViewAngle());
  zcam->SetViewUp(0.0, 0.0, 1.0);
  double distance = pvRenderer->GetActiveCamera()->GetDistance();
  zcam->SetPosition(fp[0] + distance, fp[1], fp[2]);
  this->RenderWindow->InitializeViewFromCamera(zcam.Get());
#else
  renWin->InitializeViewFromCamera(pvRenderer->GetActiveCamera());
#endif

  this->RenderWindow->SetMultiSamples(this->MultiSample ? 8 : 0);

  this->RenderWindow->Initialize();

  this->SetRightTriggerMode("Probe");
  this->OpenVRControls->SetRightTriggerMode("Probe");

  if (this->RenderWindow->GetHMD())
  {
    this->UpdateProps();
    this->RenderWindow->Render();
    this->Renderer->ResetCamera();
    this->Renderer->ResetCameraClippingRange();
    this->ApplyState();
    while (this->Interactor && !this->Interactor->GetDone())
    {
      auto state = this->RenderWindow->GetState();
      state->Initialize(this->RenderWindow);
      this->Interactor->DoOneEvent(this->RenderWindow, this->Renderer);
      this->CollaborationClient->Render();
      QCoreApplication::processEvents();
      if (this->NeedStillRender)
      {
        this->SMView->StillRender();
        this->NeedStillRender = false;
      }
      if (this->LoadLocationValue >= 0)
      {
        this->SMView->StillRender();
        this->LoadLocationState();
        this->SMView->StillRender();
      }
    }
  }

  // record the last state upon exiting VR
  // so that a later SaveState will have our last
  // recorded data
  this->RecordState();

  this->RemoveAllCropPlanes();

  // disconnect
  this->CollaborationClient->Disconnect();

  this->AddedProps->RemoveAllItems();
  this->DistanceWidget->Delete(); // must delete before the interactor
  this->Renderer->Delete();
  this->Renderer = nullptr;
  this->Interactor->Delete();
  this->Interactor = nullptr;
  this->RenderWindow->SetHelperWindow(nullptr);
  this->RenderWindow->Delete();
  this->RenderWindow = nullptr;
}

void vtkPVOpenVRHelper::UpdateProps()
{
  if (!this->Renderer)
  {
    return;
  }

  this->RenderWindow->MakeCurrent();
  vtkRenderer* pvRenderer = this->View->GetRenderView()->GetRenderer();
  if (pvRenderer->GetViewProps()->GetMTime() > this->PropUpdateTime ||
    this->AddedProps->GetNumberOfItems() == 0)
  {
    // remove prior props
    vtkCollectionSimpleIterator pit;
    vtkProp* prop;
    for (this->AddedProps->InitTraversal(pit); (prop = this->AddedProps->GetNextProp(pit));)
    {
      this->Renderer->RemoveViewProp(prop);
    }

    vtkPropCollection* pcol = pvRenderer->GetViewProps();
    vtkActor* actor;
    this->AddedProps->RemoveAllItems();
    for (pcol->InitTraversal(pit); (prop = pcol->GetNextProp(pit));)
    {
      this->AddedProps->AddItem(prop);
      actor = vtkActor::SafeDownCast(prop);
      if (actor)
      {
        // force opaque is opacity is 1.0
        if (actor->GetProperty()->GetOpacity() >= 1.0)
        {
          actor->ForceOpaqueOn();
        }
        else
        {
          actor->ForceOpaqueOff();
        }
      }
      this->Renderer->AddViewProp(prop);
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

  this->Interactor->DoOneEvent(this->RenderWindow, this->Renderer);
}

void vtkPVOpenVRHelper::Quit()
{
  if (this->Interactor)
  {
    this->Interactor->TerminateApp();
  }
}

//----------------------------------------------------------------------------
void vtkPVOpenVRHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
