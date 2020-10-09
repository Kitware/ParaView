/*=========================================================================

  Program:   ParaView
  Module:    vtkPVZSpaceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVZSpaceView
 *
 * vtkPVZSpaceView extends vtkPVRenderView to render the scene using Crystal Eyes
 * stereo and interact with it through a zSpace device.
 * The zSpace device is composed of a high definition 3D display coupled with
 * an advanced mixed reality tracking system and angle-awareness sensor.
 *
 * This view is designed to work on full screen (or in a cave display).
 *
 * This only works on Windows as the zSpace SDK is only available on this OS.
 */

#ifndef vtkPVPanoramicProjectionView_h
#define vtkPVPanoramicProjectionView_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVRenderView.h"
#include "vtkZSpaceViewModule.h" // for export macro

#include "vtkActor.h"
#include "vtkLineSource.h"
#include "vtkVector.h"

#include <zSpace.h>

class VTKZSPACEVIEW_EXPORT vtkPVZSpaceView : public vtkPVRenderView
{
public:
  static vtkPVZSpaceView* New();
  vtkTypeMacro(vtkPVZSpaceView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  /**
   * Get/Set the distance between the eyes in meters.
   */
  vtkGetMacro(InterPupillaryDistance, float);
  vtkSetClampMacro(InterPupillaryDistance, float, 0.f, 1.f);

  /**
   * Compute camera position, view up, focal point and frustum near and far clip
   * From the scene bounding box. Compute the bounding box and call CalculateFit
   * with corresponding parameters.
   */
  void UpdateZSpaceFrustumParameters(bool setPosition = false);

protected:
  vtkPVZSpaceView();
  ~vtkPVZSpaceView() override;

  void Render(bool interactive, bool skip_rendering) override;

  /**
   * Initialize the zSpace SDK and check for the zSpace devices :
   * the display, the stylus and the head trackers.
   */
  void InitializeZSpace();

  /**
   * Update the position of the stylus and head trakers.
   */
  void UpdateTrackers();

  /**
   * Get the view and projection matrix from zSpace and give them
   * to the active camera.
   */
  void UpdateCamera();

  /**
   * Give to the zSpace SDK the scene bounds to let it choose the best
   * viewer scale, near and far clip. If setPosition is true,
   * the camera position is set with the zSpace advised position.
   */
  void CalculateFit(double* bounds, bool setPosition = false);

  /**
   * Called when a command vtkCommand::ResetCameraEvent is fired.
   */
  void ResetCamera();

  /**
   * Override ResetCameraClippingRange to disable automatic clipping range
   * calculations of the camera as it is done by the zSpace SDK.
   */
  void ResetCameraClippingRange() override{};

private:
  vtkPVZSpaceView(const vtkPVZSpaceView&) = delete;
  void operator=(const vtkPVZSpaceView&) = delete;

  ZCContext ZSpaceContext = nullptr;
  ZCHandle DisplayHandle = nullptr;
  ZCHandle BufferHandle = nullptr;
  ZCHandle ViewportHandle = nullptr;
  ZCHandle FrustumHandle = nullptr;
  ZCHandle StylusHandle = nullptr;

  int WindowX = 0;
  int WindowY = 0;

  int WindowWidth = 0;
  int WindowHeight = 0;

  // Inter pupillary distance in meters
  float InterPupillaryDistance = 0.056f;

  // Store the type for each detected display devices
  std::vector<std::string> Displays;
  // The number of stylus
  int StylusTargets;
  // The number of glasses
  int HeadTargets;
  // Additional targets
  int SecondaryTargets;
};

#endif
