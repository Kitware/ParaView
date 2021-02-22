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
#include "vtkPVOpenVRWidgets.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkAssemblyPath.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDistanceRepresentation3D.h"
#include "vtkDistanceWidget.h"
#include "vtkGeometryRepresentation.h"
#include "vtkImageData.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInformation.h"
#include "vtkJPEGReader.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenVRFollower.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkOpenVRPanelRepresentation.h"
#include "vtkOpenVRPanelWidget.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLODActor.h"
#include "vtkPVOpenVRCollaborationClient.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkPVRenderView.h"
#include "vtkPlaneSource.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkVectorOperators.h"
#include "vtkView.h"
#include "vtkVolume.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

vtkStandardNewMacro(vtkPVOpenVRWidgets);

#ifdef OPENVR_HAS_IMAGO_SUPPORT
#include "vtkPVImagoLoader.cxx"
#else
class vtkImagoLoader
{
public:
  bool GetHttpImage(std::string const&, std::future<vtkImageData*>&) { return true; }
  bool GetImage(std::string const&, std::future<vtkImageData*>&) { return true; }
  bool Login(std::string const&, std::string const&) { return false; }
  bool IsLoggedIn() { return false; }

  void SetWorkspace(std::string const&) {}
  void SetDataset(std::string const&) {}
  void SetImageryType(std::string const&) {}
  void SetImageType(std::string const&) {}

  void GetWorkspaces(std::vector<std::string>&) {}
  void GetDatasets(std::vector<std::string>&) {}
  void GetImageryTypes(std::vector<std::string>&) {}
  void GetImageTypes(std::vector<std::string>&) {}
};
#endif

//----------------------------------------------------------------------------
vtkPVOpenVRWidgets::vtkPVOpenVRWidgets()
{
  this->NavRepresentation->GetTextActor()->GetTextProperty()->SetFontFamilyToCourier();
  this->NavRepresentation->GetTextActor()->GetTextProperty()->SetFrame(0);
  this->NavRepresentation->SetCoordinateSystemToLeftController();

  this->DistanceWidget = vtkDistanceWidget::New();
  vtkNew<vtkDistanceRepresentation3D> drep;
  this->DistanceWidget->SetRepresentation(drep.Get());
  vtkNew<vtkOpenVRFollower> fol;
  drep->SetLabelActor(fol.Get());
  vtkNew<vtkPointHandleRepresentation3D> hr;
  drep->SetHandleRepresentation(hr.Get());
  hr->SetHandleSize(40);

  drep->SetLabelFormat("Dist: %g\ndeltaX: %g\ndeltaY: %g\ndeltaZ: %g");

  this->DefaultCropThickness = 0;
  this->CropSnapping = false;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(this->ImagePlane->GetOutputPort());
  this->ImageActor->GetProperty()->SetAmbient(1.0);
  this->ImageActor->GetProperty()->SetDiffuse(0.0);
  this->ImageActor->SetMapper(mapper);
  // setup an observer to check if the texture is loaded yet
  this->WaitingForImage = false;

  this->LastPickedDataSet = nullptr;
  this->LastPickedRepresentation = nullptr;
  this->PreviousPickedDataSet = nullptr;
  this->PreviousPickedRepresentation = nullptr;

#ifdef OPENVR_HAS_IMAGO_SUPPORT
  this->ImagoLoader = new vtkImagoLoader();
#endif
}

vtkPVOpenVRWidgets::~vtkPVOpenVRWidgets()
{
#ifdef OPENVR_HAS_IMAGO_SUPPORT
  delete this->ImagoLoader;
#endif
  this->DistanceWidget->Delete();
}

bool vtkPVOpenVRWidgets::LoginToImago(std::string const& uid, std::string const& pw)
{
  return this->ImagoLoader->Login(uid, pw);
}

void vtkPVOpenVRWidgets::SetImagoWorkspace(std::string val)
{
  this->ImagoLoader->SetWorkspace(val);
}
void vtkPVOpenVRWidgets::SetImagoDataset(std::string val)
{
  this->ImagoLoader->SetDataset(val);
}
void vtkPVOpenVRWidgets::SetImagoImageryType(std::string val)
{
  this->ImagoLoader->SetImageryType(val);
}
void vtkPVOpenVRWidgets::SetImagoImageType(std::string val)
{
  this->ImagoLoader->SetImageType(val);
}

