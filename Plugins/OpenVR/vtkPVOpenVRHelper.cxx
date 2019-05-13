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
#include "vtkGeometryRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInformation.h"
#include "vtkJPEGWriter.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNumberToString.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenVRFollower.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkOpenVRMenuRepresentation.h"
#include "vtkOpenVRMenuWidget.h"
#include "vtkOpenVROverlay.h"
#include "vtkOpenVROverlayInternal.h"
#include "vtkOpenVRPanelRepresentation.h"
#include "vtkOpenVRPanelWidget.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPVOpenVRCollaborationClient.h"
#include "vtkPVXMLElement.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkRenderViewBase.h"
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
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
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

#include "vtkObjectFactory.h"

#include "vtkOpenVROverlayInternal.h"

#include <sstream>

#include <QCoreApplication>

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
  this->EventCommand = vtkCallbackCommand::New();
  this->EventCommand->SetClientData(this);
  this->EventCommand->SetCallback(vtkPVOpenVRHelper::EventCallback);

  this->CropMenu->SetRepresentation(this->CropMenuRepresentation.Get());
  this->CropMenu->PushFrontMenuItem("exit", "Exit", this->EventCommand);
  this->CropMenu->PushFrontMenuItem("removeallcrops", "Remove All", this->EventCommand);
  this->CropMenu->PushFrontMenuItem("addacropplane", "Add a Crop Plane", this->EventCommand);
  this->CropMenu->PushFrontMenuItem("addathickcrop", "Add a Thick Crop", this->EventCommand);
  this->CropMenu->PushFrontMenuItem("togglesnapping", "Turn Snap to Axes On", this->EventCommand);

  this->EditFieldMenu = vtkOpenVRMenuWidget::New();
  this->EditFieldMenuRepresentation = vtkOpenVRMenuRepresentation::New();
  this->EditFieldMenu->SetRepresentation(this->EditFieldMenuRepresentation);

  this->ScalarMenu = vtkOpenVRMenuWidget::New();
  this->ScalarMenuRepresentation = vtkOpenVRMenuRepresentation::New();
  this->ScalarMenu->SetRepresentation(this->ScalarMenuRepresentation);

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
}

//----------------------------------------------------------------------------
vtkPVOpenVRHelper::~vtkPVOpenVRHelper()
{
  this->ScalarMenu->Delete();
  this->ScalarMenuRepresentation->Delete();
  this->ScalarMenu = nullptr;
  this->ScalarMenuRepresentation = nullptr;

  this->EditFieldMenu->Delete();
  this->EditFieldMenuRepresentation->Delete();
  this->EditFieldMenu = nullptr;
  this->EditFieldMenuRepresentation = nullptr;

  this->EventCommand->Delete();
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
  return this->CollaborationClient->Connect(this->Renderer);
}

bool vtkPVOpenVRHelper::CollaborationDisconnect()
{
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

void vtkPVOpenVRHelper::GetScalars()
{
  this->ScalarMap.clear();

  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMPVRepresentationProxy* repr =
      vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(i));
    vtkSMProperty* prop = repr ? repr->GetProperty("ColorArrayName") : nullptr;
    if (prop)
    {
      auto scalars = prop->FindDomain<vtkSMRepresentedArrayListDomain>();
      int numsc = scalars->GetNumberOfStrings();
      for (int j = 0; j < numsc; ++j)
      {
        std::string name = scalars->GetString(j);
        int assoc = scalars->GetFieldAssociation(j);
        if (assoc != vtkDataObject::FIELD && this->ScalarMap.find(name) == this->ScalarMap.end())
        {
          this->ScalarMap.insert(std::pair<std::string, int>(name, assoc));
        }
      }
    }
  }
}

void vtkPVOpenVRHelper::BuildScalarMenu()
{
  this->ScalarMenu->RemoveAllMenuItems();
  this->ScalarMenu->PushFrontMenuItem("exit", "Exit", this->EventCommand);

  int count = 0;
  for (auto i : this->ScalarMap)
  {
    std::string tmp = i.first;
    if (i.second == vtkDataObject::POINT)
    {
      tmp += " (point)";
    }
    if (i.second == vtkDataObject::CELL)
    {
      tmp += " (cell)";
    }

    std::ostringstream toString;
    toString << count;

    this->ScalarMenu->PushFrontMenuItem(toString.str().c_str(), tmp.c_str(), this->EventCommand);
    count++;
  }
}

