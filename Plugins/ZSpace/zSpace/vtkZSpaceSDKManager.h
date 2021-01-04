/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkZSpaceSDKManager
* @brief   zSpace SDK manager class
*
*
* Encapsulates all the calls to the zSpace SDK :
*  - Initializes the zSpace SDK via InitializeZSpace(). This method looks
*     for a zSpace device and optional trackers
*  - Updates the zSpace SDK via Update(vtkRenderWindow)
*     the viewport (position, interpupillary distance, near and far plane);
*     the view and projection matrix for each eye;
*     the trackers (head pose and trackers pose such as the stylus);
*     and the state of the buttons of the stylus (Down, Pressed, Up or None).
*
*  For button states, the states Down/Up are set by this class; whereas the states
*  Pressed/None should be set by the calling class when the state Down/Up has been processed, to
*  ensure that the same input won't be processed multiple times.
*/

#ifndef vtkZSpaceSDKManager_h
#define vtkZSpaceSDKManager_h

#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkZSpaceViewModule.h" // for export macro

#include <vector>
#include <zspace.h>

class vtkRenderWindow;
class vtkCamera;
class vtkMatrix4x4;
class vtkTransform;
class vtkPVZSpaceView;

class VTKZSPACEVIEW_EXPORT vtkZSpaceSDKManager : public vtkObject
{
public:
  static vtkZSpaceSDKManager* New();
  vtkTypeMacro(vtkZSpaceSDKManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the zSpace SDK and check for zSpace devices :
   * the display, the stylus and the head trackers.
   */
  void InitializeZSpace();

  /**
   * Update the viewport, the trackers and the camera matrix
   * by calling the zSpace SDK.
   */
  void Update(vtkRenderWindow*);

  /**
   * Update the zSpace viewport position and size based
   * on the position and size of the application window.
   */
  void UpdateViewport(vtkRenderWindow*);

  /**
   * Update the position of the stylus and head trakers.
   */
  void UpdateTrackers();

  /**
   * Update the zSpace view and projection matrix for each eye.
   */
  void UpdateViewAndProjectionMatrix();

  /**
   * Update the stylus buttons state.
   */
  void UpdateButtonState();

  /**
   * Get the x position of the upper left corner of the zSpace display
   * in the virtual desktop.
   */
  vtkGetMacro(WindowX, int);

  /**
   * Get the y position of the upper left corner of the zSpace display
   * in the virtual desktop.
   */
  vtkGetMacro(WindowY, int);

  /**
   * Get the x resolution in pixels of the zSpace display.
   */
  vtkGetMacro(WindowWidth, int);

  /**
   * Get the y resolution in pixels of the zSpace display.
   */
  vtkGetMacro(WindowHeight, int);

  /**
   * Get the number of stylus connected to the zSpace device.
   */
  vtkGetMacro(StylusTargets, int);

  /**
   * Get the number of glasses connected to the zSpace device.
   */
  vtkGetMacro(HeadTargets, int);

  /**
   * Get the number of secondary targets connected to the zSpace device.
   */
  vtkGetMacro(SecondaryTargets, int);

  //@{
  /**
   * Get/Set the distance between the eyes in meters.
   */
  vtkGetMacro(InterPupillaryDistance, float);
  vtkSetClampMacro(InterPupillaryDistance, float, 0.f, 1.f);
  //@}

  /**
   * Let zSpace compute the viewer scale, camera position and camera view up from the
   * input bounds.
   */
  void CalculateFrustumFit(const double bounds[6], double position[3], double viewUp[3]);

  /**
   * Set the near and far plane.
   */
  void SetClippingRange(const float nearPlane, const float farPlane);

  /**
   * Get the viewer scale.
   */
  vtkGetMacro(ViewerScale, float);

  /**
   * Get the near plane.
   */
  vtkGetMacro(NearPlane, float);

  /**
   * Get the far plane.
   */
  vtkGetMacro(FarPlane, float);

  /**
   * zSpace stores matrix in column-major format (as OpenGL). The matrix
   * needs to be transposed to be used by VTK.
   */
  static void ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix);

  /**
  * zSpace stores matrix in column-major format (as OpenGL). The matrix
  * needs to be transposed to be used by VTK.
  */
  static void ConvertZSpaceMatrixToVTKMatrix(ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix);

  /**
   * Get the zSpace view matrix without stereo (eye set as EYE_CENTER)
   * in row major format (VTK format)
   */
  vtkGetObjectMacro(CenterEyeViewMatrix, vtkMatrix4x4);