void vtkPVOpenVRWidgets::GetImagoWorkspaces(std::vector<std::string>& vals)
{
  this->ImagoLoader->GetWorkspaces(vals);
}
void vtkPVOpenVRWidgets::GetImagoDatasets(std::vector<std::string>& vals)
{
  this->ImagoLoader->GetDatasets(vals);
}
void vtkPVOpenVRWidgets::GetImagoImageryTypes(std::vector<std::string>& vals)
{
  this->ImagoLoader->GetImageryTypes(vals);
}
void vtkPVOpenVRWidgets::GetImagoImageTypes(std::vector<std::string>& vals)
{
  this->ImagoLoader->GetImageTypes(vals);
}

void vtkPVOpenVRWidgets::SetLastEventData(vtkEventData* edd)
{
  this->LastEventData = edd;
}

void vtkPVOpenVRWidgets::SetHelper(vtkPVOpenVRHelper* val)
{
  this->Helper = val;
}

bool vtkPVOpenVRWidgets::EventCallback(vtkObject* caller, unsigned long eventID, void*)
{
  // handle different events
  switch (eventID)
  {
    case vtkCommand::InteractionEvent:
    {
      {
        vtkImplicitPlaneWidget2* widget = vtkImplicitPlaneWidget2::SafeDownCast(caller);
        if (widget)
        {
          for (size_t i = 0; i < this->CropPlanes.size(); ++i)
          {
            if (this->CropPlanes[i] == widget)
            {
              this->Helper->GetCollaborationClient()->UpdateCropPlane(i, this->CropPlanes[i]);
              return false;
            }
          }
        }
      }
      {
        vtkBoxWidget2* widget = vtkBoxWidget2::SafeDownCast(caller);
        if (widget)
        {
          for (size_t i = 0; i < this->ThickCrops.size(); ++i)
          {
            if (this->ThickCrops[i] == widget)
            {
              this->Helper->GetCollaborationClient()->UpdateThickCrop(i, this->ThickCrops[i]);
              return false;
            }
          }
        }
      }
    }
    break;
  }
  return false;
}

void vtkPVOpenVRWidgets::ReleaseGraphicsResources()
{
  this->DistanceWidget->SetInteractor(nullptr);
}

void vtkPVOpenVRWidgets::SetShowNavigationPanel(bool val, vtkOpenGLRenderWindow* renderWindow)
{
  // ignored for simulated VR
  auto ovr_rw = vtkOpenVRRenderWindow::SafeDownCast(renderWindow);
  if (!ovr_rw)
  {
    return;
  }

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
    this->NavWidget->SetInteractor(renderWindow->GetInteractor());
    this->NavWidget->SetRepresentation(this->NavRepresentation.Get());
    this->NavRepresentation->SetText("\n Position not updated yet \n");
    double scale = ovr_rw->GetPhysicalScale();

    double bnds[6] = { -0.3, 0.3, 0.01, 0.01, -0.01, -0.01 };
    double normal[3] = { 0, 2, 1 };
    double vup[3] = { 0, 1, -2 };
    this->NavRepresentation->PlaceWidgetExtended(bnds, normal, vup, scale);

    this->NavWidget->SetEnabled(1);
  }
}

bool vtkPVOpenVRWidgets::GetNavigationPanelVisibility()
{
  return this->NavWidget->GetEnabled();
}