namespace
{
std::string trim(std::string const& str)
{
  if (str.empty())
    return str;

  std::size_t firstScan = str.find_first_not_of(' ');
  std::size_t first = firstScan == std::string::npos ? str.length() : firstScan;
  std::size_t last = str.find_last_not_of(' ');
  return str.substr(first, last - first + 1);
}
}

void vtkPVOpenVRHelper::SetFieldValues(const char* val)
{
  if (this->FieldValues == val)
  {
    return;
  }

  // new values we need to rebuild the
  // edit menu
  this->EditFieldMenu->RemoveAllMenuItems();
  this->EditFieldMenu->PushFrontMenuItem("exit", "Exit", this->EventCommand);

  this->FieldValues = val;
  this->EditFieldMap.clear();

  std::istringstream iss(this->FieldValues);

  std::string token;
  int count = 0;
  while (std::getline(iss, token, ','))
  {
    token = trim(token);
    std::ostringstream toString;
    toString << count;

    this->EditFieldMap.insert(std::pair<int, std::string>(count, token));
    this->EditFieldMenu->PushFrontMenuItem(
      toString.str().c_str(), token.c_str(), this->EventCommand);
    count++;
  }
}

void vtkPVOpenVRHelper::EditField(std::string name)
{
  std::istringstream is(name);
  int target;
  is >> target;

  if (this->EditFieldMap.find(target) == this->EditFieldMap.end())
  {
    return;
  }

  std::string value = this->EditFieldMap[target];

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
  this->LastPickedRepresentation->MarkModified();

  this->NeedStillRender = true;
}