  /**
   * Get the zSpace view matrix for the right or left eye
   * in row major format (VTK format)
   */
  vtkMatrix4x4* GetStereoViewMatrix(bool leftEye);

  /**
   * Get the zSpace projection matrix without stereo (eye set as EYE_CENTER)
   * in row major format (VTK format)
   */
  vtkGetObjectMacro(CenterEyeProjectionMatrix, vtkMatrix4x4);

  /**
   * Get the zSpace projection matrix for the right or left eye
   * in row major format (VTK format)
   */
  vtkMatrix4x4* GetStereoProjectionMatrix(bool leftEye);

  /**
   * Get the zSpace stylus matrix in world space.
   * The matrix is in column major format and can be
   * used by OpenGL.
   */
  vtkGetObjectMacro(StylusMatrixColMajor, vtkMatrix4x4);

  /**
   * Get the zSpace stylus matrix in world space.
   * The matrix is in row major format and can be
   * used by VTK.
   */
  vtkGetObjectMacro(StylusMatrixRowMajor, vtkMatrix4x4);

  /**
   * Get the zSpace stylus transform in world space.
   * The transform has for matrix StylusMatrixRowMajor
   */
  vtkGetObjectMacro(StylusTransformRowMajor, vtkTransform);

  enum ButtonIds
  {
    MiddleButton = 0,
    RightButton = 1,
    LeftButton = 2,
    NumberOfButtons = 3
  };

  enum ButtonState
  {
    Down = 0,
    Pressed = 1,
    Up = 2,
    None = 3,
    NumberOfStates = 4
  };

  //@{
  /**
   * Get/Set the state of the left button of the stylus.
   */
  vtkGetMacro(LeftButtonState, int);
  vtkSetEnumMacro(LeftButtonState, ButtonState);
  //@}

  //@{
  /**
   * Get/Set the state of the middle button of the stylus.
   */
  vtkGetMacro(MiddleButtonState, int);
  vtkSetEnumMacro(MiddleButtonState, ButtonState);
  //@}

  //@{
  /**
   * Get/Set the state of the right button of the stylus.
   */
  vtkGetMacro(RightButtonState, int);
  vtkSetEnumMacro(RightButtonState, ButtonState);
  //@}

protected:
  vtkZSpaceSDKManager();
  ~vtkZSpaceSDKManager() override;

  ZCContext ZSpaceContext = nullptr;
  ZCHandle DisplayHandle = nullptr;
  ZCHandle BufferHandle = nullptr;
  ZCHandle ViewportHandle = nullptr;
  ZCHandle FrustumHandle = nullptr;
  ZCHandle StylusHandle = nullptr;

  vtkNew<vtkMatrix4x4> CenterEyeViewMatrix;
  vtkNew<vtkMatrix4x4> LeftEyeViewMatrix;
  vtkNew<vtkMatrix4x4> RightEyeViewMatrix;
  vtkNew<vtkMatrix4x4> CenterEyeProjectionMatrix;
  vtkNew<vtkMatrix4x4> LeftEyeProjectionMatrix;
  vtkNew<vtkMatrix4x4> RightEyeProjectionMatrix;

  // In column major format, used by openGL
  vtkNew<vtkMatrix4x4> StylusMatrixColMajor;

  // In row major format, used by VTK
  vtkNew<vtkMatrix4x4> StylusMatrixRowMajor;
  vtkNew<vtkTransform> StylusTransformRowMajor;

  int WindowX = 0;
  int WindowY = 0;
  int WindowWidth = 0;
  int WindowHeight = 0;

  // Store the type for each detected display devices
  std::vector<std::string> Displays;
  // The number of stylus
  int StylusTargets;
  // The number of glasses
  int HeadTargets;
  // Additional targets
  int SecondaryTargets;

  // Inter pupillary distance in meters
  float InterPupillaryDistance = 0.056f;
  float ViewerScale = 1.f;
  // Camera near plane
  float NearPlane = 0.01f;
  // Camera far plane
  float FarPlane = 10.f;

  // For interactions, store the state of each buttons
  ButtonState LeftButtonState = None;
  ButtonState MiddleButtonState = None;
  ButtonState RightButtonState = None;
  // Store buttons state to iterate over them
  ButtonState* ButtonsState[NumberOfButtons] = { &MiddleButtonState, &RightButtonState,
    &LeftButtonState };

private:
  vtkZSpaceSDKManager(const vtkZSpaceSDKManager&) = delete;
  void operator=(const vtkZSpaceSDKManager&) = delete;
};

#endif