void vtkPVOpenVRWidgets::UpdateNavigationText(
  vtkEventDataDevice3D* edd, vtkOpenGLRenderWindow* renderWindow)
{
  double pos[4];
  edd->GetWorldPosition(pos);
  pos[3] = 1.0;

  // use scale to control resolution, we want to show about
  // 2mm of resolution
  double scale = 1.0;
  auto ovr_rw = vtkOpenVRRenderWindow::SafeDownCast(renderWindow);
  if (ovr_rw)
  {
    scale = ovr_rw->GetPhysicalScale();
  }
  double sfactor = pow(10.0, floor(log10(scale * 0.002)));
  pos[0] = floor(pos[0] / sfactor) * sfactor;
  pos[1] = floor(pos[1] / sfactor) * sfactor;
  pos[2] = floor(pos[2] / sfactor) * sfactor;
  std::ostringstream toString;
  toString << std::resetiosflags(std::ios::adjustfield);
  toString << std::setiosflags(std::ios::left);
  toString << setprecision(7);
  toString << "\n Position:\n " << setw(8) << pos[0] << ", " << setw(8) << pos[1] << ", " << setw(8)
           << pos[2] << " \n";

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

void vtkPVOpenVRWidgets::TakeMeasurement(vtkOpenGLRenderWindow* renWin)
{
  this->DistanceWidget->SetInteractor(renWin->GetInteractor());
  this->DistanceWidget->SetWidgetStateToStart();
  this->DistanceWidget->SetEnabled(0);
  this->DistanceWidget->SetEnabled(1);
}

void vtkPVOpenVRWidgets::RemoveMeasurement()
{
  this->DistanceWidget->SetWidgetStateToStart();
  this->DistanceWidget->SetEnabled(0);
}

void vtkPVOpenVRWidgets::AddACropPlane(double* origin, double* normal)
{
  size_t i = this->CropPlanes.size();
  this->collabAddACropPlane(origin, normal);
  this->Helper->GetCollaborationClient()->UpdateCropPlane(i, this->CropPlanes[i]);
}

void vtkPVOpenVRWidgets::collabAddACropPlane(double* origin, double* normal)
{
  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetHandleSize(15.0);
  rep->SetDrawOutline(0);
  rep->GetPlaneProperty()->SetOpacity(0.01);
  rep->ConstrainToWidgetBoundsOff();
  rep->SetCropPlaneToBoundingBox(false);
  rep->SetSnapToAxes(this->CropSnapping);

  // rep->SetPlaceFactor(1.25);
  auto* ren = this->Helper->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());

  double* fp = ren->GetActiveCamera()->GetFocalPoint();
  double scale = 1.0;

  auto ovr_rw = vtkOpenVRRenderWindow::SafeDownCast(renWin);
  if (ovr_rw)
  {
    scale = ovr_rw->GetPhysicalScale();
  }
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
    rep->SetNormal(ren->GetActiveCamera()->GetDirectionOfProjection());
  }

  vtkNew<vtkImplicitPlaneWidget2> ps;
  this->CropPlanes.push_back(ps.Get());
  ps->Register(this);

  ps->SetRepresentation(rep.Get());
  ps->SetInteractor(renWin->GetInteractor());
  ps->SetEnabled(1);
  ps->AddObserver(vtkCommand::InteractionEvent, this, &vtkPVOpenVRWidgets::EventCallback);

  vtkCollectionSimpleIterator pit;
  vtkProp* prop;
  vtkAssemblyPath* path;
  for (this->Helper->GetAddedProps()->InitTraversal(pit);
       (prop = this->Helper->GetAddedProps()->GetNextProp(pit));)
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

