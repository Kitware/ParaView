/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkZSpaceSDKManager.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkZSpaceSDKManager.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

#include "vtkPVZSpaceView.h"

vtkStandardNewMacro(vtkZSpaceSDKManager);

#define ZSPACE_CHECK_ERROR(fn, error)                                                              \
  if (error != ZC_ERROR_OK)                                                                        \
  {                                                                                                \
    char errorString[256];                                                                         \
    zcGetErrorString(error, errorString, sizeof(errorString));                                     \
    vtkErrorMacro(<< "vtkZSpaceSDKManager::" << #fn << " error : " << errorString);                \
  }

//------------------------------------------------------------------------------
vtkZSpaceSDKManager::vtkZSpaceSDKManager()
{
  this->InitializeZSpace();
}

//------------------------------------------------------------------------------
vtkZSpaceSDKManager::~vtkZSpaceSDKManager()
{
  zcDestroyStereoBuffer(this->BufferHandle);
  zcDestroyViewport(this->ViewportHandle);
  zcShutDown(this->ZSpaceContext);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::InitializeZSpace()
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
void vtkZSpaceSDKManager::Update(vtkRenderWindow* renWin)
{
  this->UpdateViewport(renWin);
  this->UpdateViewAndProjectionMatrix();
  this->UpdateTrackers();
  this->UpdateButtonState();

  // prepare draw
  zcBeginStereoBufferFrame(this->BufferHandle);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateViewport(vtkRenderWindow* renWin)
{
  ZCError error;

  int* position = renWin->GetPosition();
  int* size = renWin->GetSize();

  error = zcSetViewportPosition(this->ViewportHandle, position[0], position[1]);
  ZSPACE_CHECK_ERROR(zcSetViewportPosition, error);
  error = zcSetViewportSize(this->ViewportHandle, size[0], size[1]);
  ZSPACE_CHECK_ERROR(zcSetViewportSize, error);

  // Update inter pupillary distance
  zcSetFrustumAttributeF32(
    this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);

  // Near and far plane
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_NEAR_CLIP, this->NearPlane);
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_FAR_CLIP, this->FarPlane);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateTrackers()
{
  ZCError error;

  // Update the zSpace SDK. This updates both tracking information
  // as well as the head poses for any frustums that have been created.
  error = zcUpdate(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zcUpdate, error);

  // Update the stylus matrix
  ZCTrackerPose stylusPose;
  error = zcGetTargetTransformedPose(
    this->StylusHandle, this->ViewportHandle, ZC_COORDINATE_SPACE_CAMERA, &stylusPose);

  vtkZSpaceSDKManager::ConvertZSpaceMatrixToVTKMatrix(
    stylusPose.matrix, this->StylusMatrixColMajor);

  // The stylus direction is the normalized negative Z axis of the pose
  this->StylusMatrixColMajor->SetElement(2, 0, -this->StylusMatrixColMajor->GetElement(2, 0));
  this->StylusMatrixColMajor->SetElement(2, 1, -this->StylusMatrixColMajor->GetElement(2, 1));
  this->StylusMatrixColMajor->SetElement(2, 2, -this->StylusMatrixColMajor->GetElement(2, 2));

  vtkNew<vtkMatrix4x4> invView;
  vtkMatrix4x4::Invert(this->CenterEyeViewMatrix.GetPointer(), invView);

  // Convert from camera space to world space
  vtkMatrix4x4::Multiply4x4(invView, this->StylusMatrixColMajor, this->StylusMatrixColMajor);

  // Transpose the matrix to be used by VTK
  vtkMatrix4x4::Transpose(this->StylusMatrixColMajor, this->StylusMatrixRowMajor);

  this->StylusTransformRowMajor->SetMatrix(this->StylusMatrixRowMajor);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateViewAndProjectionMatrix()
{
  // Update the view matrix for each eye
  ZSMatrix4 zcViewMatrix;
  zcGetFrustumViewMatrix(this->FrustumHandle, ZC_EYE_CENTER, &zcViewMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcViewMatrix, this->CenterEyeViewMatrix);

  zcGetFrustumViewMatrix(this->FrustumHandle, ZC_EYE_LEFT, &zcViewMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcViewMatrix, this->LeftEyeViewMatrix);

  zcGetFrustumViewMatrix(this->FrustumHandle, ZC_EYE_RIGHT, &zcViewMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcViewMatrix, this->RightEyeViewMatrix);

  // Update the projection matrix for each eye
  ZSMatrix4 zcProjectionMatrix;
  zcGetFrustumProjectionMatrix(this->FrustumHandle, ZC_EYE_CENTER, &zcProjectionMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcProjectionMatrix, this->CenterEyeProjectionMatrix);

  zcGetFrustumProjectionMatrix(this->FrustumHandle, ZC_EYE_LEFT, &zcProjectionMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcProjectionMatrix, this->LeftEyeProjectionMatrix);

  zcGetFrustumProjectionMatrix(this->FrustumHandle, ZC_EYE_RIGHT, &zcProjectionMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcProjectionMatrix, this->RightEyeProjectionMatrix);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateButtonState()
{
  ZSBool isButtonPressed;

  for (int buttonId = MiddleButton; buttonId < NumberOfButtons; ++buttonId)
  {
    zcIsTargetButtonPressed(this->StylusHandle, buttonId, &isButtonPressed);

    ButtonState& buttonState = *this->ButtonsState[buttonId];
    buttonState = isButtonPressed
      ? buttonState != vtkZSpaceSDKManager::Pressed ? vtkZSpaceSDKManager::Down
                                                    : vtkZSpaceSDKManager::Pressed
      : buttonState != vtkZSpaceSDKManager::None ? vtkZSpaceSDKManager::Up
                                                 : vtkZSpaceSDKManager::None;
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
  ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix)
{
  for (int i = 0; i < 16; ++i)
  {
    vtkMatrix->SetElement(i % 4, i / 4, zSpaceMatrix.f[i]);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::ConvertZSpaceMatrixToVTKMatrix(
  ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix)
{
  for (int i = 0; i < 16; ++i)
  {
    vtkMatrix->SetElement(i / 4, i % 4, zSpaceMatrix.f[i]);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::CalculateFrustumFit(
  const double bounds[6], double position[3], double viewUp[3])
{
  // Expand bounds a little bit to make sure object is not clipped
  const double w1 = bounds[1] - bounds[0];
  const double w2 = bounds[3] - bounds[2];
  const double w3 = bounds[5] - bounds[4];

  ZCBoundingBox zcBbox;
  zcBbox.lower.x = bounds[0] - w1 / 4.0;
  zcBbox.lower.y = bounds[2] - w2 / 4.0;
  zcBbox.lower.z = bounds[4] - w3 / 4.0;

  zcBbox.upper.x = bounds[1] + w1 / 4.0;
  zcBbox.upper.y = bounds[3] + w2 / 4.0;
  zcBbox.upper.z = bounds[5] + w3 / 4.0;

  ZSMatrix4 zcLookAtMatrix;
  ZSFloat zcViewerScale;

  // Calculate the appropriate viewer scale and camera lookat matrix
  // such that content in the above bounding box will take up the entire
  // viewport without being clipped.
  ZCError error =
    zcCalculateFrustumFit(this->FrustumHandle, &zcBbox, &zcViewerScale, &zcLookAtMatrix);
  ZSPACE_CHECK_ERROR(zcCalculateFrustumFit, error);

  // Set the frustum's viewer scale with the value that was calculated
  // by zcCalculateFrustumFit().
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_VIEWER_SCALE, zcViewerScale);

  this->ViewerScale = static_cast<float>(zcViewerScale);

  position[0] = -zcLookAtMatrix.m03;
  position[1] = -zcLookAtMatrix.m13;
  position[2] = -zcLookAtMatrix.m23;

  viewUp[0] = zcLookAtMatrix.m01;
  viewUp[1] = zcLookAtMatrix.m11;
  viewUp[2] = zcLookAtMatrix.m21;
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::SetClippingRange(const float nearPlane, const float farPlane)
{
  this->NearPlane = nearPlane;
  this->FarPlane = farPlane;
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceSDKManager::GetStereoViewMatrix(bool leftEye)
{
  return leftEye ? this->LeftEyeViewMatrix : this->RightEyeViewMatrix;
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceSDKManager::GetStereoProjectionMatrix(bool leftEye)
{
  return leftEye ? this->LeftEyeProjectionMatrix : this->RightEyeProjectionMatrix;
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "InterPupillaryDistance: " << this->InterPupillaryDistance << "\n";
  os << indent << "ViewerScale: " << this->ViewerScale << "\n";
  os << indent << "NearPlane: " << this->NearPlane << "\n";
  os << indent << "FarPlane: " << this->FarPlane << "\n";
  os << indent << "LeftButtonState: " << this->LeftButtonState << "\n";
  os << indent << "MiddleButtonState: " << this->MiddleButtonState << "\n";
  os << indent << "RightButtonState: " << this->RightButtonState << "\n";
}
