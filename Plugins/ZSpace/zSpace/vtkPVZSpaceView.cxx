/*=========================================================================

  Program:   ParaView
  Module:    vtkPVZSpaceView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVZSpaceView.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVHardwareSelector.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkZSpaceCamera.h"
#include "vtkZSpaceInteractorStyle.h"
#include "vtkZSpaceRayActor.h"
#include "vtkZSpaceRenderWindowInteractor.h"
#include "vtkZSpaceSDKManager.h"

vtkStandardNewMacro(vtkPVZSpaceView);
//----------------------------------------------------------------------------
vtkPVZSpaceView::vtkPVZSpaceView()
{
  if (this->InteractorStyle) //  bother creating interactor styles only if superclass did as well.
  {
    this->ZSpaceInteractorStyle->SetZSpaceRayActor(this->StylusRayActor);
    this->ZSpaceInteractorStyle->SetZSpaceView(this);
  }

  // Set our custom camera to the renderer
  this->GetRenderer()->SetActiveCamera(this->ZSpaceCamera);

  this->GetRenderer()->AddActor(this->StylusRayActor);

  // Override ResetCameraEvent to use the zSpace reset camera
  vtkMemberFunctionCommand<vtkPVZSpaceView>* observer =
    vtkMemberFunctionCommand<vtkPVZSpaceView>::New();
  observer->SetCallback(*this, &vtkPVZSpaceView::ResetCamera);
  this->AddObserver(vtkCommand::ResetCameraEvent, observer);
  observer->FastDelete();

  this->GetRenderer()->GetCullers()->RemoveAllItems();

  // Hide axes
  this->OrientationWidget->SetParentRenderer(nullptr);
}

//----------------------------------------------------------------------------
vtkPVZSpaceView::~vtkPVZSpaceView() = default;

//----------------------------------------------------------------------------
void vtkPVZSpaceView::SetupInteractor(vtkRenderWindowInteractor* vtkNotUsed(rwi))
{
  this->Interactor = vtkSmartPointer<vtkZSpaceRenderWindowInteractor>::New();
  this->Interactor->SetRenderWindow(this->GetRenderWindow());

  // this will set the interactor style.
  int mode = this->InteractionMode;
  this->InteractionMode = INTERACTION_MODE_UNINTIALIZED;
  this->SetInteractionMode(mode);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::SetInteractionMode(int mode)
{
  this->Superclass::SetInteractionMode(mode);
  if (this->Interactor &&
    this->Interactor->GetInteractorStyle() != this->ZSpaceInteractorStyle.GetPointer())
  {
    switch (this->InteractionMode)
    {
      case INTERACTION_MODE_3D:
        this->Interactor->SetInteractorStyle(this->ZSpaceInteractorStyle);
        break;
      case INTERACTION_MODE_2D:
        VTK_FALLTHROUGH;
      default:
        break;
    }
  }
  this->SetActiveCamera(this->ZSpaceCamera);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::ResetCamera()
{
  this->Update();
  if (this->GeometryBounds.IsValid() && !this->LockBounds && this->DiscreteCameras == nullptr)
  {
    double bounds[6];
    this->GeometryBounds.GetBounds(bounds);
    // Find parameters computed by zSpace SDK
    this->CalculateFit(bounds);
  }
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::ResetCamera(double bounds[6])
{
  if (!this->LockBounds && this->DiscreteCameras == nullptr)
  {
    this->CalculateFit(bounds);
  }
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::ResetAllUserTransforms()
{
  vtkActor* actor;
  vtkActorCollection* actorCollection = this->GetRenderer()->GetActors();
  vtkCollectionSimpleIterator ait;
  vtkNew<vtkTransform> identity;
  for (actorCollection->InitTraversal(ait); (actor = actorCollection->GetNextActor(ait));)
  {
    actor->SetUserTransform(identity);
  }
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::CalculateFit(double* bounds)
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  // Get the viewer scale, camera position and camera view up from zSpace
  double position[3];
  double viewUp[3];
  sdkManager->CalculateFrustumFit(bounds, position, viewUp);

  // Set the position, view up and focal point
  double center[3];
  center[0] = (bounds[0] + bounds[1]) / 2.0;
  center[1] = (bounds[2] + bounds[3]) / 2.0;
  center[2] = (bounds[4] + bounds[5]) / 2.0;

  double vn[3];
  this->ZSpaceCamera->GetViewPlaneNormal(vn);

  this->ZSpaceCamera->SetViewUp(viewUp[0], viewUp[1], viewUp[2]);
  this->ZSpaceCamera->SetFocalPoint(center[0], center[1], center[2]);
  this->ZSpaceCamera->SetPosition(center[0] + vn[0] * position[0], center[1] + vn[1] * position[1],
    center[2] + vn[2] * position[2]);

  // Set the near and far clip depending on the vtk clipping range and the viewer scale
  this->GetRenderer()->ResetCameraClippingRange(bounds);
  double clippingRange[2];
  this->ZSpaceCamera->GetClippingRange(clippingRange);

  const float viewerScale = sdkManager->GetViewerScale();
  const float nearPlane = 0.5 * clippingRange[0] / viewerScale;
  const float farPlane = 5.0 * clippingRange[1] / viewerScale;

  // Give the near and far plane to zSpace SDK
  sdkManager->SetClippingRange(nearPlane, farPlane);

  // Check every actors. In case of a surface with edges representation,
  // modify the unit of the offset of edges depending on the viewer scale.
  // Depending on the Z orientation of the camera, the unit is positive or negative
  const double* orientation = this->ZSpaceCamera->GetOrientation();
  const double sign = orientation[2] < 0 ? -1 : 1;

  vtkActor* actor;
  vtkActorCollection* actorCollection = this->GetRenderer()->GetActors();
  vtkCollectionSimpleIterator ait;
  for (actorCollection->InitTraversal(ait); (actor = actorCollection->GetNextActor(ait));)
  {
    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
    {
      actor->GetMapper()->SetResolveCoincidentTopologyLineOffsetParameters(
        0, sign * 4 / viewerScale);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::Render(bool interactive, bool skip_rendering)
{
  if (!this->GetMakingSelection())
  {
    vtkZSpaceRenderWindowInteractor::SafeDownCast(this->Interactor)->HandleInteractions();
  }

  this->Superclass::Render(interactive, skip_rendering || this->GetMakingSelection());
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PickingFieldAssociation: " << this->PickingFieldAssociation << endl;

  vtkZSpaceSDKManager::GetInstance()->PrintSelf(os, indent.GetNextIndent());
  this->ZSpaceInteractorStyle->PrintSelf(os, indent.GetNextIndent());
  this->StylusRayActor->PrintSelf(os, indent.GetNextIndent());
  this->ZSpaceCamera->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SetInterPupillaryDistance(float interPupillaryDistance)
{
  vtkZSpaceSDKManager::GetInstance()->SetInterPupillaryDistance(interPupillaryDistance);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SetDrawStylus(bool drawStylus)
{
  this->DrawStylus = drawStylus;
  this->StylusRayActor->SetVisibility(this->DrawStylus);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SetInteractivePicking(bool interactivePicking)
{
  this->ZSpaceInteractorStyle->SetInteractivePicking(interactivePicking);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SelectWithRay(const double p0[3])
{
  // Change the camera to disable projection/view matrix of zSpace
  // To make sure the picked point will be at the middle of the viewport
  vtkNew<vtkCamera> defaultCamera;
  this->GetRenderer()->SetActiveCamera(defaultCamera);

  // Set the camera in the ray direction to select the middle of the
  // viewport
  vtkTransform* rayTransform = vtkZSpaceSDKManager::GetInstance()->GetStylusTransformRowMajor();

  double pin[4] = { 0.0, 0.0, 1.0, 1.0 };
  double dop[4];
  rayTransform->MultiplyPoint(pin, dop);

  for (int i = 0; i < 3; ++i)
  {
    dop[i] = dop[i] / dop[3];
  }
  vtkMath::Normalize(dop);

  // An actor should already be picked so get the length
  // between the origin of the ray and the picked actor
  double rayLength = this->StylusRayActor->GetLength();

  defaultCamera->SetPosition(p0);
  defaultCamera->SetFocalPoint(
    p0[0] + dop[0] * rayLength, p0[1] + dop[1] * rayLength, p0[2] + dop[2] * rayLength);
  defaultCamera->OrthogonalizeViewUp();

  // Make sure the picked cell/point is not clipped
  defaultCamera->SetClippingRange(0.5 * rayLength, 2.0 * rayLength);

  int* size = this->GetRenderer()->GetSize();
  // pick at the middle
  int region[4] = { size[0] / 2, size[1] / 2, size[0] / 2, size[1] / 2 };

  this->Selector->Modified();
  this->Select(this->PickingFieldAssociation, region);

  // Reset to zSpace camera
  this->GetRenderer()->SetActiveCamera(this->ZSpaceCamera);
}
