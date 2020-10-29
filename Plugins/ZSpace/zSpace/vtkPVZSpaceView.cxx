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

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkGeometryRepresentation.h"
#include "vtkMapper.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <zSpace.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVZSpaceView);

#define ZSPACE_CHECK_ERROR(fn, error)                                                              \
  if (error != ZC_ERROR_OK)                                                                        \
  {                                                                                                \
    char errorString[256];                                                                         \
    zcGetErrorString(error, errorString, sizeof(errorString));                                     \
    vtkErrorMacro(<< "vtkPVZSpaceView::" << #fn << " error : " << errorString);                    \
  }

//----------------------------------------------------------------------------
vtkPVZSpaceView::vtkPVZSpaceView()
{
  this->InitializeZSpace();
  this->GetRenderer()->GetCullers()->RemoveAllItems();

  vtkMemberFunctionCommand<vtkPVZSpaceView>* observer =
    vtkMemberFunctionCommand<vtkPVZSpaceView>::New();
  observer->SetCallback(*this, &vtkPVZSpaceView::ResetCamera);
  this->AddObserver(vtkCommand::ResetCameraEvent, observer);
  observer->FastDelete();
}

//------------------------------------------------------------------------------
vtkPVZSpaceView::~vtkPVZSpaceView()
{
  zcDestroyStereoBuffer(this->BufferHandle);
  zcDestroyViewport(this->ViewportHandle);
  zcShutDown(this->ZSpaceContext);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::ResetCamera()
{
  this->UpdateZSpaceFrustumParameters();
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::InitializeZSpace()
{
  ZCError error;

  // Initialize the zSpace SDK. This MUST be called before
  // calling any other zSpace API.
  error = zcInitialize(&this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zcInitialize, error);

  int numDisplays;
  error = zcGetNumDisplays(this->ZSpaceContext, &numDisplays);

  this->Displays.reserve(numDisplays);

  for (int i = 0; i < numDisplays; i++)
  {
    ZCHandle displayHandle;
    zcGetDisplayByIndex(this->ZSpaceContext, i, &displayHandle);

    ZCDisplayType displayType;
    zcGetDisplayType(displayHandle, &displayType);

    switch (displayType)
    {
      case ZC_DISPLAY_TYPE_GENERIC:
        this->Displays.push_back("Generic");
        break;
      case ZC_DISPLAY_TYPE_ZSPACE:
        this->Displays.push_back("ZSpace");
        break;
      default:
        this->Displays.push_back("Unknown");
        break;
    }
  }

  // Create a stereo buffer to handle L/R detection.
  error =
    zcCreateStereoBuffer(this->ZSpaceContext, ZC_RENDERER_QUAD_BUFFER_GL, 0, &this->BufferHandle);
  ZSPACE_CHECK_ERROR(zcCreateStereoBuffer, error);

  // Create a zSpace viewport object and grab its associated frustum.
  // Note: The zSpace viewport is abstract and not an actual window/viewport
  // that is created and registered through the Windows OS. It manages
  // a zSpace stereo frustum, which is responsible for various stereoscopic
  // 3D calculations such as calculating the view and projection matrices for
  // each eye.
  error = zcCreateViewport(this->ZSpaceContext, &this->ViewportHandle);
  ZSPACE_CHECK_ERROR(zcCreateViewport, error);

  error = zcGetFrustum(this->ViewportHandle, &this->FrustumHandle);
  ZSPACE_CHECK_ERROR(zcGetFrustum, error);

  // Enable auto stereo.
  zcSetFrustumAttributeB(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_AUTO_STEREO_ENABLED, true);
  zcSetFrustumAttributeF32(
    this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_HEAD_SCALE, 1.f);

  zcSetFrustumPortalMode(this->FrustumHandle, ZC_PORTAL_MODE_NONE);

  error = zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_PRIMARY, &this->StylusTargets);
  ZSPACE_CHECK_ERROR(zcGetNumTargetsByType, error);

  error = zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_HEAD, &this->HeadTargets);
  ZSPACE_CHECK_ERROR(zcGetNumTargetsByType, error);

  error =
    zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_SECONDARY, &this->SecondaryTargets);
  ZSPACE_CHECK_ERROR(zcGetNumTargetsByType, error);

  // Grab a handle to the stylus target.
  error = zcGetTargetByType(this->ZSpaceContext, ZC_TARGET_TYPE_PRIMARY, 0, &this->StylusHandle);
  ZSPACE_CHECK_ERROR(zcGetTargetByType, error);

  // Find the zSpace display and set the window position
  // to be the top left corner of the zSpace display.
  error = zcGetDisplayByType(this->ZSpaceContext, ZC_DISPLAY_TYPE_ZSPACE, 0, &this->DisplayHandle);
  ZSPACE_CHECK_ERROR(zcGetDisplayByType, error);

  error = zcGetDisplayPosition(this->DisplayHandle, &this->WindowX, &this->WindowY);
  ZSPACE_CHECK_ERROR(zcGetDisplayPosition, error);

  error =
    zcGetDisplayNativeResolution(this->DisplayHandle, &this->WindowWidth, &this->WindowHeight);
  ZSPACE_CHECK_ERROR(zcGetDisplayNativeResolution, error);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::UpdateTrackers()
{
  ZCError error;

  vtkRenderWindow* renWin = this->GetRenderWindow();
  int* position = renWin->GetPosition();
  int* size = renWin->GetSize();

  // Update the zSpace viewport position and size based
  // on the position and size of the application window.
  error = zcSetViewportPosition(this->ViewportHandle, position[0], position[1]);
  ZSPACE_CHECK_ERROR(zcSetViewportPosition, error);
  error = zcSetViewportSize(this->ViewportHandle, size[0], size[1]);
  ZSPACE_CHECK_ERROR(zcSetViewportSize, error);

  // Update inter pupillary distance
  zcSetFrustumAttributeF32(
    this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);

  // Update the zSpace SDK. This updates both tracking information
  // as well as the head poses for any frustums that have been created.
  error = zcUpdate(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zcUpdate, error);

  ZCTrackerPose headPose;
  zcGetFrustumHeadPose(this->FrustumHandle, &headPose);

  error = zcTransformMatrix(this->ViewportHandle, ZC_COORDINATE_SPACE_TRACKER,
    ZC_COORDINATE_SPACE_CAMERA, &headPose.matrix);
  ZSPACE_CHECK_ERROR(zcTransformMatrix, error);

  // prepare draw
  zcBeginStereoBufferFrame(this->BufferHandle);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::UpdateCamera()
{
  vtkCamera* activeCamera = this->GetActiveCamera();

  ZCEye eye = activeCamera->GetLeftEye() ? ZC_EYE_LEFT : ZC_EYE_RIGHT;

  // get zSpace matrix
  ZSMatrix4 zViewMatrix;
  zcGetFrustumViewMatrix(this->FrustumHandle, eye, &zViewMatrix);

  vtkNew<vtkMatrix4x4> zSpaceViewMatrix;
  for (int i = 0; i < 16; i++)
  {
    zSpaceViewMatrix->SetElement(i % 4, i / 4, zViewMatrix.f[i]);
  }

  // Give to the active camera the zSpace view transform
  vtkNew<vtkTransform> zSpaceViewTransform;
  zSpaceViewTransform->SetMatrix(zSpaceViewMatrix);
  activeCamera->SetUserViewTransform(zSpaceViewTransform);

  ZSMatrix4 zProjectionMatrix;
  zcGetFrustumProjectionMatrix(this->FrustumHandle, eye, &zProjectionMatrix);

  vtkNew<vtkMatrix4x4> zSpaceTransformMatrix;
  for (int i = 0; i < 16; i++)
  {
    zSpaceTransformMatrix->SetElement(i % 4, i / 4, zProjectionMatrix.f[i]);
  }

  // Give to the camera the zSpace projection matrix
  activeCamera->SetExplicitProjectionTransformMatrix(zSpaceTransformMatrix);
  activeCamera->SetUseExplicitProjectionTransformMatrix(true);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::UpdateZSpaceFrustumParameters(bool setPosition)
{
  if (this->GeometryBounds.IsValid() && !this->LockBounds && this->DiscreteCameras == nullptr)
  {
    double bounds[6];
    this->GeometryBounds.GetBounds(bounds);
    // Find parameters computed by zSpace SDK
    this->CalculateFit(bounds, setPosition);
  }
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::CalculateFit(double* bounds, bool setPosition)
{
  // Expand bounds a little bit to make sure object is not clipped
  double w1 = bounds[1] - bounds[0];
  double w2 = bounds[3] - bounds[2];
  double w3 = bounds[5] - bounds[4];

  ZCBoundingBox b;
  b.lower.x = bounds[0] - w1 / 4.0;
  b.lower.y = bounds[2] - w2 / 4.0;
  b.lower.z = bounds[4] - w3 / 4.0;

  b.upper.x = bounds[1] + w1 / 4.0;
  b.upper.y = bounds[3] + w2 / 4.0;
  b.upper.z = bounds[5] + w3 / 4.0;

  ZSMatrix4 lookAtMatrix;
  ZSFloat viewerScale;

  // Calculate the appropriate viewer scale and camera lookat matrix
  // such that content in the above bounding box will take up the entire
  // viewport without being clipped.
  ZCError error = zcCalculateFrustumFit(this->FrustumHandle, &b, &viewerScale, &lookAtMatrix);
  ZSPACE_CHECK_ERROR(zcCalculateFrustumFit, error);

  if (setPosition)
  {
    double center[3];
    center[0] = (bounds[0] + bounds[1]) / 2.0;
    center[1] = (bounds[2] + bounds[3]) / 2.0;
    center[2] = (bounds[4] + bounds[5]) / 2.0;

    this->GetActiveCamera()->SetPosition(-lookAtMatrix.m03, -lookAtMatrix.m13, -lookAtMatrix.m23);
    this->GetActiveCamera()->SetViewUp(lookAtMatrix.m01, lookAtMatrix.m11, lookAtMatrix.m21);
    this->GetActiveCamera()->SetFocalPoint(center[0], center[1], center[2]);
  }

  // Set the frustum's viewer scale with the value that was calculated
  // by zcCalculateFrustumFit().
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_VIEWER_SCALE, viewerScale);

  // Set the near and far clip depending on the vtk clipping range and the viewer scale
  double clippingRange[2];
  this->GetActiveCamera()->GetClippingRange(clippingRange);

  const float nearPlane = 0.5 * clippingRange[0] / viewerScale;
  const float farPlane = 1.5 * clippingRange[1] / viewerScale;

  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_NEAR_CLIP, nearPlane);
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_FAR_CLIP, farPlane);

  // Check every actors. In case of a surface with edges representation,
  // modify the unit of the offset of edges depending on the viewer scale.
  // Depending on the Z orientation of the camera, the unit is positive or negative
  const double* orientation = this->GetActiveCamera()->GetOrientation();
  double sign = orientation[2] < 0 ? -1 : 1;

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
  this->UpdateTrackers();
  this->UpdateCamera();

  this->Superclass::Render(interactive, skip_rendering);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WindowX: " << this->WindowX << "\n";
  os << indent << "WindowY: " << this->WindowY << "\n";
  os << indent << "WindowWidth: " << this->WindowWidth << "\n";
  os << indent << "WindowHeight: " << this->WindowHeight << "\n";
  os << indent << "NbDisplays: " << this->Displays.size() << "\n";
  for (auto const& display : this->Displays)
  {
    os << indent << "\t" << display << "\n";
  }
  os << indent << "StylusTargets: " << this->StylusTargets << "\n";
  os << indent << "HeadTargets: " << this->HeadTargets << "\n";
  os << indent << "SecondaryTargets: " << this->SecondaryTargets << "\n";
}