void vtkPVOpenVRWidgets::collabRemoveAllCropPlanes()
{
  for (vtkImplicitPlaneWidget2* iter : this->CropPlanes)
  {
    iter->SetEnabled(0);

    vtkImplicitPlaneRepresentation* rep =
      static_cast<vtkImplicitPlaneRepresentation*>(iter->GetRepresentation());

    vtkCollectionSimpleIterator pit;
    vtkProp* prop;
    vtkAssemblyPath* path;
    for (this->Helper->GetAddedProps()->InitTraversal(pit);
         (prop = this->Helper->GetAddedProps()->GetNextProp(pit));)
    {
      for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
      {
        vtkProp* aProp = path->GetLastNode()->GetViewProp();
        vtkActor* aPart = vtkActor::SafeDownCast(aProp);
        if (aPart)
        {
          if (aPart->GetMapper() && aPart->GetMapper()->GetClippingPlanes())
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
}

void vtkPVOpenVRWidgets::collabRemoveAllThickCrops()
{
  for (vtkBoxWidget2* iter : this->ThickCrops)
  {
    iter->SetEnabled(0);

    vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(iter->GetRepresentation());

    vtkCollectionSimpleIterator pit;
    vtkProp* prop;
    vtkAssemblyPath* path;
    for (this->Helper->GetAddedProps()->InitTraversal(pit);
         (prop = this->Helper->GetAddedProps()->GetNextProp(pit));)
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
}

void vtkPVOpenVRWidgets::AddAThickCrop(vtkTransform* intrans)
{
  size_t i = this->ThickCrops.size();
  this->collabAddAThickCrop(intrans);
  this->Helper->GetCollaborationClient()->UpdateThickCrop(i, this->ThickCrops[i]);
}

void vtkPVOpenVRWidgets::collabAddAThickCrop(vtkTransform* intrans)
{
  vtkNew<vtkBoxRepresentation> rep;
  rep->SetHandleSize(15.0);
  rep->SetTwoPlaneMode(true);
  double bnds[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  rep->PlaceWidget(bnds);

  auto* ren = this->Helper->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());

  if (intrans)
  {
    rep->SetTransform(intrans);
  }
  else
  {
    vtkNew<vtkTransform> t;
    double* fp = ren->GetActiveCamera()->GetFocalPoint();
    double scale = this->DefaultCropThickness;
    auto ovr_rw = vtkOpenVRRenderWindow::SafeDownCast(renWin);
    if (ovr_rw && this->DefaultCropThickness == 0)
    {
      scale = ovr_rw->GetPhysicalScale();
    }
    t->Translate(fp);
    t->Scale(scale, scale, scale);
    rep->SetTransform(t);
  }

  rep->SetSnapToAxes(this->CropSnapping);

  vtkNew<vtkBoxWidget2> ps;
  this->ThickCrops.push_back(ps.Get());
  ps->Register(this);

  ps->SetRepresentation(rep.Get());
  ps->SetInteractor(renWin->GetInteractor());
  ps->SetEnabled(1);
  ps->AddObserver(vtkCommand::InteractionEvent, this, &vtkPVOpenVRWidgets::EventCallback);

  vtkCollectionSimpleIterator pit;
  vtkProp* prop;
  vtkAssemblyPath* path;
  for (this->Helper->GetAddedProps()->InitTraversal(pit);
       (prop = this->Helper->GetAddedProps()->GetNextProp(pit));)
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

void vtkPVOpenVRWidgets::collabUpdateCropPlane(int index, double* origin, double* normal)
{
  if (index >= this->CropPlanes.size())
  {
    this->collabAddACropPlane(origin, normal);
    return;
  }

  int count = 0;
  for (auto const& widget : this->CropPlanes)
  {
    if (count == index)
    {
      vtkImplicitPlaneRepresentation* rep =
        static_cast<vtkImplicitPlaneRepresentation*>(widget->GetRepresentation());
      rep->SetNormal(normal);
      rep->SetOrigin(origin);
      return;
    }
    count++;
  }
}

void vtkPVOpenVRWidgets::collabUpdateThickCrop(int index, double* matrix)
{
  if (index >= this->ThickCrops.size())
  {
    vtkNew<vtkTransform> t;
    t->SetMatrix(matrix);
    this->collabAddAThickCrop(t);
    return;
  }

  int count = 0;
  for (auto const& widget : this->ThickCrops)
  {
    if (count == index)
    {
      vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(widget->GetRepresentation());
      vtkNew<vtkTransform> t;
      t->SetMatrix(matrix);
      rep->SetTransform(t);
      return;
    }
    count++;
  }
}

void vtkPVOpenVRWidgets::SetCropSnapping(int val)
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

void vtkPVOpenVRWidgets::SaveLocationState(vtkPVOpenVRHelperLocation& sd)
{
  sd.NavigationPanelVisibility = this->GetNavigationPanelVisibility();

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
}

void vtkPVOpenVRWidgets::MoveThickCrops(bool forward)
{
  for (auto i = 0; i < this->ThickCrops.size(); ++i)
  {
    vtkBoxRepresentation* rep =
      static_cast<vtkBoxRepresentation*>(this->ThickCrops[i]->GetRepresentation());
    if (forward)
    {
      rep->StepForward();
    }
    else
    {
      rep->StepBackward();
    }
    this->Helper->GetCollaborationClient()->UpdateThickCrop(i, this->ThickCrops[i]);
  }
}

// used for async texture loading
void vtkPVOpenVRWidgets::UpdateTexture()
{
  if (this->WaitingForImage && this->ImageFuture.valid() &&
    this->ImageFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
  {
    vtkOpenGLRenderer* ren = this->Helper->GetRenderer();
    ren->RemoveObserver(this->RenderObserver);
    this->WaitingForImage = false;
    vtkImageData* id = this->ImageFuture.get();
    if (id)
    {
      vtkNew<vtkTexture> texture;
      texture->InterpolateOn();
      texture->MipmapOn();
      texture->SetInputData(id);
      id->UnRegister(nullptr);
      // use constant area of 0.5 m^2
      int* dims = id->GetDimensions();
      double xsize = sqrt(0.5 * dims[0] / dims[1]);
      double ysize = 0.5 / xsize;
      this->ImagePlane->SetOrigin(-xsize - 0.01, 0.0, 0.0);
      this->ImagePlane->SetPoint1(-0.01, 0.0, 0.0);
      this->ImagePlane->SetPoint2(-xsize - 0.01, ysize, 0.0);
      this->ImageActor->SetTexture(texture);
    }
  }
}

void vtkPVOpenVRWidgets::ShowBillboard(
  const std::string& text, bool updatePosition, std::string const& textureFile)
{
  vtkOpenGLRenderer* ren = this->Helper->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());
  if (!renWin || !ren)
  {
    return;
  }

  vtkOpenVRRenderWindow* ovr = vtkOpenVRRenderWindow::SafeDownCast(renWin);

  double orient[3];
  double tpos[3];
  if (updatePosition)
  {
    double vr[3];
    double* vup;
    double scale = 1.0;
    if (ovr)
    {
      ovr->UpdateHMDMatrixPose();
      vup = ovr->GetPhysicalViewUp();
      scale = ovr->GetPhysicalScale();
    }
    else
    {
      vup = ren->GetActiveCamera()->GetViewUp();
      scale = ren->GetActiveCamera()->GetDistance();
    }
    double dop[3];
    ren->GetActiveCamera()->GetDirectionOfProjection(dop);

    // dtmp is dop but orthogonal to vup
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
      // just rotate vup by 90 degress on X then Y
      // that will give us an orthogonal vector.
      dtmp[0] = vup[2];
      dtmp[1] = vup[0];
      dtmp[2] = vup[1];
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

    ren->GetActiveCamera()->GetPosition(tpos);
    tpos[0] += (0.7 * scale * dop[0] - 0.1 * scale * vr[0] - 0.4 * scale * vup[0]);
    tpos[1] += (0.7 * scale * dop[1] - 0.1 * scale * vr[1] - 0.4 * scale * vup[1]);
    tpos[2] += (0.7 * scale * dop[2] - 0.1 * scale * vr[2] - 0.4 * scale * vup[2]);
    this->TextActor3D->SetPosition(tpos);
    // scale should cover 10% of FOV
    double fov = ren->GetActiveCamera()->GetViewAngle();
    double tsize = 0.1 * 2.0 * atan(fov * 0.5); // 10% of fov
    tsize /= 200.0;                             // about 200 pixel texture map
    tsize *= scale;
    this->TextActor3D->SetScale(tsize, tsize, tsize);

    this->ImageActor->SetOrientation(orient);
    this->ImageActor->RotateX(-30.0);
    this->ImageActor->SetPosition(tpos);
    this->ImageActor->SetScale(scale, scale, scale);
  }

  this->TextActor3D->SetInput(text.c_str());
  ren->AddActor(this->TextActor3D);

  this->TextActor3D->ForceOpaqueOn();
  vtkTextProperty* prop = this->TextActor3D->GetTextProperty();
  prop->SetFrame(1);
  prop->SetFrameColor(1.0, 1.0, 1.0);
  prop->SetBackgroundOpacity(1.0);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(14);

  if (textureFile.size())
  {
    // is it a file reference?
    if (!strncmp(textureFile.c_str(), "file://", 7))
    {
      std::string fname = (textureFile.c_str() + 7);

      // handle non standard \\c:\ format?
      // clang-format off
      std::regex matcher(R"=(\\\\[a-zA-Z]:\\)=");
      // clang-format on
      if (std::regex_match(fname, matcher))
      {
        fname = fname.substr(2);
      }

      vtkNew<vtkJPEGReader> rdr;
      if (rdr->CanReadFile(textureFile.c_str() + 7))
      {
        vtkNew<vtkTexture> texture;
        rdr->SetFileName(textureFile.c_str() + 7);
        texture->SetInputConnection(rdr->GetOutputPort());
        rdr->Update();

        texture->InterpolateOn();
        texture->MipmapOn();

        // use constant area of 0.5 m^2
        int* dims = texture->GetInput()->GetDimensions();
        double xsize = sqrt(0.5 * dims[0] / dims[1]);
        double ysize = 0.5 / xsize;
        this->ImagePlane->SetOrigin(-xsize - 0.01, 0.0, 0.0);
        this->ImagePlane->SetPoint1(-0.01, 0.0, 0.0);
        this->ImagePlane->SetPoint2(-xsize - 0.01, ysize, 0.0);

        this->ImageActor->SetTexture(texture);

        ren->AddActor(this->ImageActor);
      }
    }

// is it a http reference
#ifdef OPENVR_HAS_IMAGO_SUPPORT
    {
      bool success = false;
      if (!strncmp(textureFile.c_str(), "http", 4))
      {
        success = this->ImagoLoader->GetHttpImage(textureFile, this->ImageFuture);
      }
      if (!strncmp(textureFile.c_str(), "holeid:", 7))
      {
        success = this->ImagoLoader->GetImage(textureFile, this->ImageFuture);
      }

      if (success)
      {
        this->ImageActor->SetTexture(nullptr);
        this->ImagePlane->SetOrigin(-0.2 - 0.01, 0.0, 0.0);
        this->ImagePlane->SetPoint1(-0.01, 0.0, 0.0);
        this->ImagePlane->SetPoint2(-0.2 - 0.01, 0.2, 0.0);
        this->WaitingForImage = true;
        this->RenderObserver =
          ren->AddObserver(vtkCommand::StartEvent, this, &vtkPVOpenVRWidgets::UpdateTexture);

        ren->AddActor(this->ImageActor);
      }
    }
#endif
  }
}

void vtkPVOpenVRWidgets::HideBillboard()
{
  this->Helper->GetRenderer()->RemoveActor(this->TextActor3D);
  this->Helper->GetRenderer()->RemoveActor(this->ImageActor);
}

bool vtkPVOpenVRWidgets::HasCellImage(vtkStringArray* sa, vtkIdType currCell)
{
  std::string svalue = sa->GetValue(currCell);
  if (!strncmp(svalue.c_str(), "file://", 7))
  {
    return true;
  }
  if (!strncmp(svalue.c_str(), "http", 4))
  {
    return true;
  }
  return false;
}

bool vtkPVOpenVRWidgets::FindCellImage(
  vtkDataSetAttributes* celld, vtkIdType currCell, std::string& image)
{
  image = "";

  vtkStringArray* sa = nullptr;
  vtkIdType numcd = celld->GetNumberOfArrays();

  // first check for a file: or http: column
  for (vtkIdType i = 0; i < numcd; ++i)
  {
    vtkAbstractArray* aa = celld->GetAbstractArray(i);
    if (aa && aa->GetName())
    {
      sa = vtkStringArray::SafeDownCast(aa);
      if (sa)
      {
        if (this->HasCellImage(sa, currCell))
        {
          image = sa->GetValue(currCell);
          return true;
        }
      }
    }
  }

  // second check for a holeid and depth
  for (vtkIdType i = 0; i < numcd; ++i)
  {
    vtkAbstractArray* aa = celld->GetAbstractArray(i);
    if (aa && aa->GetName())
    {
      std::string fname = aa->GetName();
      sa = vtkStringArray::SafeDownCast(aa);
      if (sa)
      {
        std::transform(fname.begin(), fname.end(), fname.begin(),
          [](unsigned char c) -> unsigned char { return std::tolower(c); });
        if (fname == "hole_id" || fname == "holeid" || fname == "dhid")
        {
          image = "holeid:de=";
          image += sa->GetValue(currCell);
        }
      }
    }
  }

  if (!image.size())
  {
    return false;
  }

  // find a depth
  for (vtkIdType i = 0; i < numcd; ++i)
  {
    vtkAbstractArray* aa = celld->GetAbstractArray(i);
    if (aa && aa->GetName())
    {
      vtkDataArray* da = vtkDataArray::SafeDownCast(aa);
      if (da)
      {
        std::string fname = aa->GetName();
        std::transform(fname.begin(), fname.end(), fname.begin(),
          [](unsigned char c) -> unsigned char { return std::tolower(c); });
        if (fname == "from")
        {
          image += "&dp=";
          image += std::to_string(da->GetComponent(currCell, 0));
          return true;
        }
      }
    }
  }

  return false;
}

bool vtkPVOpenVRWidgets::IsCellImageDifferent(std::string const& oldimg, std::string const& newimg)
{
  // if one is empty when the other is not return quickly
  if (oldimg.size() == 0 && newimg.size() != 0)
  {
    return true;
  }
  if (newimg.size() == 0 && oldimg.size() != 0)
  {
    return true;
  }
  // if they are exactly equal return quickly
  if (oldimg == newimg)
  {
    return false;
  }

  // at this point we know they are different, non empty strings

  // for file: links we just do a string compare
  if (!strncmp(newimg.c_str(), "file://", 7))
  {
    return oldimg != newimg;
  }

// deal with http and holeid: style values
#ifdef OPENVR_HAS_IMAGO_SUPPORT
  return this->ImagoLoader->IsCellImageDifferent(oldimg, newimg);
#else
  return oldimg != newimg;
#endif
}

void vtkPVOpenVRWidgets::MoveToNextImage()
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
  double originWC[4];
  double point1WC[4];
  double point2WC[4];
  double tmp[4];
  tmp[3] = 1.0;
  this->ImagePlane->GetOrigin(tmp);
  mat4->MultiplyPoint(tmp, originWC);
  this->ImagePlane->GetPoint1(tmp);
  mat4->MultiplyPoint(tmp, point1WC);
  this->ImagePlane->GetPoint2(tmp);
  mat4->MultiplyPoint(tmp, point2WC);

  // now we have the points all in WC create a coord system
  // with X being along the X axis of the plane
  // XZ containing the ray position and Y perp
  vtkVector3d originPos(originWC);
  vtkVector3d point1Pos(point1WC);
  vtkVector3d point2Pos(point2WC);

  vtkVector3d dirx = point1Pos - originPos;
  dirx.Normalize();
  vtkVector3d diry = point2Pos - originPos;
  diry.Normalize();
  vtkVector3d plane = dirx.Cross(diry);

  // intersect the ray with the plane
  double factor = plane.Dot(rdir);
  double offset = -plane.Dot(rpos - originPos) / factor;
  vtkVector3d intersect = rpos + rdir * offset;

  double xpos = dirx.Dot(intersect - originPos) / (point1Pos - originPos).Norm();
  double ypos = diry.Dot(intersect - originPos) / (point2Pos - originPos).Norm();

  double value = (fabs(xpos - 0.5) * (point1Pos - originPos).Norm() >
                   fabs(ypos - 0.5) * (point2Pos - originPos).Norm())
    ? xpos
    : 1.0 - ypos;

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
  this->FindCellImage(celld, currCell, lastImage);

  // now move forward or back to find the next new image
  int step = (value < 0.5 ? -1 : 1);
  while (!found && currCell + step < numCells && currCell + step >= 0)
  {
    currCell += step;
    std::string nextImage;
    this->FindCellImage(celld, currCell, nextImage);
    if (this->IsCellImageDifferent(lastImage, nextImage))
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

void vtkPVOpenVRWidgets::MoveToNextCell()
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

void vtkPVOpenVRWidgets::UpdateBillboard(bool updatePosition)
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

  std::string textureFile;

  vtkIdType numcd = celld->GetNumberOfArrays();

  // watch for image file names
  this->FindCellImage(celld, aid, textureFile);
  for (vtkIdType i = 0; i < numcd; ++i)
  {
    vtkAbstractArray* aa = celld->GetAbstractArray(i);
    if (aa && aa->GetName())
    {
      toString << " " << aa->GetName() << ": ";
      vtkStringArray* sa = vtkStringArray::SafeDownCast(aa);
      if (sa)
      {
        std::string svalue = sa->GetValue(aid);
        toString << svalue << " \n";
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

  vtkOpenGLRenderer* ren = this->Helper->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());

  std::vector<std::string> cvals;
  cvals.push_back(toString.str());
  cvals.push_back(updatePosition ? "true" : "false");
  cvals.push_back(textureFile);
  this->Helper->GetCollaborationClient()->ShowBillboard(cvals);
  this->ShowBillboard(toString.str(), updatePosition, textureFile);
  auto style = renWin->GetInteractor()
    ? vtkOpenVRInteractorStyle::SafeDownCast(renWin->GetInteractor()->GetInteractorStyle())
    : nullptr;
  if (style)
  {
    style->ShowPickCell(cell, vtkProp3D::SafeDownCast(this->LastPickedProp));
  }
}

void vtkPVOpenVRWidgets::SetEditableFieldValue(std::string value)
{
  this->Helper->GetEditableField();

  if (!this->LastPickedDataSet || !this->LastPickedDataSet->GetCellData())
  {
    vtkErrorMacro("no last picked dataset to edit or no cell data on last picked dataset.");
    return;
  }

  vtkAbstractArray* array =
    this->LastPickedDataSet->GetCellData()->GetAbstractArray(this->EditableField.c_str());

  if (!array)
  {
    vtkErrorMacro("array named " << this->EditableField << " not found in cell data.");
    return;
  }

  if (this->LastPickedCellId < 0 || this->LastPickedCellId >= array->GetNumberOfTuples())
  {
    vtkErrorMacro("last picked cell id is outside the number of cellls in the edit field.");
    return;
  }

  vtkStringArray* sarray = vtkStringArray::SafeDownCast(array);
  if (sarray)
  {
    for (vtkIdType cidx : this->SelectedCells)
    {
      sarray->SetValue(cidx, value);
    }
  }
  else
  {
    vtkDataArray* darray = vtkDataArray::SafeDownCast(array);
    if (darray)
    {
      char* pEnd;
      double d1;
      d1 = strtod(value.c_str(), &pEnd);
      if (pEnd == value.c_str() + value.size())
      {
        for (vtkIdType cidx : this->SelectedCells)
        {
          darray->SetTuple1(cidx, d1);
        }
      }
      else
      {
        vtkErrorMacro("unable to convert field value " << value << " to double.");
        return;
      }
    }
  }

  array->Modified();
  this->LastPickedDataSet->Modified();
  this->LastPickedRepresentation->Modified();

  // find the matching repr
  vtkSMPropertyHelper helper(this->Helper->GetSMView(), "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMPVRepresentationProxy* repr2 =
      vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(i));

    if (repr2)
    {
      if (vtkPVDataRepresentation::SafeDownCast(repr2->GetClientSideObject())
            ->GetRenderedDataObject(0) == this->LastPickedRepresentation->GetRenderedDataObject(0))
      {
        vtkSMPropertyHelper helper2(repr2, "Input");
        vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(helper2.GetAsProxy());

        source->MarkDirty(source);
        source->UpdateSelfAndAllInputs();
        source->UpdatePipeline();
        repr2->UpdateSelfAndAllInputs();
        repr2->UpdatePipeline();
        this->Helper->SetNeedStillRender(true);
        break;
      }
    }
  }
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
  return nullptr;
}
}

void vtkPVOpenVRWidgets::HandlePickEvent(vtkObject*, void* calldata)
{
  this->SelectedCells.clear();
  this->HideBillboard();
  this->Helper->GetCollaborationClient()->HideBillboard();

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

  auto* view = vtkPVRenderView::SafeDownCast(this->Helper->GetSMView()->GetClientSideView());
  vtkPVDataRepresentation* repr = FindRepresentation(prop, view);
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
  vtkDataSet* ds = nullptr;
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