void vtkPVOpenVRHelper::ToggleNavigationPanel()
{
  if (!this->NavWidget->GetEnabled())
  {
    // add an observer on the left controller to update the bearing and position
    this->NavigationTag =
      this->Interactor->AddObserver(vtkCommand::Move3DEvent, this->EventCommand, 1.0);

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
  else
  {
    this->NavWidget->SetEnabled(0);
    this->Renderer->RemoveObserver(this->NavigationTag);
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
}

void vtkPVOpenVRHelper::AddAThickCrop(vtkTransform* intrans)
{
  // add an observer on the left controller to move the crop
  if (this->ThickCrops.size() == 0)
  {
    this->CropTag =
      this->Interactor->AddObserver(vtkCommand::Button3DEvent, this->EventCommand, 1.0);
  }

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

void vtkPVOpenVRHelper::RemoveAllThickCrops()
{
  if (this->ThickCrops.size())
  {
    this->Renderer->RemoveObserver(this->CropTag);
  }

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
}

void vtkPVOpenVRHelper::ToggleCropSnapping()
{
  this->CropSnapping = !this->CropSnapping;

  this->CropMenu->RenameMenuItem(
    "togglesnapping", (this->CropSnapping ? "Turn Snap to Axes Off" : "Turn Snap to Axes On"));

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

void vtkPVOpenVRHelper::SelectScalar()
{
  std::string name = this->SelectedScalar;
  this->SelectedScalar = "";
  std::istringstream is(name);
  int target;
  is >> target;

  int count = 0;
  std::string tname;
  int tassoc = -1;
  for (auto i : this->ScalarMap)
  {
    if (count == target)
    {
      tname = i.first;
      tassoc = i.second;
      break;
    }
    count++;
  }

  vtkSMPropertyHelper helper(this->SMView, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMPVRepresentationProxy* repr =
      vtkSMPVRepresentationProxy::SafeDownCast(helper.GetAsProxy(i));
    vtkSMProperty* prop = repr ? repr->GetProperty("ColorArrayName") : nullptr;
    if (prop)
    {
      auto scalars = prop->FindDomain<vtkSMRepresentedArrayListDomain>();
      int numsc = scalars->GetNumberOfStrings();
      bool found = false;
      for (int j = 0; j < numsc && !found; ++j)
      {
        std::string sname = scalars->GetString(j);
        int assoc = scalars->GetFieldAssociation(j);
        if (assoc == tassoc && sname == tname)
        {
          // vtkSMPVRepresentationProxy - has method SetColorArrayName
          repr->SetScalarColoring(tname.c_str(), tassoc);
          found = true;
        }
      }
      if (!found)
      {
        // solid color
        repr->SetScalarColoring(nullptr, tassoc);
      }
    }
  }
}

void vtkPVOpenVRHelper::HandleMenuEvent(
  vtkOpenVRMenuWidget* menu, vtkObject*, unsigned long eventID, void*, void* calldata)
{
  // handle menu events
  if (menu == this->Style->GetMenu() && eventID == vtkWidgetEvent::Select3D)
  {
    std::string name = static_cast<const char*>(calldata);

    if (name == "takemeasurement")
    {
      this->DistanceWidget->SetWidgetStateToStart();
      this->DistanceWidget->SetEnabled(0);
      this->DistanceWidget->SetEnabled(1);
    }
    if (name == "togglenavigationpanel")
    {
      this->ToggleNavigationPanel();
    }
    if (name == "selectscalar")
    {
      menu->ShowSubMenu(this->ScalarMenu);
    }
    if (name == "editfield")
    {
      menu->ShowSubMenu(this->EditFieldMenu);
    }
    if (name == "cropping")
    {
      menu->ShowSubMenu(this->CropMenu.Get());
    }
    return;
  }

  if (menu == this->ScalarMenu && eventID == vtkWidgetEvent::Select3D)
  {
    std::string name = static_cast<const char*>(calldata);

    if (name != "exit")
    {
      this->SelectedScalar = name;
    }
    return;
  }

  if (menu == this->EditFieldMenu && eventID == vtkWidgetEvent::Select3D)
  {
    std::string name = static_cast<const char*>(calldata);

    if (name != "exit")
    {
      this->EditField(name);
    }
    return;
  }

  if (menu == this->ScalarMenu && eventID == vtkWidgetEvent::Select)
  {
    this->Style->GetMenu()->On();
  }

  if (menu == this->EditFieldMenu && eventID == vtkWidgetEvent::Select)
  {
    this->Style->GetMenu()->On();
  }

  if (menu == this->CropMenu.Get() && eventID == vtkWidgetEvent::Select3D)
  {
    std::string name = static_cast<const char*>(calldata);

    if (name == "removeallcrops")
    {
      this->RemoveAllThickCrops();
      this->RemoveAllCropPlanes();
    }
    if (name == "addacropplane")
    {
      this->AddACropPlane(nullptr, nullptr);
    }
    if (name == "addathickcrop")
    {
      this->AddAThickCrop(nullptr);
    }
    if (name == "togglesnapping")
    {
      this->ToggleCropSnapping();
    }
    return;
  }

  if (menu == this->CropMenu.Get() && eventID == vtkWidgetEvent::Select)
  {
    this->Style->GetMenu()->On();
  }
}

void vtkPVOpenVRHelper::HandleInteractorEvent(
  vtkOpenVRRenderWindowInteractor*, vtkObject*, unsigned long eventID, void*, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
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

  if (edd && edd->GetDevice() == vtkEventDataDevice::LeftController &&
    eventID == vtkCommand::Button3DEvent && edd->GetInput() == vtkEventDataDeviceInput::TrackPad &&
    edd->GetAction() == vtkEventDataAction::Press)
  {
    const double* tpos = edd->GetTrackPadPosition();
    for (vtkBoxWidget2* iter : this->ThickCrops)
    {
      vtkBoxRepresentation* rep = static_cast<vtkBoxRepresentation*>(iter->GetRepresentation());
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
  this->CropMenu->RenameMenuItem(
    "togglesnapping", (this->CropSnapping ? "Turn Snap to Axes Off" : "Turn Snap to Axes On"));

  // set camera poses
  this->RenderWindow->GetDashboardOverlay()->GetSavedCameraPoses().clear();
  for (auto& loci : this->Locations)
  {
    this->RenderWindow->GetDashboardOverlay()->SetSavedCameraPose(loci.first, loci.second.Pose);
  }
}

void vtkPVOpenVRHelper::EventCallback(
  vtkObject* caller, unsigned long eventID, void* clientdata, void* calldata)
{
  vtkPVOpenVRHelper* self = static_cast<vtkPVOpenVRHelper*>(clientdata);

  vtkOpenVRRenderWindowInteractor* iren = vtkOpenVRRenderWindowInteractor::SafeDownCast(caller);
  if (iren)
  {
    self->HandleInteractorEvent(iren, caller, eventID, clientdata, calldata);
    return;
  }

  vtkOpenVRMenuWidget* menu = vtkOpenVRMenuWidget::SafeDownCast(caller);
  if (menu)
  {
    self->HandleMenuEvent(menu, caller, eventID, clientdata, calldata);
    return;
  }

  // handle different events
  switch (eventID)
  {
    case vtkCommand::SaveStateEvent:
    {
      self->SaveLocationState(reinterpret_cast<vtkTypeInt64>(calldata));
    }
    break;
    case vtkCommand::LoadStateEvent:
    {
      self->LoadLocationValue = reinterpret_cast<vtkTypeInt64>(calldata);
    }
    break;
    case vtkCommand::EndPickEvent:
    {
      self->SelectedCells.clear();

      vtkSelection* sel = vtkSelection::SafeDownCast(reinterpret_cast<vtkObjectBase*>(calldata));

      if (!sel || sel->GetNumberOfNodes() == 0)
      {
        self->PreviousPickedRepresentation = nullptr;
        self->PreviousPickedDataSet = nullptr;
        return;
      }

      vtkOpenVRInteractorStyle* is =
        vtkOpenVRInteractorStyle::SafeDownCast(reinterpret_cast<vtkObjectBase*>(caller));

      // for multiple nodes which one do we use?
      vtkSelectionNode* node = sel->GetNode(0);
      vtkProp3D* prop =
        vtkProp3D::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
      vtkPVDataRepresentation* repr = FindRepresentation(prop, self->View);
      if (!repr)
      {
        return;
      }
      self->PreviousPickedRepresentation = self->LastPickedRepresentation;
      self->LastPickedRepresentation = repr;

      // next two lines are debugging code to mark what actor was picked
      // by changing its color. Useful to track down picking errors.
      // double *color = static_cast<vtkActor*>(prop)->GetProperty()->GetColor();
      // static_cast<vtkActor*>(prop)->GetProperty()->SetColor(color[0] > 0.0 ? 0.0 : 1.0, 0.5,
      // 0.5);
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
      vtkCell* cell = ds->GetCell(aid);

      self->PreviousPickedDataSet = self->LastPickedDataSet;
      self->LastPickedDataSet = ds;
      self->PreviousPickedCellId = self->LastPickedCellId;
      self->LastPickedCellId = aid;
      self->SelectedCells.push_back(aid);

      vtkCellData* celld = ds->GetCellData();

      // update the billboard with information about this data
      std::ostringstream toString;
      double p1[6];
      cell->GetBounds(p1);
      double pos[3] = { 0.5 * (p1[1] + p1[0]), 0.5 * (p1[3] + p1[2]), 0.5 * (p1[5] + p1[4]) };
      toString << "\n Cell Center (DC): " << pos[0] << ", " << pos[1] << ", " << pos[2] << " \n";
      vtkMatrix4x4* pmat = prop->GetMatrix();
      double wpos[4];
      wpos[0] = pos[0];
      wpos[1] = pos[1];
      wpos[2] = pos[2];
      wpos[3] = 1.0;
      pmat->MultiplyPoint(wpos, wpos);
      toString << " Cell Center (WC): " << wpos[0] << ", " << wpos[1] << ", " << wpos[2] << " \n";

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
            toString << sa->GetValue(aid) << " \n";
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
        self->PreviousPickedRepresentation == self->LastPickedRepresentation &&
        self->PreviousPickedDataSet == self->LastPickedDataSet)
      {
        // ok same dataset, lets see if we have composite results
        vtkVariant hid1 = holeid->GetVariantValue(aid);
        vtkVariant hid2 = holeid->GetVariantValue(self->PreviousPickedCellId);
        if (hid1 == hid2)
        {
          toString << "\n Composite results:\n";
          double totDist = 0;
          double fromEnd = fromArray->GetTuple1(aid);
          double toEnd = toArray->GetTuple1(aid);
          double fromEnd2 = fromArray->GetTuple1(self->PreviousPickedCellId);
          double toEnd2 = toArray->GetTuple1(self->PreviousPickedCellId);
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
                    self->SelectedCells.push_back(cidx);
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

      is->ShowBillboard(toString.str());
      is->ShowPickCell(cell, vtkProp3D::SafeDownCast(prop));
    }
    break;
  }
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
  if (this->NavWidget->GetEnabled() != loc.NavigationPanelVisibility)
  {
    this->ToggleNavigationPanel();
  }

  // load crops
  this->RemoveAllCropPlanes();
  for (auto i : loc.CropPlaneStates)
  {
    this->AddACropPlane(i.first.data(), i.second.data());
  }

  // load thick crops
  this->RemoveAllThickCrops();
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
          "\"orientation\": { \"values\": [\"right\", \"left\", \"up\", \"down\", \"back\", "
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

  vtkNew<vtkXMLDataElement> topel;
  topel->SetName("View");

  std::vector<vtkActor*> actors;
  std::vector<vtkPolyData*> datas;
  std::vector<vtkImageData*> textures;

  vtkNew<vtkXMLDataElement> actorsel;
  actorsel->SetName("Actors");
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
    int acount = 0;
    for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
    {
      vtkNew<vtkXMLDataElement> actorel;
      actorel->SetName("Actor");

      if (!actor->GetVisibility() || !actor->GetMapper() ||
        !actor->GetMapper()->GetInputAlgorithm())
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

      // record the polydata if not already done
      int dcount;
      int pdfound = -1;
      for (dcount = 0; dcount < datas.size(); ++dcount)
      {
        if (datas[dcount] == pd)
        {
          pdfound = dcount;
          break;
        }
      }
      if (pdfound == -1)
      {
        pdfound = static_cast<int>(datas.size());
        datas.push_back(pd);
      }

      // record the texture if there is one
      int tfound = -1;
      if (actor->GetTexture() && actor->GetTexture()->GetInput())
      {
        vtkImageData* idata = actor->GetTexture()->GetInput();
        for (dcount = 0; dcount < textures.size(); ++dcount)
        {
          if (textures[dcount] == idata)
          {
            tfound = dcount;
            break;
          }
        }
        if (tfound == -1)
        {
          tfound = static_cast<int>(textures.size());
          textures.push_back(idata);
          actor->GetTexture()->Update();
        }
      }

      // record the actor id
      int found = -1;
      for (dcount = 0; dcount < actors.size(); ++dcount)
      {
        if (actors[dcount] == actor)
        {
          found = dcount;
          break;
        }
      }
      if (found == -1)
      {
        found = static_cast<int>(actors.size());
        actors.push_back(actor);

        // record actor properties
        vtkNew<vtkXMLDataElement> adatael;
        adatael->SetName("ActorData");

        adatael->SetIntAttribute("PolyData", pdfound);
        adatael->SetIntAttribute("PolyDataSize", pd->GetActualMemorySize());

        if (tfound != -1)
        {
          adatael->SetIntAttribute("TextureData", tfound);
          adatael->SetIntAttribute(
            "TextureSize", actor->GetTexture()->GetInput()->GetActualMemorySize());
        }

        adatael->SetIntAttribute("ActorID", found);
        adatael->SetVectorAttribute("DiffuseColor", 3, actor->GetProperty()->GetDiffuseColor());
        adatael->SetDoubleAttribute("Diffuse", actor->GetProperty()->GetDiffuse());
        adatael->SetVectorAttribute("AmbientColor", 3, actor->GetProperty()->GetAmbientColor());
        adatael->SetDoubleAttribute("Ambient", actor->GetProperty()->GetAmbient());
        adatael->SetVectorAttribute("SpecularColor", 3, actor->GetProperty()->GetSpecularColor());
        adatael->SetDoubleAttribute("Specular", actor->GetProperty()->GetSpecular());
        adatael->SetDoubleAttribute("SpecularPower", actor->GetProperty()->GetSpecularPower());
        adatael->SetDoubleAttribute("Opacity", actor->GetProperty()->GetOpacity());
        adatael->SetDoubleAttribute("LineWidth", actor->GetProperty()->GetLineWidth());

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

      actorel->SetIntAttribute("ActorID", found);
      poseel->AddNestedElement(actorel);
      acount++;
    }

    posesel->AddNestedElement(poseel);
    count++;
  }

  topel->AddNestedElement(posesel);
  topel->AddNestedElement(actorsel);

  // create subdir for the data
  std::string datadir = dir;
  datadir += "data/";
  vtksys::SystemTools::MakeDirectory(datadir);

  // now write the polydata
  for (int dcount = 0; dcount < datas.size(); ++dcount)
  {
    std::ostringstream sdir;
    sdir << datadir << "pdata" << dcount << ".vtp";
    vtkNew<vtkXMLPolyDataWriter> writer;
    writer->SetDataModeToAppended();
    writer->SetCompressorTypeToLZ4();
    writer->SetFileName(sdir.str().c_str());
    writer->SetInputData(datas[dcount]);
    writer->Write();
  }

  // now write the textures
  for (int dcount = 0; dcount < textures.size(); ++dcount)
  {
    std::ostringstream sdir;
    sdir << datadir << "tdata" << dcount << ".vti";
    vtkNew<vtkXMLImageDataWriter> writer;
    writer->SetDataModeToAppended();
    writer->SetCompressorTypeToLZ4();
    writer->SetFileName(sdir.str().c_str());
    writer->SetInputData(textures[dcount]);
    writer->Write();
  }

  vtkIndent indent;
  vtkXMLUtilities::WriteElementToFile(topel, "pv-view/index.mvx", &indent);

  // create empty extra.xml file
  vtkNew<vtkXMLDataElement> topel2;
  topel2->SetName("View");
  topel2->SetAttribute("ViewImage", "Filename.jpg");
  topel2->SetAttribute("Longitude", "0.0");
  topel2->SetAttribute("Latitude", "0.0");

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
    // test out sharing the context
    this->RenderWindow->SetHelperWindow(static_cast<vtkOpenGLRenderWindow*>(pvRenderWindow));
  }

  this->Renderer = vtkOpenVRRenderer::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Interactor = vtkOpenVRRenderWindowInteractor::New();
  this->RenderWindow->SetInteractor(this->Interactor);

  // iren->SetDesiredUpdateRate(220.0);
  // iren->SetStillUpdateRate(220.0);
  // renWin->SetDesiredUpdateRate(220.0);

  // add a pick observer
  this->Style = vtkOpenVRInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle());
  this->Style->AddObserver(vtkCommand::EndPickEvent, this->EventCommand, 1.0);
  this->RenderWindow->GetDashboardOverlay()->AddObserver(
    vtkCommand::SaveStateEvent, this->EventCommand, 1.0);
  this->RenderWindow->GetDashboardOverlay()->AddObserver(
    vtkCommand::LoadStateEvent, this->EventCommand, 1.0);
  this->Style->GetMenu()->PushFrontMenuItem(
    "takemeasurement", "Take a measurement", this->EventCommand);
  this->Style->GetMenu()->PushFrontMenuItem(
    "togglenavigationpanel", "Toggle the Navigation Panel", this->EventCommand);
  this->Style->GetMenu()->PushFrontMenuItem("cropping", "Cropping", this->EventCommand);
  this->Style->GetMenu()->PushFrontMenuItem(
    "selectscalar", "Select Scalar to View", this->EventCommand);
  this->Style->GetMenu()->PushFrontMenuItem("editfield", "Edit Field", this->EventCommand);

  static_cast<vtkOpenVRInteractorStyle*>(this->Interactor->GetInteractorStyle())
    ->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_PICK);

  // get the scalars
  this->GetScalars();
  this->BuildScalarMenu();

  this->Renderer->RemoveCuller(this->Renderer->GetCullers()->GetLastItem());
  this->Renderer->SetBackground(pvRenderer->GetBackground());

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

// send the first renderer to openVR

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

  if (this->RenderWindow->GetHMD())
  {
    this->UpdateProps();
    this->RenderWindow->Render();
    this->Renderer->ResetCamera();
    this->Renderer->ResetCameraClippingRange();
    this->ApplyState();
    this->Style->GetMenu()->RemoveMenuItem("grabmode");
    while (this->Interactor && !this->Interactor->GetDone())
    {
      this->Interactor->DoOneEvent(this->RenderWindow, this->Renderer);
      this->CollaborationClient->Render();
      QCoreApplication::processEvents();
      if (this->NeedStillRender)
      {
        this->SMView->StillRender();
        this->NeedStillRender = false;
      }
      if (this->SelectedScalar.size())
      {
        this->SMView->StillRender();
        this->SelectScalar();
        this->SMView->StillRender();
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

  this->RemoveAllThickCrops();
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

    vtkActorCollection* acol = pvRenderer->GetActors();
    vtkActor* actor;
    this->AddedProps->RemoveAllItems();
    for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
    {
      this->AddedProps->AddItem(actor);
      // force opaque is opacity is 1.0
      if (actor->GetProperty()->GetOpacity() >= 1.0)
      {
        actor->ForceOpaqueOn();
      }
      else
      {
        actor->ForceOpaqueOff();
      }
      this->Renderer->AddActor(actor);
      if (actor->GetTexture())
      {
        // release graphics resources
        actor->GetTexture()->InterpolateOn();
        if (!actor->GetTexture()->GetMipmap())
        {
          actor->GetTexture()->MipmapOn();
          actor->GetTexture()->ReleaseGraphicsResources(this->RenderWindow);
        }
        // mipmap on
      }
    }
    vtkVolumeCollection* avol = pvRenderer->GetVolumes();
    vtkVolume* volume;
    for (avol->InitTraversal(pit); (volume = avol->GetNextVolume(pit));)
    {
      this->AddedProps->AddItem(volume);
      this->Renderer->AddVolume(volume);
    }

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
