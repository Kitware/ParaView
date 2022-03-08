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

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtkPVZSpaceView.h"

vtkStandardNewMacro(vtkZSpaceSDKManager);

#define ZSPACE_CHECK_ERROR(fn, error)                                                              \
  if (error != ZC_COMPAT_ERROR_OK)                                                                 \
  {                                                                                                \
    std::string errorMessage = std::string("zSpace Core Compatibility API call \"") +              \
      std::string(#fn) +                                                                           \
      "\" failed with error "                                                                      \
      "code " +                                                                                    \
      std::to_string(error) + ".";                                                                 \
    vtkErrorMacro(<< "vtkZSpaceSDKManager::" << #fn << " error : " << error);                      \
  }

static const char* ZSPACE_CORE_COMPATIBILITY_DLL_FILE_PATH = "zSpaceCoreCompatibility"
#ifdef _WIN64
                                                             "64"
#else
                                                             "32"
#endif
  ;

//----------------------------------------------------------------------------
vtkZSpaceSDKManager* vtkZSpaceSDKManager::GetInstance()
{
  static vtkSmartPointer<vtkZSpaceSDKManager> instance = nullptr;
  if (instance.GetPointer() == nullptr)
  {
    instance = vtkSmartPointer<vtkZSpaceSDKManager>::New();
  }

  return instance;
}

//------------------------------------------------------------------------------
vtkZSpaceSDKManager::vtkZSpaceSDKManager()
{
  this->InitializeZSpace();
}

//------------------------------------------------------------------------------
vtkZSpaceSDKManager::~vtkZSpaceSDKManager()
{
  // this->EntryPts.zccompatDestroyStereoBuffer(this->BufferHandle);
  this->EntryPts.zccompatDestroyViewport(this->ViewportHandle);
  this->EntryPts.zccompatShutDown(this->ZSpaceContext);
}

//------------------------------------------------------------------------------
bool vtkZSpaceSDKManager::loadZspaceCoreCompatibilityEntryPoints(
  const char* zSpaceCoreCompatDllFilePath, HMODULE& dllModuleHandle,
  zSpaceCoreCompatEntryPoints& entryPoints)
{
  dllModuleHandle = LoadLibraryA(zSpaceCoreCompatDllFilePath);

  if (dllModuleHandle == nullptr)
  {
    // If the release variant of the zSpace Core Compatibility API DLL
    // could not be loaded, attempt to the debug variant instead.

    std::string zSpaceCoreCompatDllDebugFilePath(zSpaceCoreCompatDllFilePath);
    zSpaceCoreCompatDllDebugFilePath.append("_D");

    dllModuleHandle = LoadLibraryA(zSpaceCoreCompatDllDebugFilePath.c_str());

    if (dllModuleHandle == nullptr)
    {
      vtkErrorMacro(<< "Win32 Error : "
                    << "Failed to load zSpace Core Compatibility API DLL.");

      return false;
    }
  }

#define ZC_COMPAT_SAMPLE_LOCAL_LOAD_ENTRY_POINT(undecoratedFuncName)                               \
  {                                                                                                \
    void* entryPointProcAddress =                                                                  \
      GetProcAddress(dllModuleHandle, "zccompat" #undecoratedFuncName);                            \
                                                                                                   \
    if (entryPointProcAddress == nullptr)                                                          \
    {                                                                                              \
      vtkErrorMacro(<< "Win32 Error : "                                                            \
                    << "Failed to get zSpace Core Compatibility entry point "                      \
                    << "proc address for entry point "                                             \
                    << "\"zccompat" << #undecoratedFuncName << "\".");                             \
                                                                                                   \
      return false;                                                                                \
    }                                                                                              \
                                                                                                   \
    entryPoints.zccompat##undecoratedFuncName =                                                    \
      reinterpret_cast<ZCCompat##undecoratedFuncName##FuncPtrType>(entryPointProcAddress);         \
  }                                                                                                \
  /**/

  ZC_COMPAT_REFLECTION_LIST_UNDECORATED_FUNC_NAMES(ZC_COMPAT_SAMPLE_LOCAL_LOAD_ENTRY_POINT)

#undef ZC_COMPAT_SAMPLE_LOCAL_LOAD_ENTRY_POINT

  return true;
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::InitializeZSpace()
{
  const bool didSucceed = loadZspaceCoreCompatibilityEntryPoints(
    ZSPACE_CORE_COMPATIBILITY_DLL_FILE_PATH, this->zSpaceCoreCompatDllModuleHandle, this->EntryPts);

  if (!didSucceed)
  {
    return;
  }

  ZCCompatError error;

  // Initialize the zSpace SDK. This MUST be called before
  // calling any other zSpace API.
  error = this->EntryPts.zccompatInitialize(nullptr, nullptr, &this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatInitialize, error);

  // Check the SDK version
  ZSInt32 major, minor, patch;
  error = this->EntryPts.zccompatGetRuntimeVersion(this->ZSpaceContext, &major, &minor, &patch);
  ZSPACE_CHECK_ERROR(zccompatGetRuntimeVersion, error);

  vtkDebugMacro(<< "zSpace SDK version: " << major << "." << minor << "." << patch);

  int numDisplays;
  error = this->EntryPts.zccompatGetNumDisplays(this->ZSpaceContext, &numDisplays);

  this->Displays.reserve(numDisplays);

  for (int i = 0; i < numDisplays; i++)
  {
    ZCCompatDisplay displayHandle;
    this->EntryPts.zccompatGetDisplayByIndex(this->ZSpaceContext, i, &displayHandle);

    ZCCompatDisplayType displayType;
    this->EntryPts.zccompatGetDisplayType(displayHandle, &displayType);

    switch (displayType)
    {
      case ZC_COMPAT_DISPLAY_TYPE_GENERIC:
        this->Displays.push_back("Generic");
        break;
      case ZC_COMPAT_DISPLAY_TYPE_ZSPACE:
        this->Displays.push_back("ZSpace");
        break;
      default:
        this->Displays.push_back("Unknown");
        break;
    }
  }

  // Create a stereo buffer to handle L/R detection.
  // error = this->EntryPts.zccompatCreateStereoBuffer(this->ZSpaceContext,
  //   ZC_COMPAT_RENDERER_QUAD_BUFFER_GL, 0, &this->BufferHandle);
  // ZSPACE_CHECK_ERROR(zccompatCreateStereoBuffer, error);

  // Create a zSpace viewport object and grab its associated frustum.
  // Note: The zSpace viewport is abstract and not an actual window/viewport
  // that is created and registered through the Windows OS. It manages
  // a zSpace stereo frustum, which is responsible for various stereoscopic
  // 3D calculations such as calculating the view and projection matrices for
  // each eye.

  // error = this->EntryPts.zccompatCreateViewport(this->ZSpaceContext, &this->ViewportHandle);
  error = this->EntryPts.zccompatGetPrimaryViewport(
    this->ZSpaceContext, &this->ViewportHandle); // Not create but retrieve
  ZSPACE_CHECK_ERROR(zccompatCreateViewport, error);

  error = this->EntryPts.zccompatGetFrustum(this->ViewportHandle, &this->FrustumHandle);
  ZSPACE_CHECK_ERROR(zccompatGetFrustum, error);

  // Enable auto stereo.
  this->EntryPts.zccompatSetFrustumAttributeB(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_AUTO_STEREO_ENABLED, true);
  this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);
  this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_HEAD_SCALE, 1.f);

  // Disable the portal mode
  this->EntryPts.zccompatSetFrustumPortalMode(this->FrustumHandle, 0);

  error = this->EntryPts.zccompatGetNumTargetsByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_PRIMARY, &this->StylusTargets);
  ZSPACE_CHECK_ERROR(zccompatGetNumTargetsByType, error);

  error = this->EntryPts.zccompatGetNumTargetsByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_HEAD, &this->HeadTargets);
  ZSPACE_CHECK_ERROR(zccompatGetNumTargetsByType, error);

  error = this->EntryPts.zccompatGetNumTargetsByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_SECONDARY, &this->SecondaryTargets);
  ZSPACE_CHECK_ERROR(zccompatGetNumTargetsByType, error);

  // Grab a handle to the stylus target.
  error = this->EntryPts.zccompatGetTargetByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_PRIMARY, 0, &this->StylusHandle);
  ZSPACE_CHECK_ERROR(zccompatGetTargetByType, error);

  // Find the zSpace display and set the window position
  // to be the top left corner of the zSpace display.
  error = this->EntryPts.zccompatGetDisplayByType(
    this->ZSpaceContext, ZC_COMPAT_DISPLAY_TYPE_ZSPACE, 0, &this->DisplayHandle);
  ZSPACE_CHECK_ERROR(zccompatGetDisplayByType, error);

  error =
    this->EntryPts.zccompatGetDisplayPosition(this->DisplayHandle, &this->WindowX, &this->WindowY);
  ZSPACE_CHECK_ERROR(zccompatGetDisplayPosition, error);

  error = this->EntryPts.zccompatGetDisplayNativeResolution(
    this->DisplayHandle, &this->WindowWidth, &this->WindowHeight);
  ZSPACE_CHECK_ERROR(zccompatGetDisplayNativeResolution, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::Update(vtkRenderWindow* renWin)
{
  this->UpdateViewport(renWin);
  this->UpdateViewAndProjectionMatrix();
  this->UpdateTrackers();
  this->UpdateButtonState();

  // prepare draw
  // this->EntryPts.zccompatBeginStereoBufferFrame(this->BufferHandle);

  // this->EntryPts.zccompatBeginFrame(this->ZSpaceContext); // Keep
  // TODO : needs end somewhere ?
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateViewport(vtkRenderWindow* renWin)
{
  ZCCompatError error;

  static bool isInitialized = false;

  if (!isInitialized)
  {
    auto* rw = vtkGenericOpenGLRenderWindow::SafeDownCast(renWin);
    if (!rw)
    {
      vtkErrorMacro("Unable to cast render window.");
      renWin->Print(std::cout);
    }
    else
    {
      // Give the application window handle to the zSpace Core Compatibilty API.
      HWND hWnd = static_cast<HWND>(rw->GetGenericWindowId());
      this->EntryPts.zccompatSetApplicationWindowHandle(this->ZSpaceContext, hWnd);
    }
    isInitialized = true;
  }

  int* position = renWin->GetPosition();
  int* size = renWin->GetSize();

  error =
    this->EntryPts.zccompatSetViewportPosition(this->ViewportHandle, position[0], position[1]);
  ZSPACE_CHECK_ERROR(zccompatSetViewportPosition, error);
  error = this->EntryPts.zccompatSetViewportSize(this->ViewportHandle, size[0], size[1]);
  ZSPACE_CHECK_ERROR(zccompatSetViewportSize, error);

  // Update inter pupillary distance
  this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);

  // Near and far plane
  this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_NEAR_CLIP, this->NearPlane);
  this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_FAR_CLIP, this->FarPlane);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateTrackers()
{
  ZCCompatError error;

  // Update the zSpace SDK. This updates both tracking information
  // as well as the head poses for any frustums that have been created.
  error = this->EntryPts.zccompatUpdate(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatUpdate, error);

  // Update the stylus matrix
  ZCCompatTrackerPose stylusPose;
  // error = this->EntryPts.zccompatGetTargetTransformedPose(
  //   this->StylusHandle, this->ViewportHandle, ZC_COMPAT_COORDINATE_SPACE_CAMERA, &stylusPose);
  error = this->EntryPts.zccompatGetTargetPose(this->StylusHandle, &stylusPose);
  ZSPACE_CHECK_ERROR(zccompatGetTargetPose, error);

  error =
    this->EntryPts.zccompatTransformMatrix(this->ViewportHandle, ZC_COMPAT_COORDINATE_SPACE_TRACKER,
      ZC_COMPAT_COORDINATE_SPACE_CAMERA, &stylusPose.matrix); // Manual transfo added
  ZSPACE_CHECK_ERROR(zccompatGetTargetPose, error);

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
  ZSMatrix4 zccompatViewMatrix;
  this->EntryPts.zccompatGetFrustumViewMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_CENTER, &zccompatViewMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatViewMatrix, this->CenterEyeViewMatrix);

  this->EntryPts.zccompatGetFrustumViewMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_LEFT, &zccompatViewMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatViewMatrix, this->LeftEyeViewMatrix);

  this->EntryPts.zccompatGetFrustumViewMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_RIGHT, &zccompatViewMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatViewMatrix, this->RightEyeViewMatrix);

  // Update the projection matrix for each eye
  ZSMatrix4 zccompatProjectionMatrix;
  this->EntryPts.zccompatGetFrustumProjectionMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_CENTER, &zccompatProjectionMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatProjectionMatrix, this->CenterEyeProjectionMatrix);

  this->EntryPts.zccompatGetFrustumProjectionMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_LEFT, &zccompatProjectionMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatProjectionMatrix, this->LeftEyeProjectionMatrix);

  this->EntryPts.zccompatGetFrustumProjectionMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_RIGHT, &zccompatProjectionMatrix);
  vtkZSpaceSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatProjectionMatrix, this->RightEyeProjectionMatrix);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::UpdateButtonState()
{
  ZSBool isButtonPressed;

  for (int buttonId = MiddleButton; buttonId < NumberOfButtons; ++buttonId)
  {
    this->EntryPts.zccompatIsTargetButtonPressed(this->StylusHandle, buttonId, &isButtonPressed);

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
  ZCCompatError error;

  // Expand the bounding box a little bit to make sure the object is not clipped
  const double bBoxWidth = bounds[1] - bounds[0];
  const double bBoxHeight = bounds[3] - bounds[2];
  const double bBoxDepth = bounds[5] - bounds[4];

  vtkVector3d lowerBound;
  lowerBound.SetX(bounds[0] - bBoxWidth * 0.25f);
  lowerBound.SetY(bounds[2] - bBoxHeight * 0.25f);
  lowerBound.SetZ(bounds[4] - bBoxDepth * 0.25f);

  vtkVector3d upperBound;
  upperBound.SetX(bounds[1] + bBoxWidth * 0.25f);
  upperBound.SetY(bounds[3] + bBoxHeight * 0.25f);
  upperBound.SetZ(bounds[5] + bBoxDepth * 0.25f);

  // Retrieve viewport size (pixels)
  ZSInt32 viewportResWidth = 0;
  ZSInt32 viewportResHeight = 0;
  error = this->EntryPts.zccompatGetViewportSize(
    this->ViewportHandle, &viewportResWidth, &viewportResHeight);
  ZSPACE_CHECK_ERROR(zccompatGetViewportSize, error);

  // Retrieve display size (meters)
  ZSFloat displayWidth = 0.0f;
  ZSFloat displayHeight = 0.0f;
  error = this->EntryPts.zccompatGetDisplaySize(this->DisplayHandle, &displayWidth, &displayHeight);
  ZSPACE_CHECK_ERROR(zccompatGetViewportSize, error);

  // Retrieve display resolution
  ZSInt32 displayResWidth = 0;
  ZSInt32 displayResHeight = 0;
  error = this->EntryPts.zccompatGetDisplayNativeResolution(
    this->DisplayHandle, &displayResWidth, &displayResHeight);

  // Retrieve coupled zone maximum depth value for positive parallax
  ZSFloat ppMaxDepth = 0.0f;
  error = this->EntryPts.zccompatGetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_UC_DEPTH, &ppMaxDepth);
  ZSPACE_CHECK_ERROR(zccompatGetViewportSize, error);

  // Retrieve coupled zone maximum depth value for negative parallax
  ZSFloat npMaxDepth = 0.0f;
  error = this->EntryPts.zccompatGetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_CC_DEPTH, &npMaxDepth);
  ZSPACE_CHECK_ERROR(zccompatGetViewportSize, error);

  // Compute viewport size in meters
  float viewportWidth = (static_cast<float>(viewportResWidth) / displayResWidth) * displayWidth;
  float viewportHeight = (static_cast<float>(viewportResHeight) / displayResHeight) * displayHeight;

  // Compute viewer scale as the maximum of widthScale, heightScale, and depthScale.
  float widthScale = (upperBound.GetX() - lowerBound.GetX()) / viewportWidth;
  float heightScale = (upperBound.GetY() - lowerBound.GetY()) / viewportHeight;
  float depthScale = (upperBound.GetZ() - lowerBound.GetZ()) / (npMaxDepth - ppMaxDepth);

  this->ViewerScale = std::max(depthScale, std::max(widthScale, heightScale));

  // Get frustum's camera offset (distance to world center)
  ZSVector3 zsCameraOffset = { 0.0, 0.0, 0.0 };
  error = this->EntryPts.zccompatGetFrustumCameraOffset(this->FrustumHandle, &zsCameraOffset);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumCameraOffset, error);

  // Compute new frustum's camera viewUp and position
  vtkVector3d worldCenter = (lowerBound + upperBound) * 0.5f;
  vtkVector3d cameraOffset = { zsCameraOffset.x, zsCameraOffset.y, zsCameraOffset.z };
  vtkVector3d cameraForward = (-cameraOffset).Normalized();
  vtkVector3d cameraRight = { 1.0f, 0.0f, 0.0f };
  vtkVector3d cameraUp = cameraRight.Cross(cameraForward);
  vtkVector3d cameraPosition =
    worldCenter - (cameraForward * cameraOffset.Norm() * this->ViewerScale);

  viewUp[0] = cameraUp.GetX();
  viewUp[1] = cameraUp.GetY();
  viewUp[2] = cameraUp.GetZ();

  position[0] = cameraPosition.GetX();
  position[1] = cameraPosition.GetY();
  position[2] = cameraPosition.GetZ();

  // Set the frustum's viewer scale with the value that was calculated.
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_VIEWER_SCALE, this->ViewerScale);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumCameraOffset, error);

  ///////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>

  std::ofstream myfile;
  myfile.open("C:\\Users\\tgalland\\outputNew.txt");

  myfile << "Display width (pixels): " << displayResWidth << endl;
  myfile << "Display height (pixels): " << displayResHeight << endl;
  myfile << "Display width (meters): " << displayWidth << endl;
  myfile << "Display height (meters): " << displayHeight << endl;
  myfile << "Viewport width (pixels): " << viewportResWidth << endl;
  myfile << "Viewport height (pixels): " << viewportResHeight << endl;
  myfile << "Viewport width (meters): " << viewportWidth << endl;
  myfile << "Viewport height (meters): " << viewportHeight << endl;
  myfile << "Positive max depth: " << ppMaxDepth << endl;
  myfile << "Negative max depth: " << npMaxDepth << endl;
  myfile << endl;

  myfile << "WidthScale: " << widthScale << endl;
  myfile << "HeightScale: " << heightScale << endl;
  myfile << "DepthScale: " << depthScale << endl;
  myfile << "Viewer scale: " << this->ViewerScale << endl;
  myfile << endl;

  myfile << "World center: " << worldCenter[0] << " " << worldCenter[1] << " " << worldCenter[2]
         << endl;
  myfile << "Camera offset: " << cameraOffset[0] << " " << cameraOffset[1] << " " << cameraOffset[2]
         << endl;
  myfile << endl;

  myfile << "ViewUp: " << viewUp[0] << " " << viewUp[1] << " " << viewUp[2] << endl;
  myfile << "Position: " << position[0] << " " << position[1] << " " << position[2] << endl;

  myfile.close();
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
void vtkZSpaceSDKManager::BeginFrame()
{
  this->EntryPts.zccompatBeginFrame(this->ZSpaceContext);
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::EndFrame()
{
  this->EntryPts.zccompatEndFrame(this->ZSpaceContext);
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
